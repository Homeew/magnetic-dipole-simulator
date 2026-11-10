#pragma once
#include "vec3.hpp"      // Vec3 for position and field vectors
#include "dipole.hpp"    // DipoleSource so the sensor can query the field
#include <string>        // std::string for the sensor label

/**
 * Sensor models a single 3-axis magnetometer at a fixed location in space.
 *
 * A 3-axis magnetometer measures all three components of B simultaneously:
 *   measurement = (Bx, By, Bz)  [T]
 *
 * In this simulation each sensor is ideal — it sits at a known position
 * and returns the exact field value (before noise is added by GaussianNoise).
 */
class Sensor {
public:
    Vec3        position;   // where this sensor is located [m]
    std::string label;      // human-readable ID, e.g. "R00", "G2_3"

    // default constructor
    Sensor() = default;

    // construct with a position and optional label
    Sensor(Vec3 pos, std::string lbl = "")
        : position(pos), label(std::move(lbl)) {}

    /**
     * Return the ideal (noiseless) B-field reading at this sensor's position.
     * Noise is added later by GaussianNoise::corrupt() in SensorArray.
     */
    Vec3 measure(const DipoleSource& src) const {
        return src.fieldAt(position);   // just evaluate the dipole formula here
    }
};
