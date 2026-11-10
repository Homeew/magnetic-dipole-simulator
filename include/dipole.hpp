#pragma once           // include guard — only compiled once
#include "vec3.hpp"    // we need Vec3 for positions and field vectors
#include <cmath>       // for M_PI

// Physical constants in SI units
namespace constants {
    // mu0 = permeability of free space = 4π × 10⁻⁷  [T·m/A]
    constexpr double mu0 = 4e-7 * M_PI;
}

/**
 * DipoleSource represents a magnetic dipole — the simplest model of
 * a localized magnetic source (e.g. a small permanent magnet or coil).
 *
 * The magnetic field it produces at any point r is:
 *
 *   B(r) = (mu0 / 4π) * [ 3(m·r̂)r̂ - m ] / |r|³
 *
 * where:
 *   r   = observation point - dipole position  (displacement vector)
 *   r̂   = r / |r|                              (unit vector)
 *   m   = magnetic dipole moment               [A·m²]
 *   |r| = distance from dipole to sensor       [m]
 */
class DipoleSource {
public:
    Vec3 position;   // where the dipole sits in space [m]
    Vec3 moment;     // direction and strength of the dipole [A·m²]

    // default constructor
    DipoleSource() = default;

    // construct with known position and moment
    DipoleSource(Vec3 position, Vec3 moment)
        : position(position), moment(moment) {}

    /**
     * Compute the B-field vector at observation point obs [T].
     *
     * Returns zero if obs == position (singularity — sensor sitting
     * exactly on the dipole makes |r|=0 which would divide by zero).
     */
    Vec3 fieldAt(const Vec3& obs) const {

        Vec3 r = obs - position;   // displacement from dipole to sensor

        double R = r.norm();       // distance |r|

        if (R < 1e-15) return {0, 0, 0};   // guard against division by zero

        Vec3   rhat = r / R;       // unit vector r̂ = r / |r|
        double R3   = R * R * R;   // |r|³

        // leading coefficient: mu0 / (4π * |r|³)
        double coeff = constants::mu0 / (4.0 * M_PI * R3);

        // full formula: coeff * [ 3(m·r̂)r̂ - m ]
        return (rhat * (3.0 * moment.dot(rhat)) - moment) * coeff;
    }

    /**
     * The LM solver works with a flat array of 6 numbers:
     *   [px, py, pz, mx, my, mz]
     * These two functions convert between DipoleSource and that array.
     */

    // build a DipoleSource from a 6-element parameter array
    static DipoleSource fromParams(const double* p) {
        return { {p[0], p[1], p[2]},   // position
                 {p[3], p[4], p[5]} };  // moment
    }

    // write this dipole's values into a 6-element parameter array
    void toParams(double* p) const {
        p[0] = position.x;   p[1] = position.y;   p[2] = position.z;
        p[3] = moment.x;     p[4] = moment.y;     p[5] = moment.z;
    }
};
