// Apsilume tests: Headless 3D contracts: rotation covariance, camera rays, forecasts and legacy scene
// compatibility. Suite ownership and feature-change checks: docs/architecture.md. Spatial behavior tests:
// geometry, physics covariance, camera picking and scene compatibility.
#include "camera3d.hpp"
#include "scene_serializer.hpp"
#include "trajectory_prediction.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace orbitsimlite;
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
bool close(const Vec3 &a, const Vec3 &b, double tolerance = 1e-10) {
    return (a - b).length() < tolerance;
}
Vec3 rotate(const Vec3 &v) {
    // A rigid rotation into a plane with all three nonzero world components.
    const double c = std::cos(0.67), s = std::sin(0.67);
    const Vec3 x{v.x, c * v.y - s * v.z, s * v.y + c * v.z};
    return {c * x.x - s * x.y, s * x.x + c * x.y, x.z};
}
void vector_and_gravity() {
    require(close(Vec3::cross({1, 0, 0}, {0, 1, 0}), {0, 0, 1}), "cross product orientation");
    require(std::abs(Vec3{2, 3, 6}.length() - 7) < 1e-12, "spatial length");
    Body target(1, {0, 0, 2}, {}, 1, 0), source(4, {0, 0, 0}, {}, 1, 0);
    require(close(Physics::acceleration(target, {source}, 1), {0, 0, -1}), "gravity must act along Z");
    source.pos = {1, 2, 3};
    target.pos = {-1, 0, 1};
    const Vec3 a = Physics::acceleration(target, {source}, 1);
    require(a.x > 0 && a.y > 0 && a.z > 0, "all acceleration components");
}
void simulation_covariance() {
    for (auto method : {Integrator::Euler, Integrator::RK4}) {
        Simulator planar(1, 0.001, method), spatial(1, 0.001, method);
        const std::vector<Body> initial = {Body(2, {-1, 0}, {0, -0.4}, 1, 0), Body(3, {1, 0}, {0, 0.3}, 1, 0),
                                           Body(0.1, {0, 2}, {-0.5, 0}, 1, 0)};
        planar.set_bodies(initial);
        for (auto b : initial) {
            b.pos = rotate(b.pos);
            b.vel = rotate(b.vel);
            spatial.add_body(b);
        }
        planar.set_substeps(3);
        spatial.set_substeps(3);
        for (int step = 0; step < 600; ++step) {
            planar.step();
            spatial.step();
        }
        for (std::size_t i = 0; i < initial.size(); ++i) {
            require(close(rotate(planar.get_bodies()[i].pos), spatial.get_bodies()[i].pos, 1e-9),
                    "rotation must preserve simulated position");
            require(close(rotate(planar.get_bodies()[i].vel), spatial.get_bodies()[i].vel, 1e-9),
                    "rotation must preserve simulated velocity");
            require(planar.get_bodies()[i].pos.z == 0 && planar.get_bodies()[i].vel.z == 0,
                    "planar initial conditions must remain planar");
        }
        require(std::abs(spatial.get_time() - 0.6) < 1e-12, "spatial timestep");
    }
}
void inclined_orbit() {
    Body sun(1, {}, {}, 1, 0), planet(1e-6, rotate({1, 0, 0}), rotate({0, 1, 0}), 1, 0);
    const Vec3 initial = planet.pos;
    constexpr double pi = 3.14159265358979323846;
    for (int i = 0; i < 4000; ++i)
        Physics::step_rk4(planet, {sun}, 1, 2 * pi / 4000);
    require(close(planet.pos, initial, 1e-8), "inclined orbit must close in fixed central field");
    require(std::abs(planet.vel.z) > 0.1, "inclined velocity must retain its Z component");
}
void camera_geometry() {
    Camera3D camera;
    camera.distance = 20;
    camera.target = {3, 4, 5};
    camera.yaw = -0.9;
    for (bool orthographic : {false, true}) {
        camera.orthographic = orthographic;
        for (int plane = 0; plane < 3; ++plane) {
            const Vec3 normal = plane == 0 ? Vec3{0, 0, 1} : (plane == 1 ? Vec3{0, 1, 0} : Vec3{1, 0, 0});
            Vec3 p = camera.target + Vec3{0.7, -0.4, 0.6};
            const auto screen = camera.project(p, 960, 840);
            require(screen.has_value(), "point in front of camera");
            const auto ray = camera.ray(screen->x, screen->y, 960, 840);
            const auto hit = camera.intersect_plane(ray, normal, Vec3::dot(normal, p));
            require(hit.has_value() && close(*hit, p), "projection and edit-plane unprojection agree");
        }
        const Ray3 ray = camera.ray(480, 420, 960, 840);
        const auto near_hit = camera.hit_sphere(ray, camera.target, 1);
        const auto far_hit = camera.hit_sphere(ray, camera.target - camera.back() * 5, 1);
        require(near_hit && far_hit && *near_hit < *far_hit, "pick front sphere when silhouettes overlap");
        require(!camera.hit_sphere(ray, camera.eye() + camera.back() * 5, 1), "do not pick behind camera");
        require(!camera.intersect_plane({{0, 0, 1}, {1, 0, 0}}, {0, 0, 1}, 0),
                "edge-on creation plane is unavailable");
        require(!camera.project(camera.eye() + camera.back(), 960, 840), "clip points behind camera");
    }
    camera.orbit(100, 200);
    camera.pan(20, -15, 840);
    camera.dolly(2);
    const auto center = camera.project(camera.target, 960, 840);
    require(center && std::abs(center->x - 480) < 1e-9 && std::abs(center->y - 420) < 1e-9,
            "navigation preserves camera target projection");
}
void viewport_resize() {
    Camera3D camera;
    camera.pitch = 1.5707963267948966;
    camera.distance = 4;
    const Vec3 edge{1.1, 0, 0};
    const auto before = camera.project(edge, 680, 800);
    require(before && before->x > 0 && before->x < 680, "body must initially fit the viewport");
    camera.resize_viewport(680, 800, 592, 968);
    const auto after = camera.project(edge, 592, 968);
    require(after && after->x > 0 && after->x < 592, "narrow tiling resize must not hide the third body");
    require(std::abs(before->x / 680 - after->x / 592) < 1e-10,
            "resize should preserve horizontal scene framing");
    camera.resize_viewport(592, 968, 680, 800);
    require(std::abs(camera.distance - 4) < 1e-10,
            "resizing back must restore the original viewing distance");
}
void long_range_prediction() {
    Simulator sim(1, 0.001, Integrator::RK4);
    sim.set_substeps(4);
    sim.add_body(Body(1, {}, {}, 1, 0));
    sim.add_body(Body(1e-6, {1, 0, 0}, {0, std::cos(0.6), std::sin(0.6)}, 1, 0));
    TrajectoryPrediction forecast;
    const auto finish = [&](const Simulator &source, int selected, double range) {
        for (int frame = 0; frame < 10000; ++frame) {
            forecast.update(source, selected, range, 20, true);
            if (forecast.complete())
                return;
        }
        throw std::runtime_error("forecast did not finish within bounded work");
    };
    finish(sim, 1, 1.5);
    require(forecast.horizon() > 9 && forecast.horizon() < 10, "forecast should cover 1.5 orbital periods");
    require(forecast.points().size() > 100, "long prediction should have enough spatial samples");
    double excursion = 0, z_extent = 0;
    for (const auto &p : forecast.points()) {
        excursion = std::max(excursion, Vec3::distance(p, sim.get_bodies()[1].pos));
        z_extent = std::max(z_extent, std::abs(p.z));
    }
    require(excursion > 1.9 && z_extent > 0.5,
            "prediction must show a long inclined orbit, not a short tangent");
    require(sim.get_time() == 0 && close(sim.get_bodies()[1].pos, {1, 0, 0}),
            "prediction must not advance live simulation");
    const double short_horizon = forecast.horizon();
    finish(sim, 1, 3);
    require(std::abs(forecast.horizon() - 2 * short_horizon) < 1e-10, "range control must extend horizon");
    sim.access_bodies()[1].pos.z = 0.4;
    finish(sim, 1, 3);
    require(forecast.points().front().z == 0.4, "paused body edits must invalidate cached prediction");
    Simulator free(0, 0.01, Integrator::Euler);
    free.add_body(Body(1, {}, {0, 0, 2}, 1, 0));
    finish(free, 0, 1.5);
    require(std::abs(forecast.points().back().z - 30) < 1e-8,
            "free flight should forecast a view-sized spatial distance");
    forecast.invalidate();
    require(forecast.points().empty(), "disabling prediction should clear its cached geometry");
}
void scene_roundtrip() {
    ScenarioPreset original;
    original.name = "Spatial \"quoted\" \n Հայերեն";
    original.gravity = 1;
    original.dt = 0.001;
    original.meters_to_pixels = 250;
    original.bodies.emplace_back(3.141592653589793, Vec3{1.123456789012345, 2.5, -3.75},
                                 Vec3{-0.25, 0.5, 1.23456789012345}, 4, 0x123456, false, true, "Spatial");
    original.integrator = Integrator::Euler;
    std::string json;
    require(bool(SceneSerializer::encode(original, json)), "save spatial scene");
    ScenarioPreset scene;
    require(bool(SceneSerializer::decode(json, scene)), "load spatial scene");
    require(scene.name == original.name && scene.integrator == Integrator::Euler,
            "Unicode name and method roundtrip");
    require(close(scene.bodies[0].pos, original.bodies[0].pos, 1e-15) &&
                close(scene.bodies[0].vel, original.bodies[0].vel, 1e-15),
            "XYZ scene precision roundtrip");
    const std::string legacy =
        R"({"name":"Old","gravity":1,"dt":1,"substeps":1,"meters_to_pixels":1,"integrator":"RK4","bodies":[{"name":"2D","mass":1,"radius":1,"pos_x":2,"pos_y":3,"vel_x":4,"vel_y":5,"color":123}]})";
    require(bool(SceneSerializer::decode(legacy, scene)), "load old 2D scene");
    require(scene.bodies[0].pos.z == 0 && scene.bodies[0].vel.z == 0, "missing Z defaults to zero");
    require(!SceneSerializer::decode(R"({"gravity":1e999})", scene), "invalid numeric input must not crash");
}
} // namespace
int main() {
    int passed = 0, failed = 0;
    const auto run = [&](const char *name, auto function) {
        try {
            function();
            ++passed;
            std::cout << "[3D] " << name << ": OK\n";
        } catch (const std::exception &error) {
            ++failed;
            std::cerr << "[3D] " << name << ": " << error.what() << '\n';
        }
    };
    run("vectors and XYZ gravity", vector_and_gravity);
    run("Euler/RK4 spatial covariance and planar compatibility", simulation_covariance);
    run("inclined orbit", inclined_orbit);
    run("projection, selection and navigation", camera_geometry);
    run("camera framing across desktop resize", viewport_resize);
    run("long-range spatial forecasts and cache invalidation", long_range_prediction);
    run("3D scene precision and old scene loading", scene_roundtrip);
    std::cout << passed << " spatial groups passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
