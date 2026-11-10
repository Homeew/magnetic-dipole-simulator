// Unit tests using Catch2 (downloaded automatically by CMake)
// Run with:  cd build && ctest --output-on-failure

#define CATCH_CONFIG_MAIN   // Catch2 provides its own main() function
#include <catch2/catch.hpp> // Catch2 header (fetched by CMake)

#include "dipole.hpp"
#include "sensor_array.hpp"
#include "noise.hpp"
#include "lm_solver.hpp"

// ── Vec3 tests ────────────────────────────────────────────────────────────

TEST_CASE("Vec3 arithmetic", "[vec3]") {

    Vec3 a{1.0, 2.0, 3.0};
    Vec3 b{4.0, 5.0, 6.0};

    SECTION("addition") {
        Vec3 c = a + b;
        REQUIRE(c.x == Approx(5.0));
        REQUIRE(c.y == Approx(7.0));
        REQUIRE(c.z == Approx(9.0));
    }

    SECTION("subtraction") {
        Vec3 c = b - a;
        REQUIRE(c.x == Approx(3.0));
        REQUIRE(c.y == Approx(3.0));
        REQUIRE(c.z == Approx(3.0));
    }

    SECTION("scalar multiply") {
        Vec3 c = a * 2.0;
        REQUIRE(c.x == Approx(2.0));
        REQUIRE(c.y == Approx(4.0));
        REQUIRE(c.z == Approx(6.0));
    }

    SECTION("dot product") {
        // 1*4 + 2*5 + 3*6 = 4+10+18 = 32
        REQUIRE(a.dot(b) == Approx(32.0));
    }

    SECTION("norm") {
        Vec3 v{3.0, 4.0, 0.0};
        REQUIRE(v.norm() == Approx(5.0));   // 3-4-5 triangle
    }

    SECTION("unit vector has length 1") {
        Vec3 u = a.unit();
        REQUIRE(u.norm() == Approx(1.0));
    }
}

// ── DipoleSource field tests ──────────────────────────────────────────────

TEST_CASE("DipoleSource field", "[dipole]") {

    // dipole at origin, moment pointing in z
    DipoleSource src({0,0,0}, {0,0,1.0});

    SECTION("field is zero at singularity") {
        Vec3 b = src.fieldAt({0,0,0});
        REQUIRE(b.norm() == Approx(0.0));
    }

    SECTION("field falls off with distance") {
        // B ∝ 1/r³ — doubling distance reduces field by factor 8
        Vec3 b1 = src.fieldAt({1.0, 0.0, 0.0});
        Vec3 b2 = src.fieldAt({2.0, 0.0, 0.0});
        double ratio = b1.norm() / b2.norm();
        REQUIRE(ratio == Approx(8.0).epsilon(1e-6));
    }

    SECTION("field is symmetric around z-axis for z-aligned moment") {
        // rotate 90° in XY plane — field magnitude should be the same
        Vec3 b1 = src.fieldAt({ 1.0,  0.0, 0.0});
        Vec3 b2 = src.fieldAt({ 0.0,  1.0, 0.0});
        REQUIRE(b1.norm() == Approx(b2.norm()).epsilon(1e-10));
    }

    SECTION("fromParams / toParams round-trip") {
        double p[6] = {1,2,3,4,5,6};
        DipoleSource d = DipoleSource::fromParams(p);
        double q[6];
        d.toParams(q);
        for (int i = 0; i < 6; ++i)
            REQUIRE(p[i] == Approx(q[i]));
    }
}

// ── SensorArray geometry tests ────────────────────────────────────────────

TEST_CASE("SensorArray ring geometry", "[sensor_array]") {

    auto ring = SensorArray::ring(8, 1.0, 0.0);   // 8 sensors, radius 1m

    SECTION("correct number of sensors") {
        REQUIRE(ring.size() == 8);
    }

    SECTION("all sensors at correct radius") {
        for (const auto& s : ring.sensors) {
            double r = std::sqrt(s.position.x*s.position.x +
                                 s.position.y*s.position.y);
            REQUIRE(r == Approx(1.0).epsilon(1e-10));
        }
    }

    SECTION("all sensors at correct height") {
        for (const auto& s : ring.sensors)
            REQUIRE(s.position.z == Approx(0.0));
    }
}

TEST_CASE("SensorArray grid geometry", "[sensor_array]") {

    auto grid = SensorArray::grid(4, 4, 0.5, 0.5, 0.0);

    SECTION("correct number of sensors") {
        REQUIRE(grid.size() == 16);
    }

    SECTION("sensors within bounds") {
        for (const auto& s : grid.sensors) {
            REQUIRE(s.position.x >= Approx(-0.5).epsilon(1e-10));
            REQUIRE(s.position.x <= Approx( 0.5).epsilon(1e-10));
            REQUIRE(s.position.y >= Approx(-0.5).epsilon(1e-10));
            REQUIRE(s.position.y <= Approx( 0.5).epsilon(1e-10));
        }
    }
}

// ── GaussianNoise tests ───────────────────────────────────────────────────

TEST_CASE("GaussianNoise", "[noise]") {

    SECTION("same seed gives same sequence") {
        GaussianNoise n1(1e-9, 42);
        GaussianNoise n2(1e-9, 42);
        for (int i = 0; i < 20; ++i)
            REQUIRE(n1.sample() == Approx(n2.sample()));
    }

    SECTION("corrupt shifts values by noise amount") {
        GaussianNoise noise(0.0, 42);   // zero noise
        Vec3 v{1.0, 2.0, 3.0};
        Vec3 c = noise.corrupt(v);
        // with sigma=0 the output should equal the input exactly
        REQUIRE(c.x == Approx(v.x));
        REQUIRE(c.y == Approx(v.y));
        REQUIRE(c.z == Approx(v.z));
    }
}

// ── LM solver recovery test ───────────────────────────────────────────────

TEST_CASE("LMSolver recovers dipole from noiseless data", "[lm_solver]") {

    // known truth
    DipoleSource truth({0.05, -0.03, 0.02}, {0.0, 0.0, 1.5});

    // noiseless ring of 16 sensors
    auto array = SensorArray::ring(16, 0.08, 0.10);
    auto meas  = array.measure(truth);   // noiseless

    // start from a reasonable initial guess
    DipoleSource init({0.0, 0.0, 0.0}, {0.0, 0.0, 1.0});

    LMSolver::Options opts;
    opts.verbose = false;
    LMSolver solver(opts);

    LMResult result = solver.solve(array, meas, init);

    REQUIRE(result.converged == true);

    // position recovered to within 0.1 mm
    double posErr = (result.estimated.position - truth.position).norm();
    REQUIRE(posErr < 1e-4);

    // moment recovered to within 0.001 A·m²
    double momErr = (result.estimated.moment - truth.moment).norm();
    REQUIRE(momErr < 1e-3);
}
