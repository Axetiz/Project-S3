// Apsilume: Build reproducible initial conditions. Solar/binary scenes use SI; Figure Eight uses
// dimensionless G=1 units. Module relationships and invariants: docs/architecture.md.
#include "scene_catalog.hpp"
#include "utils.hpp"
#include <cmath>
namespace orbitsimlite {
namespace {
// Legacy builder multipliers adjust the base numerical dt; playback speed belongs to the controller.
ScenarioPreset make_solar_system_preset(double multiplier) {
    ScenarioPreset preset;
    preset.name = "Solar System";
    preset.gravity = Physics::DefaultG;
    preset.dt = 36000.0 * multiplier;
    preset.integrator = Integrator::RK4;
    preset.substeps = 12;
    preset.meters_to_pixels = 2e-9;

    constexpr double pi = 3.14159265358979323846;
    Vec3 earth_pos;
    Vec3 earth_vel;

    preset.bodies.emplace_back(1.989e30, Vec3{0.0, 0.0}, Vec3{0.0, 0.0}, 30.0, rgb_u32(255, 255, 0), false,
                               true, "Sun");

    {
        const double R = 5.79e10;
        const double v = 47360.0;
        const double theta = 0.0;
        preset.bodies.emplace_back(3.301e23, Vec3{R * std::cos(theta), R * std::sin(theta)},
                                   Vec3{-v * std::sin(theta), v * std::cos(theta)}, 6.0,
                                   rgb_u32(180, 180, 180), false, false, "Mercury");
    }

    {
        const double R = 1.082e11;
        const double v = 35020.0;
        const double theta = 2.0 * pi / 3.0;
        preset.bodies.emplace_back(4.867e24, Vec3{R * std::cos(theta), R * std::sin(theta)},
                                   Vec3{-v * std::sin(theta), v * std::cos(theta)}, 9.0,
                                   rgb_u32(255, 200, 120), false, false, "Venus");
    }

    {
        const double R = 2.0e11;
        const double v = 22000.0;
        const double theta = pi / 4.0;
        preset.bodies.emplace_back(5.0e24, Vec3{R * std::cos(theta), R * std::sin(theta)},
                                   Vec3{-v * std::sin(theta), v * std::cos(theta)}, 8.0, rgb_u32(0, 250, 180),
                                   false, false, "Blop");
    }

    {
        const double R = 1.496e11;
        const double v = 29783.0;
        const double theta = 4.0 * pi / 3.0;
        earth_pos = Vec3{R * std::cos(theta), R * std::sin(theta)};
        earth_vel = Vec3{-v * std::sin(theta), v * std::cos(theta)};
        preset.bodies.emplace_back(5.972e24, earth_pos, earth_vel, 10.0, rgb_u32(70, 120, 255), false, false,
                                   "Earth");
    }

    {
        const double moon_dist = 3.84e8;
        const double moon_speed = 1022.0;
        const double theta_earth = 4.0 * pi / 3.0;
        Vec3 t_hat(-std::sin(theta_earth), std::cos(theta_earth));
        Vec3 moon_pos = earth_pos + moon_dist * t_hat;
        Vec3 moon_vel = earth_vel + moon_speed * t_hat;
        preset.bodies.emplace_back(7.35e22, moon_pos, moon_vel, 3.0, rgb_u32(200, 200, 200), true, false,
                                   "Moon");
    }

    {
        const double R = 2.279e11;
        const double v = 24077.0;
        const double theta = pi / 2.0;
        preset.bodies.emplace_back(6.417e23, Vec3{R * std::cos(theta), R * std::sin(theta)},
                                   Vec3{-v * std::sin(theta), v * std::cos(theta)}, 7.0,
                                   rgb_u32(255, 100, 80), false, false, "Mars");
    }

    // Inclinations make the spatial extension visible immediately. Earth and
    // Moon share a rotation so their relative initial state is preserved.
    for (auto &body : preset.bodies) {
        double angle = body.name == "Moon" || body.name == "Earth"
                           ? 0.18
                           : (body.name == "Mercury" ? 0.40 : (body.name == "Venus" ? -0.25 : 0.08));
        const auto rotate = [angle](const Vec3 &v) {
            return Vec3{v.x, v.y * std::cos(angle) - v.z * std::sin(angle),
                        v.y * std::sin(angle) + v.z * std::cos(angle)};
        };
        body.pos = rotate(body.pos);
        body.vel = rotate(body.vel);
    }
    return preset;
}

ScenarioPreset make_binary_stars_preset(double multiplier) {
    ScenarioPreset preset;
    preset.name = "Binary Stars";
    preset.gravity = Physics::DefaultG;
    preset.dt = 3600.0 * multiplier;
    preset.integrator = Integrator::RK4;
    preset.substeps = 20;
    preset.meters_to_pixels = 2e-10;

    const double mass_sun = 1.989e30;
    const double dist = 3.0e11;
    const double v = std::sqrt(Physics::DefaultG * mass_sun / (4.0 * dist));

    preset.bodies.emplace_back(mass_sun, Vec3{-dist, 0.0}, Vec3{0.0, v}, 30.0, rgb_u32(255, 220, 120), false,
                               true, "SunA");
    preset.bodies.emplace_back(mass_sun, Vec3{dist, 0.0}, Vec3{0.0, -v}, 30.0, rgb_u32(255, 240, 180), false,
                               true, "SunB");
    return preset;
}

ScenarioPreset make_figure_eight_preset(double multiplier) {
    ScenarioPreset preset;
    preset.name = "Figure Eight";
    preset.gravity = 1.0;
    preset.dt = 0.001 * multiplier;
    preset.integrator = Integrator::RK4;
    preset.substeps = 4;
    preset.meters_to_pixels = 250.0;

    preset.bodies.emplace_back(1.0, Vec3{0.97000436, -0.24308753}, Vec3{0.4662036850, 0.4323657300}, 8.0,
                               rgb_u32(255, 200, 120), false, false, "BodyA");
    preset.bodies.emplace_back(1.0, Vec3{-0.97000436, 0.24308753}, Vec3{0.4662036850, 0.4323657300}, 8.0,
                               rgb_u32(120, 220, 255), false, false, "BodyB");
    preset.bodies.emplace_back(1.0, Vec3{0.0, 0.0}, Vec3{-0.93240737, -0.86473146}, 8.0,
                               rgb_u32(200, 120, 255), false, false, "BodyC");
    return preset;
}

ScenarioPreset make_sandbox_preset(double multiplier) {
    ScenarioPreset preset;
    preset.name = "Sandbox";
    preset.gravity = Physics::DefaultG;
    preset.dt = 7200.0 * multiplier;
    preset.integrator = Integrator::RK4;
    preset.substeps = 8;
    preset.meters_to_pixels = 3e-9;

    preset.bodies.emplace_back(1.5e30, Vec3{0.0, 0.0}, Vec3{0.0, 0.0}, 26.0, rgb_u32(255, 235, 150), false,
                               true, "Core");
    preset.bodies.emplace_back(7.0e24, Vec3{9.0e10, 0.0}, Vec3{0.0, 33500.0}, 9.0, rgb_u32(120, 190, 255),
                               false, false, "Alpha");
    preset.bodies.emplace_back(4.5e24, Vec3{-1.5e11, 0.0}, Vec3{0.0, -25000.0}, 8.0, rgb_u32(255, 140, 130),
                               false, false, "Beta");
    preset.bodies.emplace_back(2.0e23, Vec3{0.0, 2.3e11}, Vec3{-18000.0, 0.0}, 5.0, rgb_u32(180, 255, 170),
                               false, false, "Gamma");
    return preset;
}

} // namespace
std::vector<ScenarioPreset> make_builtin_scenes() {
    auto figure = make_figure_eight_preset(1);
    figure.units = "dimensionless";
    ScenarioPreset empty;
    empty.name = "Empty workspace";
    empty.dt = 3600;
    empty.substeps = 8;
    empty.meters_to_pixels = 3e-9;
    return {make_solar_system_preset(1), make_binary_stars_preset(1), figure, make_sandbox_preset(1), empty};
}
} // namespace orbitsimlite
