#include <iostream>
#include <iomanip>
#include <cmath>

#include "dipole.hpp"
#include "sensor_array.hpp"
#include "noise.hpp"
#include "lm_solver.hpp"

void printDipole(const std::string& tag, const DipoleSource& d) {
    std::cout << std::fixed << std::setprecision(5);
    std::cout << tag << "\n"
              << "  position : " << d.position << " m\n"
              << "  moment   : " << d.moment   << " A·m²\n";
}

double posError(const DipoleSource& a, const DipoleSource& b) {
    return (a.position - b.position).norm();
}

double momError(const DipoleSource& a, const DipoleSource& b) {
    return (a.moment - b.moment).norm();
}

int main(int argc, char* argv[]) {

    bool csvMode      = (argc > 1 && std::string(argv[1]) == "--csv");
    bool benchMode    = (argc > 1 && std::string(argv[1]) == "--benchmark");
    DipoleSource truth({ 0.05, -0.03, 0.02 }, { 0.0, 0.0, 1.5 });
    auto array = SensorArray::ring(16, 0.08, 0.10);

    const double sigmaNoise = 1e-9;
    GaussianNoise noise(sigmaNoise, 2024);
    auto measurements = array.measureNoisy(truth, noise);

    DipoleSource init({ 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 });

    LMSolver::Options opts;
    opts.verbose = !csvMode && !benchMode;
    LMSolver solver(opts);
    LMResult result = solver.solve(array, measurements, init);

    auto gridArray = SensorArray::grid(8, 8, 0.25, 0.25, 0.3);
    GaussianNoise noise2(sigmaNoise, 999);
    auto gridMeas = gridArray.measureNoisy(truth, noise2);
    LMSolver::Options opts2;
    opts2.verbose = false;
    LMSolver solver2(opts2);
    LMResult result2 = solver2.solve(gridArray, gridMeas, init);

    // ── Benchmark mode ────────────────────────────────────────────────────
    if (benchMode) {
        DipoleSource benchTruth({ 0.05, -0.03, 0.02 }, { 0.0, 0.0, 1.5 });
        DipoleSource benchInit({ 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 });
        LMSolver::Options bopts;
        bopts.verbose = false;
        LMSolver bsolver(bopts);

        // ── sweep 1: position error vs noise level ────────────────────────
        std::cout << "NOISE_SWEEP\n";
        std::vector<double> sigmas = {
            1e-12, 1e-11, 1e-10, 1e-9, 1e-8, 1e-7, 1e-6, 1e-5
        };
        auto barray = SensorArray::ring(16, 0.08, 0.10);
        for (double sigma : sigmas) {
            double totalErr = 0;
            for (int trial = 0; trial < 20; ++trial) {
                GaussianNoise n(sigma, trial * 100);
                auto m = barray.measureNoisy(benchTruth, n);
                auto r = bsolver.solve(barray, m, benchInit);
                totalErr += (r.estimated.position - benchTruth.position).norm();
            }
            double meanErr = totalErr / 20.0;
            std::cout << sigma << "," << meanErr << "\n";
        }

        // ── sweep 2: position error vs number of sensors ──────────────────
        std::cout << "SENSOR_SWEEP\n";
        std::vector<int> nSensors = { 4, 6, 8, 12, 16, 24, 32, 48 };
        const double benchSigma = 1e-9;
        for (int n : nSensors) {
            auto arr = SensorArray::ring(n, 0.08, 0.10);
            double totalErr = 0;
            for (int trial = 0; trial < 20; ++trial) {
                GaussianNoise noise(benchSigma, trial * 100);
                auto m = arr.measureNoisy(benchTruth, noise);
                auto r = bsolver.solve(arr, m, benchInit);
                totalErr += (r.estimated.position - benchTruth.position).norm();
            }
            std::cout << n << "," << totalErr / 20.0 << "\n";
        }

        return 0;
    }
    // ── CSV mode ──────────────────────────────────────────────────────────
    if (csvMode) {
        std::cout << "SENSORS\n";
        for (const auto& s : array.sensors)
            std::cout << s.position.x << ","
                      << s.position.y << ","
                      << s.position.z << "\n";

        std::cout << "FIELDS\n";
        for (const auto& s : array.sensors) {
            Vec3 b = truth.fieldAt(s.position);
            std::cout << b.x << "," << b.y << "," << b.z << "\n";
        }

        std::cout << "TRUE\n";
        std::cout << truth.position.x << ","
                  << truth.position.y << ","
                  << truth.position.z << "\n";
        std::cout << truth.moment.x << ","
                  << truth.moment.y << ","
                  << truth.moment.z << "\n";
        std::cout << "COSTS\n";
        for (double c : result.costHistory)
            std::cout << c << "\n";
        std::cout << "HISTORY\n";
        for (const auto& p : result.paramHistory)
            std::cout << p[0] << "," << p[1] << "," << p[2] << ","
                      << p[3] << "," << p[4] << "," << p[5] << "\n";
        std::cout << "ESTIMATED\n";
        std::cout << result.estimated.position.x << ","
                  << result.estimated.position.y << ","
                  << result.estimated.position.z << "\n";
        std::cout << result.estimated.moment.x << ","
                  << result.estimated.moment.y << ","
                  << result.estimated.moment.z << "\n";

        return 0;
    }

    // ── Normal mode ───────────────────────────────────────────────────────
    std::cout << "=================================================\n"
              << "   Magnetic Dipole Field Simulator\n"
              << "=================================================\n\n";
    printDipole("True dipole:", truth);
    std::cout << "\nSensor array: " << array.size()
              << " sensors on a ring (r=0.08m, z=0.10m)\n";
    double bMag = truth.fieldAt(array.sensors[0].position).norm();
    std::cout << std::scientific << std::setprecision(2);
    std::cout << "\nTypical |B| at sensor : " << bMag     << " T\n"
              << "Noise sigma           : " << sigmaNoise << " T\n"
              << "SNR                   : " << std::fixed
              << 20.0 * std::log10(bMag / sigmaNoise)     << " dB\n";
    printDipole("\nInitial guess:", init);
    std::cout << "\n--- Levenberg-Marquardt optimisation (ring array) ---\n";
    std::cout << "\n--- Results ---\n"
              << "Converged  : " << (result.converged ? "YES" : "NO") << "\n"
              << "Iterations : " << result.iterations << "\n"
              << "Final cost : " << std::scientific << result.finalCost << "\n"
              << "Message    : " << result.message << "\n\n";
    printDipole("Estimated dipole:", result.estimated);
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "\nPosition error : "
              << posError(truth, result.estimated) * 1e3 << " mm\n"
              << "Moment error   : "
              << momError(truth, result.estimated) << " A·m²\n";
    std::cout << "\n=================================================\n"
              << "   Grid array comparison (8x8, 0.5m x 0.5m)\n"
              << "=================================================\n";
    std::cout << "\nSensor array: " << gridArray.size()
              << " sensors on a grid (0.5x0.5m, z=0.3m)\n";
    printDipole("\nEstimated dipole:", result2.estimated);
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "\nPosition error : "
              << posError(truth, result2.estimated) * 1e3 << " mm\n"
              << "Moment error   : "
              << momError(truth, result2.estimated) << " A·m²\n";

    return 0;
}
