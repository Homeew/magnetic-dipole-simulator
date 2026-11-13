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

    bool csvMode = (argc > 1 && std::string(argv[1]) == "--csv");

    DipoleSource truth({ 0.05, -0.03, 0.02 }, { 0.0, 0.0, 1.5 });
    auto array = SensorArray::ring(16, 0.08, 0.10);

    const double sigmaNoise = 1e-9;
    GaussianNoise noise(sigmaNoise, 2024);
    auto measurements = array.measureNoisy(truth, noise);

    DipoleSource init({ 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 });

    LMSolver::Options opts;
    opts.verbose = !csvMode;
    LMSolver solver(opts);
    LMResult result = solver.solve(array, measurements, init);

    auto gridArray = SensorArray::grid(8, 8, 0.25, 0.25, 0.3);
    GaussianNoise noise2(sigmaNoise, 999);
    auto gridMeas = gridArray.measureNoisy(truth, noise2);
    LMSolver::Options opts2;
    opts2.verbose = false;
    LMSolver solver2(opts2);
    LMResult result2 = solver2.solve(gridArray, gridMeas, init);

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
