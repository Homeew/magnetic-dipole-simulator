#include "lm_solver.hpp"  // the class we are implementing
#include <cstring>        // std::memcpy for copying parameter arrays
#include <cmath>          // std::fabs, std::sqrt
#include <iostream>       // std::cout for verbose output
#include <iomanip>        // std::setw, std::setprecision for formatting

// ── computeResiduals ──────────────────────────────────────────────────────
//
// Fills vector r with:  r[3i+0] = Bx_pred - Bx_meas
//                       r[3i+1] = By_pred - By_meas
//                       r[3i+2] = Bz_pred - Bz_meas
//
// for each sensor i.  Total size = 3 * N.
//
void LMSolver::computeResiduals(const DipoleSource&      src,
                                const SensorArray&       array,
                                const std::vector<Vec3>& meas,
                                std::vector<double>&     r)
{
    const std::size_t N = array.size();
    r.resize(3 * N);

    for (std::size_t i = 0; i < N; ++i) {
        Vec3 pred = src.fieldAt(array.sensors[i].position);  // predicted field
        r[3*i+0]  = pred.x - meas[i].x;   // x residual
        r[3*i+1]  = pred.y - meas[i].y;   // y residual
        r[3*i+2]  = pred.z - meas[i].z;   // z residual
    }
}

// ── cost ─────────────────────────────────────────────────────────────────
//
// F = 0.5 * ||r||²  =  0.5 * sum of squared residuals
// The 0.5 is a convention — it makes the gradient cleaner: dF/dx = J'r
//
double LMSolver::cost(const std::vector<double>& r) {
    double s = 0;
    for (double v : r) s += v * v;   // sum of squares
    return 0.5 * s;
}

// ── computeJacobian ───────────────────────────────────────────────────────
//
// The Jacobian J is a matrix of size (3*N) × 6:
//   J[i][j] = dr_i / dp_j
//
// We compute it by central finite differences:
//   J[i][j] ≈ ( r(p + h*ej) - r(p - h*ej) ) / (2h)
//
// where ej is the unit vector along parameter j.
// Central differences are more accurate than forward differences (O(h²) vs O(h)).
//
void LMSolver::computeJacobianAnalytical(const DipoleSource&  src,
                                          const SensorArray&   array,
                                          std::vector<double>& J)
{
    const std::size_t N    = array.size();
    const std::size_t nRes = 3 * N;
    J.resize(nRes * NP);

    const double coeff0 = constants::mu0 / (4.0 * M_PI);

    for (std::size_t i = 0; i < N; ++i) {
        Vec3 r = array.sensors[i].position - src.position;
        double R = r.norm();
        if (R < 1e-15) {
            for (int j = 0; j < NP; ++j)
                J[(3*i)*NP+j] = J[(3*i+1)*NP+j] = J[(3*i+2)*NP+j] = 0;
            continue;
        }

        double R2    = R * R;
        double R3    = R2 * R;
        Vec3   rhat  = r / R;
        double C     = coeff0 / R3;
        double md    = src.moment.dot(rhat);  // m · r̂

        double rx = rhat.x, ry = rhat.y, rz = rhat.z;
        double mx = src.moment.x, my = src.moment.y, mz = src.moment.z;

        // ── dB/dm  (columns 3,4,5) ───────────────────────────────────────
        // dB/dm_j = C * [3*r̂_j*r̂ - e_j]
        double rh[3] = {rx, ry, rz};
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                double val = C * (3.0*rh[j]*rh[k] - (j==k ? 1.0 : 0.0));
                J[(3*i+k)*NP+(3+j)] = val;
            }
        }

        // ── dB/dp  (columns 0,1,2) ───────────────────────────────────────
        // r = sensor - p  =>  dr/dp_j = -e_j
        //
        // dB/dp_j = (3C·r_j/R²)·[3md·r̂ - m]
        //         + (3C/R)·[-m_j·r̂  +  2·r̂_j·md·r̂  -  md·e_j]
        double r_arr[3] = {r.x, r.y, r.z};
        double m_arr[3] = {mx, my, mz};
        double B_arr[3] = {3*md*rx - mx, 3*md*ry - my, 3*md*rz - mz};

        for (int j = 0; j < 3; ++j) {
            double t1 = 3.0 * C * r_arr[j] / R2;
            double t2 = 3.0 * C / R;
            for (int k = 0; k < 3; ++k) {
                J[(3*i+k)*NP+j] = t1 * B_arr[k]
                                 + t2 * (-m_arr[j]*rh[k]
                                         + 2.0*rh[j]*md*rh[k]
                                         - md*(j==k ? 1.0 : 0.0));
            }
        }
    }
}
// ── solveNormalEquations ─────────────────────────────────────────────────
//
// Solves the 6×6 linear system:  (J'J + λI) Δx = -J'r
//
// Step 1: form A = J'J + λI  and  b = -J'r
// Step 2: solve Ax = b using Gaussian elimination with partial pivoting
//
// Returns false if the matrix is singular (can't be solved).
//
bool LMSolver::solveNormalEquations(const std::vector<double>& J,
                                     const std::vector<double>& r,
                                     double                     lambda,
                                     std::size_t                nRes,
                                     std::array<double, NP>&    dx)
{
    double A[NP][NP] = {};   // A = J'J + λI  (6×6, zero-initialized)
    double b[NP]     = {};   // b = -J'r       (6×1, zero-initialized)

    // Form A and b by accumulating over all residuals
    for (std::size_t i = 0; i < nRes; ++i) {
        for (int j = 0; j < NP; ++j) {
            double jij = J[i*NP+j];     // J[i][j]
            b[j] -= jij * r[i];         // b = -J'r
            for (int k = 0; k < NP; ++k)
                A[j][k] += jij * J[i*NP+k];  // A = J'J
        }
    }

    // Add damping: A = J'J + λI
    for (int j = 0; j < NP; ++j) A[j][j] += lambda;

    // Augmented matrix [A | b] for Gaussian elimination
    double aug[NP][NP+1];
    for (int j = 0; j < NP; ++j) {
        for (int k = 0; k < NP; ++k) aug[j][k] = A[j][k];
        aug[j][NP] = b[j];   // last column is the right-hand side
    }

    // Gaussian elimination with partial pivoting
    for (int col = 0; col < NP; ++col) {

        // find the row with the largest value in this column (pivot)
        int pivot = col;
        for (int row = col+1; row < NP; ++row)
            if (std::fabs(aug[row][col]) > std::fabs(aug[pivot][col]))
                pivot = row;

        // if largest value is ~0 the matrix is singular
        if (std::fabs(aug[pivot][col]) < 1e-15) return false;

        // swap current row with pivot row
        std::swap(aug[col], aug[pivot]);

        // normalize the pivot row so diagonal = 1
        double scale = aug[col][col];
        for (int k = col; k <= NP; ++k) aug[col][k] /= scale;

        // eliminate this column from all other rows
        for (int row = 0; row < NP; ++row) {
            if (row == col) continue;
            double f = aug[row][col];
            for (int k = col; k <= NP; ++k) aug[row][k] -= f * aug[col][k];
        }
    }

    // solution is now in the last column of aug
    for (int j = 0; j < NP; ++j) dx[j] = aug[j][NP];
    return true;
}

// ── solve (main loop) ─────────────────────────────────────────────────────
//
// The core LM iteration:
//
//   1. Compute residuals r and cost F
//   2. Compute Jacobian J
//   3. Solve (J'J + λI) Δx = -J'r
//   4. Try the step: compute new cost at x + Δx
//   5a. If cost decreased → accept step, decrease λ, check convergence
//   5b. If cost increased → reject step, increase λ, try again
//
LMResult LMSolver::solve(const SensorArray&       array,
                          const std::vector<Vec3>& meas,
                          const DipoleSource&      init) const
{
    LMResult result;
    result.estimated = init;   // start from the initial guess

    std::vector<double> r, J;
    computeResiduals(init, array, meas, r);   // initial residuals
    double currCost = cost(r);                // initial cost

    double lambda = opts_.lambda0;            // initial damping factor

    for (int iter = 0; iter < opts_.maxIter; ++iter) {

        // compute Jacobian at current estimate (analytical — exact, no FD steps)
        computeJacobianAnalytical(result.estimated, array, J);
        // solve the damped normal equations for step Δx
        std::array<double, NP> dx{};
        if (!solveNormalEquations(J, r, lambda, r.size(), dx)) {
            result.message = "singular normal equations";
            break;
        }

        // check gradient convergence: if ||J'r|| is tiny, we are at a minimum
	if (iter>0)
        {
            double gNorm = 0;
            for (int j = 0; j < NP; ++j) {
                double g = 0;
                for (std::size_t i = 0; i < r.size(); ++i)
                    g += J[i*NP+j] * r[i];   // g = J'r
                gNorm += g * g;
            }
            if (std::sqrt(gNorm) < opts_.gtol) {
                result.converged = true;
                result.message   = "gradient tolerance reached";
                break;
            }
        }

        // try the candidate step: x_new = x + Δx
        double params[NP], candidateParams[NP];
        result.estimated.toParams(params);
        for (int j = 0; j < NP; ++j) candidateParams[j] = params[j] + dx[j];

        DipoleSource candidate = DipoleSource::fromParams(candidateParams);
        std::vector<double> rNew;
        computeResiduals(candidate, array, meas, rNew);
        double newCost = cost(rNew);

        // verbose output: print progress each iteration
        if (opts_.verbose) {
            double dxNorm = 0;
            for (auto v : dx) dxNorm += v*v;
            std::cout << "[LM iter " << std::setw(3) << iter << "]"
                      << "  cost=" << std::scientific << std::setprecision(3)
                      << currCost
                      << "  lambda=" << lambda
                      << "  ||dx||=" << std::sqrt(dxNorm) << "\n";
        }

        if (newCost < currCost) {
            // ── Accept the step ──────────────────────────────────────────
            result.costHistory.push_back(newCost);
            std::array<double,6> p{};
            candidate.toParams(p.data());
            result.paramHistory.push_back(p);
            double relDelta = std::fabs(currCost - newCost) / (currCost + 1e-30);

            double dxNorm = 0;
            for (auto v : dx) dxNorm += v*v;
            dxNorm = std::sqrt(dxNorm);
            result.estimated = candidate;   // update estimate
            r        = rNew;                // update residuals
            currCost = newCost;             // update cost
            lambda  /= opts_.lambdaFactor;  // decrease damping (more aggressive)

            // check cost convergence
            if (relDelta < opts_.ftol) {
                result.converged = true;
                result.message   = "cost tolerance reached";
                break;
            }

            // check step size convergence
            if (dxNorm < opts_.xtol) {
                result.converged = true;
                result.message   = "step tolerance reached";
                break;
            }

        } else {
            // ── Reject the step ──────────────────────────────────────────
            lambda *= opts_.lambdaFactor;   // increase damping (more cautious)

            if (lambda > 1e16) {
                result.message = "lambda overflow — solver stuck";
                break;
            }
        }

        result.iterations = iter + 1;
    }

    result.finalCost = currCost;
    if (result.message.empty())
        result.message = result.converged ? "converged" : "max iterations reached";

    return result;
}
