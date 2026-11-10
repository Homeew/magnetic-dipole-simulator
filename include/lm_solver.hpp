#pragma once
#include "dipole.hpp"        // DipoleSource — what we are estimating
#include "sensor_array.hpp"  // SensorArray — where measurements come from
#include <vector>            // std::vector
#include <array>             // std::array for the 6-element parameter step
#include <string>            // std::string for the result message
#include <cstddef>           // std::size_t

/**
 * LMResult holds everything the solver reports after it finishes.
 */
struct LMResult {
    DipoleSource estimated;       // the best-fit dipole found
    double       finalCost{0};    // 0.5 * ||residuals||² at the solution
    int          iterations{0};   // how many iterations it took
    bool         converged{false};// true if a tolerance was met
    std::string  message;         // human-readable reason for stopping
};

/**
 * LMSolver implements the Levenberg-Marquardt (LM) algorithm — a standard
 * method for nonlinear least-squares problems.
 *
 * THE PROBLEM:
 *   Given noisy B measurements from N sensors, find the dipole parameters
 *   x = [px, py, pz, mx, my, mz] that minimise:
 *
 *     F(x) = 0.5 * sum_i  ||B_predicted(x, sensor_i) - B_measured_i||²
 *
 *   This is nonlinear because B depends on x in a nonlinear way (1/r³, dot products).
 *
 * THE METHOD (Levenberg-Marquardt):
 *   Each iteration solves the linear system:
 *
 *     (J'J + λI) Δx = -J'r
 *
 *   where:
 *     J  = Jacobian matrix (how each residual changes with each parameter)
 *     r  = residual vector (predicted - measured, for all sensors/components)
 *     λ  = damping factor (controls the step size)
 *     Δx = parameter update to apply
 *
 *   - If the new cost is lower  → accept Δx, decrease λ (more like Gauss-Newton)
 *   - If the new cost is higher → reject Δx, increase λ (more like gradient descent)
 *
 * THE JACOBIAN:
 *   Computed by central finite differences — no analytical derivatives needed:
 *     J[i][j] ≈ ( r(x + h*ej) - r(x - h*ej) ) / (2h)
 */
class LMSolver {
public:

    // All tuning parameters for the solver
    struct Options {
	Options() = default;
        double lambda0      = 1e-3;   // initial damping factor λ
        double lambdaFactor = 10.0;   // multiply/divide λ on failure/success
        double ftol         = 1e-10;  // stop if relative cost change < ftol
        double xtol         = 1e-10;  // stop if parameter step size < xtol
        double gtol         = 1e-10;  // stop if gradient norm < gtol
        double fdStep       = 1e-6;   // finite-difference step h
        int    maxIter      = 500;    // hard limit on iterations
        bool   verbose      = false;  // print cost at each iteration
    };

    // construct with default or custom options
    LMSolver() : opts_(Options{}) {}
    explicit LMSolver(Options opts) : opts_(opts) {}
    /**
     * Run the solver and return the result.
     *
     * @param array   the sensor array (positions used for predictions)
     * @param meas    measured B vectors, one per sensor
     * @param init    initial guess for the dipole (starting point)
     */
    LMResult solve(const SensorArray&       array,
                   const std::vector<Vec3>& meas,
                   const DipoleSource&      init) const;

private:
    Options opts_;

    static constexpr int NP = 6;   // number of parameters: [px,py,pz,mx,my,mz]

    // Build residual vector: r[i] = predicted[i] - measured[i]
    // size = 3*N  (Bx, By, Bz for each of the N sensors)
    static void computeResiduals(const DipoleSource&      src,
                                 const SensorArray&       array,
                                 const std::vector<Vec3>& meas,
                                 std::vector<double>&     r);

    // cost = 0.5 * ||r||²
    static double cost(const std::vector<double>& r);

    // Finite-difference Jacobian: J is (3*N) × 6, stored row-major
    static void computeJacobian(const DipoleSource&      src,
                                const SensorArray&       array,
                                const std::vector<Vec3>& meas,
                                double                   h,
                                std::vector<double>&     J);

    // Solve (J'J + λI) Δx = -J'r  via Gaussian elimination
    // Returns false if the matrix is singular
    static bool solveNormalEquations(const std::vector<double>& J,
                                     const std::vector<double>& r,
                                     double                     lambda,
                                     std::size_t                nRes,
                                     std::array<double, NP>&    dx);
};
