// Double precision spatial vectors; two-coordinate construction remains planar.
#pragma once
#include "vec2.hpp"
#include <cmath>
namespace orbitsimlite {
struct Vec3 {
    double x{0}, y{0}, z{0};
    constexpr Vec3() = default;
    constexpr Vec3(double x_, double y_, double z_ = 0) : x(x_), y(y_), z(z_) {}
    Vec3(const Vec2 &v) : x(v.x), y(v.y), z(0) {}
    Vec3 operator+(const Vec3 &v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vec3 operator-(const Vec3 &v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }
    Vec3 &operator+=(const Vec3 &v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    Vec3 &operator-=(const Vec3 &v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }
    Vec3 &operator*=(double s) {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    Vec3 &operator/=(double s) {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }
    double length_squared() const { return dot(*this, *this); }
    double length() const { return std::sqrt(length_squared()); }
    Vec3 normalized() const {
        double n = length();
        return n > 0 ? *this / n : Vec3{};
    }
    static double dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    static Vec3 cross(const Vec3 &a, const Vec3 &b) {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }
    static double distance(const Vec3 &a, const Vec3 &b) { return (a - b).length(); }
};
inline Vec3 operator*(double s, const Vec3 &v) {
    return v * s;
}
} // namespace orbitsimlite
