// Apsilume - Body definition
//
// A Body represents a single point mass in 3D with rendering attributes.
// IDs connect editor selection, serialization and trail caches to the same body.
// Visibility is presentation state; it never changes the gravitational model.
#pragma once

#include "vec3.hpp"
#include <cstdint>
#include <string>

namespace orbitsimlite {

using BodyId = std::uint64_t;

struct Body {
    BodyId id{0};        // Stable identity; assigned by Simulator when absent.
    bool visible{true};  // Presentation visibility never disables gravitational interaction.
    double mass;         // Mass in kg for SI scenes; dimensionless in G=1 scenes.
    double radius;       // Display radius at the scene scale; also the collision proxy.
    Vec3 pos;            // World position (metres for SI scenes).
    Vec3 vel;            // World velocity (m/s for SI scenes).
    Vec3 acc;            // Acceleration (metres / second^2), updated per step.
    std::uint32_t color; // Packed RGB colour in 0xRRGGBB format.

    bool is_satellite; // True for natural satellites (e.g., Moon).
    bool is_star;      // True for stars (e.g., Sun); used by collision logic.

    // Display name, preserved verbatim by JSON serialization.
    std::string name;

    Body();
    Body(double mass_, const Vec3 &pos_, const Vec3 &vel_, double radius_, std::uint32_t color_,
         bool is_satellite_ = false, bool is_star_ = false, const std::string &name_ = {});
};

} // namespace orbitsimlite
