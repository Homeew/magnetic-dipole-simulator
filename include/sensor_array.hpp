#pragma once
#include "sensor.hpp"    // Sensor class
#include "noise.hpp"     // GaussianNoise for noisy measurements
#include <vector>        // std::vector to hold the sensors
#include <cstddef>       // std::size_t

/**
 * SensorArray holds a collection of Sensor objects and provides
 * two ways to collect measurements from all of them at once:
 *   - measure()      : noiseless (ideal) readings
 *   - measureNoisy() : readings corrupted by Gaussian noise
 *
 * Three static factory methods build common array geometries:
 *   - ring()   : sensors on a circle in the XY plane
 *   - grid()   : sensors on a rectangular grid
 *   - sphere() : sensors distributed over a sphere
 */
class SensorArray {
public:
    std::vector<Sensor> sensors;   // all sensors in this array

    // default constructor — empty array
    SensorArray() = default;

    // construct from an existing vector of sensors
    explicit SensorArray(std::vector<Sensor> s) : sensors(std::move(s)) {}

    // return how many sensors are in this array
    std::size_t size() const { return sensors.size(); }

    /**
     * Collect noiseless B readings from every sensor.
     * Returns one Vec3 per sensor in the same order as sensors[].
     */
    std::vector<Vec3> measure(const DipoleSource& src) const {
        std::vector<Vec3> out;
        out.reserve(sensors.size());              // pre-allocate memory
        for (const auto& s : sensors)
            out.push_back(s.measure(src));        // ask each sensor to measure
        return out;
    }

    /**
     * Collect noisy B readings from every sensor.
     * Each component of each reading is corrupted independently by noise.
     */
    std::vector<Vec3> measureNoisy(const DipoleSource& src,
                                   GaussianNoise& noise) const {
        std::vector<Vec3> out;
        out.reserve(sensors.size());
        for (const auto& s : sensors) {
            Vec3 b = s.measure(src);         // get ideal field first
            out.push_back(noise.corrupt(b)); // then add noise
        }
        return out;
    }

    // ── Factory methods — defined in src/sensor_array.cpp ────────────────

    // Ring: n sensors evenly spaced on a circle of given radius
    // in the XY plane at height z, centred at (cx, cy)
    static SensorArray ring(int n, double radius,
                            double z  = 0.0,
                            double cx = 0.0,
                            double cy = 0.0);

    // Grid: rows×cols sensors on a rectangular grid in the XY plane at height z
    // sensors span [-halfW, halfW] in x and [-halfH, halfH] in y
    static SensorArray grid(int rows, int cols,
                            double halfW, double halfH,
                            double z = 0.0);

    // Sphere: n sensors distributed over a sphere of given radius
    // using the Fibonacci lattice for near-uniform coverage
    static SensorArray sphere(int n, double radius);
};
