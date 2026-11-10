#pragma once          // include guard — ensures this file is only compiled once
#include <cmath>      // for std::sqrt
#include <ostream>    // for std::ostream (used in the << print operator)

struct Vec3 {
    double x{0}, y{0}, z{0};   // three components, default-initialized to zero

    Vec3() = default;                               // default constructor: (0,0,0)
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}  // constructor from three values

    // vector addition: (a+b) component-wise
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }

    // vector subtraction: used to compute r = observation - dipole_position
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }

    // scalar multiplication: scale a vector by a number (e.g. apply the mu0/4pi coefficient)
    Vec3 operator*(double s) const { return {x*s, y*s, z*s}; }

    // scalar division: used in unit() to normalize a vector
    Vec3 operator/(double s) const { return {x/s, y/s, z/s}; }

    // in-place addition: v += other
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }

    // dot product: a·b = ax*bx + ay*by + az*bz
    // used in the dipole formula as m·r̂
    double dot(const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }

    // cross product: gives a vector perpendicular to both inputs
    // used in some geometry calculations
    Vec3 cross(const Vec3& o) const {
        return { y*o.z - z*o.y,   // x-component
                 z*o.x - x*o.z,   // y-component
                 x*o.y - y*o.x }; // z-component
    }

    // Euclidean length: |v| = sqrt(x²+y²+z²)
    // used as |r| (distance from dipole to sensor)
    double norm() const { return std::sqrt(dot(*this)); }

    // squared length: avoids a sqrt when only the square is needed
    double norm2() const { return dot(*this); }

    // unit vector: same direction, length 1
    // used to compute r̂ = r / |r| in the dipole formula
    Vec3 unit() const { return *this / norm(); }

    // allows writing:  2.0 * v  (scalar on the left side)
    friend Vec3 operator*(double s, const Vec3& v) { return v*s; }

    // allows printing a Vec3 with std::cout << v
    friend std::ostream& operator<<(std::ostream& os, const Vec3& v) {
        return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    }
};
