#include "sensor_array.hpp"  // the class we are implementing
#include <cmath>             // std::cos, std::sin, std::sqrt, M_PI
#include <sstream>           // std::ostringstream for building labels
#include <iomanip>           // std::setw, std::setfill for label formatting

// ── Ring geometry ─────────────────────────────────────────────────────────
//
// Places n sensors evenly around a circle of given radius in the XY plane.
// Angle between consecutive sensors = 360° / n
//
SensorArray SensorArray::ring(int n, double radius,
                               double z, double cx, double cy)
{
    std::vector<Sensor> sensors;
    sensors.reserve(n);                        // pre-allocate for n sensors

    const double step = 2.0 * M_PI / n;       // angular spacing in radians

    for (int i = 0; i < n; ++i) {
        double angle = i * step;               // angle of this sensor [rad]

        // convert polar (radius, angle) to Cartesian (x, y)
        Vec3 pos{ cx + radius * std::cos(angle),
                  cy + radius * std::sin(angle),
                  z };

        // build label like "R00", "R01", ... "R15"
        std::ostringstream lbl;
        lbl << "R" << std::setw(2) << std::setfill('0') << i;

        sensors.emplace_back(pos, lbl.str());
    }
    return SensorArray(std::move(sensors));
}

// ── Grid geometry ─────────────────────────────────────────────────────────
//
// Places rows×cols sensors on a uniform rectangular grid in the XY plane.
// X spans [-halfW, +halfW], Y spans [-halfH, +halfH].
//
SensorArray SensorArray::grid(int rows, int cols,
                               double halfW, double halfH,
                               double z)
{
    std::vector<Sensor> sensors;
    sensors.reserve(rows * cols);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {

            // linearly interpolate x from -halfW to +halfW across rows
            double x = (rows > 1) ? -halfW + 2.0*halfW*r/(rows-1) : 0.0;

            // linearly interpolate y from -halfH to +halfH across cols
            double y = (cols > 1) ? -halfH + 2.0*halfH*c/(cols-1) : 0.0;

            // build label like "G0_0", "G0_1", "G2_3"
            std::ostringstream lbl;
            lbl << "G" << r << "_" << c;

            sensors.emplace_back(Vec3{x, y, z}, lbl.str());
        }
    }
    return SensorArray(std::move(sensors));
}

// ── Sphere geometry ───────────────────────────────────────────────────────
//
// Distributes n sensors over the surface of a sphere using the Fibonacci
// lattice — a simple algorithm that gives near-uniform coverage without
// solving a complex optimization problem.
//
// Key idea: step through latitudes using the golden ratio to avoid
// clustering sensors at the poles (a problem with simple latitude/longitude grids).
//
SensorArray SensorArray::sphere(int n, double radius)
{
    std::vector<Sensor> sensors;
    sensors.reserve(n);

    // golden ratio — irrational, so successive angles never repeat exactly
    const double golden = (1.0 + std::sqrt(5.0)) / 2.0;

    for (int i = 0; i < n; ++i) {

        // theta: polar angle from north pole, evenly spaced in cos(theta)
        // ensures equal area coverage per sensor
        double theta = std::acos(1.0 - 2.0*(i+0.5)/n);

        // phi: azimuthal angle, stepped by golden ratio to spread sensors
        double phi = 2.0 * M_PI * i / golden;

        // convert spherical (r, theta, phi) to Cartesian (x, y, z)
        Vec3 pos{ radius * std::sin(theta) * std::cos(phi),
                  radius * std::sin(theta) * std::sin(phi),
                  radius * std::cos(theta) };

        std::ostringstream lbl;
        lbl << "SP" << i;

        sensors.emplace_back(pos, lbl.str());
    }
    return SensorArray(std::move(sensors));
}
