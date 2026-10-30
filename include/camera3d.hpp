// Projection, picking and navigation share one camera model, independent of SFML.
#pragma once
#include "vec3.hpp"
#include <algorithm>
#include <optional>
namespace orbitsimlite {
// direction must be unit length for hit_sphere() distances to be correct.
struct Ray3 {
    Vec3 origin, direction;
};
struct Camera3D {
    Vec3 target{};
    double distance{1000};
    double yaw{-1.5707963267948966}, pitch{0.65};
    bool orthographic{false};
    static constexpr double tangent = 0.414213562373095; // vertical FOV = 45 degrees
    Vec3 back() const {
        return {std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch)};
    }
    Vec3 right() const { return {-std::sin(yaw), std::cos(yaw), 0}; }
    Vec3 up() const { return Vec3::cross(back(), right()); }
    Vec3 eye() const { return target + back() * distance; }
    // Camera X/Y follow right/up. Positive Z is forward; the OpenGL renderer flips that Z sign.
    Vec3 view(const Vec3 &p) const {
        const Vec3 d = p - eye();
        return {Vec3::dot(d, right()), Vec3::dot(d, up()), -Vec3::dot(d, back())};
    }
    // Return viewport pixels (top-left origin) plus world-space forward depth, or no result if clipped.
    // width excludes the sidebar; rendering and picking must use this same viewport.
    std::optional<Vec3> project(const Vec3 &p, double width, double height) const {
        const Vec3 v = view(p);
        if (width <= 0 || height <= 0 || v.z <= distance * 0.001 || v.z >= distance * 10000)
            return {};
        const double span = (orthographic ? distance : v.z) * tangent;
        return Vec3{width * 0.5 + v.x * height / (2 * span), height * 0.5 - v.y * height / (2 * span), v.z};
    }
    // Inverse of project(): perspective rays share an eye, orthographic rays share a direction.
    Ray3 ray(double x, double y, double width, double height) const {
        const double sx = (2 * x - width) / height * tangent;
        const double sy = (height - 2 * y) / height * tangent;
        if (orthographic)
            return {eye() + (right() * sx + up() * sy) * distance, -back()};
        return {eye(), (-back() + right() * sx + up() * sy).normalized()};
    }
    // The plane equation is dot(normal, point) = offset; edge-on and behind-eye intersections fail.
    std::optional<Vec3> intersect_plane(const Ray3 &ray, const Vec3 &normal, double offset) const {
        const double d = Vec3::dot(ray.direction, normal);
        if (std::abs(d) < 1e-9)
            return {};
        const double t = (offset - Vec3::dot(ray.origin, normal)) / d;
        if (t < 0)
            return {};
        return ray.origin + ray.direction * t;
    }
    // Return the nearest nonnegative surface distance, including an exit hit if the eye is inside.
    std::optional<double> hit_sphere(const Ray3 &ray, const Vec3 &center, double radius) const {
        const Vec3 delta = ray.origin - center;
        const double b = Vec3::dot(delta, ray.direction);
        const double c = delta.length_squared() - radius * radius;
        const double discriminant = b * b - c;
        if (discriminant < 0)
            return {};
        const double first = -b - std::sqrt(discriminant), last = -b + std::sqrt(discriminant);
        if (last < 0)
            return {};
        return first >= 0 ? first : last;
    }
    void orbit(double dx, double dy) {
        yaw -= dx * 0.007;
        pitch = std::clamp(pitch + dy * 0.007, -1.57069632679, 1.57069632679);
    }
    void pan(double dx, double dy, double height) {
        target += (-right() * dx + up() * dy) * (2 * distance * tangent / std::max(1.0, height));
    }
    // Preserve the fitted scene when a window changes shape, including the
    // initial resize applied by a tiling desktop after window construction.
    void resize_viewport(double old_width, double old_height, double new_width, double new_height) {
        const double old_aspect = std::max(1.0, old_width) / std::max(1.0, old_height);
        const double new_aspect = std::max(1.0, new_width) / std::max(1.0, new_height);
        distance *= std::min(1.0, old_aspect) / std::min(1.0, new_aspect);
    }
    void dolly(double steps) { distance = std::clamp(distance * std::pow(0.85, steps), 1e-12, 1e30); }
};
} // namespace orbitsimlite
