#pragma once
#include "vec3.hpp"      // Vec3 for the corrupt() return type
#include <random>        // std::mt19937_64, std::normal_distribution
#include <cstdint>       // uint64_t for the seed type

/**
 * GaussianNoise models additive white Gaussian noise (AWGN) on a 3-axis sensor.
 *
 * In real magnetometers, electronic noise causes each measurement component
 * (Bx, By, Bz) to be offset by a small random amount drawn from N(0, sigma²).
 *
 * Each component is corrupted independently — this is the standard noise model
 * for MEMS and fluxgate magnetometers.
 */
class GaussianNoise {
public:
    double sigma;   // standard deviation of the noise [T]
                    // e.g. 1e-9 T = 1 nT, typical for a good magnetometer

    // Constructor: set noise level and seed the random number generator
    // Using a fixed seed makes simulations reproducible
    explicit GaussianNoise(double sigma, uint64_t seed = 42)
        : sigma(sigma),
          rng_(seed),              // seed the Mersenne Twister RNG
          dist_(0.0, sigma) {}     // normal distribution: mean=0, std=sigma

    // Add independent Gaussian noise to each component of a field vector
    // Returns the corrupted measurement: v + noise
    Vec3 corrupt(const Vec3& v) {
        return { v.x + dist_(rng_),   // Bx + noise
                 v.y + dist_(rng_),   // By + noise
                 v.z + dist_(rng_) }; // Bz + noise
    }

    // Draw a single noise sample (useful for testing)
    double sample() { return dist_(rng_); }

    // Reseed the RNG — call this to get a fresh independent noise sequence
    void reseed(uint64_t seed) {
        rng_.seed(seed);
        dist_.reset();   // reset distribution internal state
    }

private:
    std::mt19937_64 rng_;                    // 64-bit Mersenne Twister RNG
    std::normal_distribution<double> dist_;  // Gaussian distribution N(0, sigma²)
};
