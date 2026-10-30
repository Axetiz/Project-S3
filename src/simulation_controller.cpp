// Apsilume: Schedule fixed dt steps from real time and handle simplified collision decisions after completed
// steps. Module relationships and invariants: docs/architecture.md.
#include "simulation_controller.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
namespace orbitsimlite {
namespace {
constexpr double initial_steps_per_second = 60;
constexpr int maximum_steps_per_frame = 512;
constexpr auto simulation_work_budget = std::chrono::milliseconds(8);
} // namespace
void SimulationController::configure(const SceneDocument &document) {
    base_rate = document.simulation.get_dt() * initial_steps_per_second;
    speed = 1;
    paused = false;
    accumulated_ = 0;
    prompt.reset();
    catching_up = false;
}
void SimulationController::set_paused(bool value) {
    paused = value;
    accumulated_ = 0;
}
int SimulationController::advance(SceneDocument &document, double real_seconds,
                                  const std::function<void()> &after_step) {
    if (paused || prompt) {
        accumulated_ = 0;
        return 0;
    }
    const double dt = document.simulation.get_dt();
    if (!std::isfinite(real_seconds) || real_seconds < 0 || dt <= 0)
        return 0;
    // Convert wall time to simulation-time units once. Remaining work survives the per-frame budget.
    accumulated_ += real_seconds * base_rate * speed;
    const auto deadline = std::chrono::steady_clock::now() + simulation_work_budget;
    int steps = 0;
    // The tiny relative tolerance prevents a rounding remainder from losing a whole step.
    while (accumulated_ + dt * 1e-10 >= dt && steps < maximum_steps_per_frame) {
        document.simulation.step();
        resolve_collisions(document);
        accumulated_ = std::max(0.0, accumulated_ - dt);
        ++steps;
        if (after_step)
            after_step();
        if (prompt || std::chrono::steady_clock::now() >= deadline)
            break;
    }
    catching_up = accumulated_ >= dt;
    return steps;
}
void SimulationController::single_step(SceneDocument &document) {
    if (prompt)
        return;
    document.simulation.step();
    resolve_collisions(document);
}
void SimulationController::resolve_collisions(SceneDocument &document) {
    if (collisions == CollisionMode::Ignore || prompt)
        return;
    auto &bodies = document.simulation.access_bodies();
    for (std::size_t i = 0; i < bodies.size(); ++i)
        for (std::size_t j = i + 1; j < bodies.size(); ++j) {
            if (bodies[i].is_satellite || bodies[j].is_satellite)
                continue;
            // Radii are reference display sizes; convert to world length without using camera zoom.
            const double radius = (bodies[i].radius + bodies[j].radius) / document.scale;
            if ((bodies[i].pos - bodies[j].pos).length_squared() > radius * radius)
                continue;
            const std::size_t keep = bodies[i].mass >= bodies[j].mass ? i : j, remove = keep == i ? j : i;
            if (collisions == CollisionMode::PromptRemoveSmaller) {
                prompt = CollisionPrompt{bodies[keep].id, bodies[remove].id};
                return;
            }
            if (collisions == CollisionMode::Merge) {
                Body &merged = bodies[keep];
                const Body other = bodies[remove];
                // Mass-weighted position/velocity conserve center of mass and linear momentum.
                const double mass = merged.mass + other.mass;
                merged.pos = (merged.pos * merged.mass + other.pos * other.mass) / mass;
                merged.vel = (merged.vel * merged.mass + other.vel * other.mass) / mass;
                merged.mass = mass;
                merged.radius = std::cbrt(std::pow(merged.radius, 3) + std::pow(other.radius, 3));
                merged.is_star = merged.is_star || other.is_star;
                merged.is_satellite = merged.is_satellite && other.is_satellite;
                merged.name += "+" + other.name;
                if (document.selection && *document.selection == other.id)
                    document.selection = merged.id;
            }
            // Erasure invalidates indices/references. Resolve at most one pair per call.
            bodies.erase(bodies.begin() + std::ptrdiff_t(remove));
            document.mark_edited();
            return;
        }
}
void SimulationController::confirm_collision(SceneDocument &document) {
    if (!prompt)
        return;
    const auto index = document.simulation.index_of(prompt->remove);
    if (index) {
        auto &bodies = document.simulation.access_bodies();
        bodies.erase(bodies.begin() + std::ptrdiff_t(*index));
        document.mark_edited();
    }
    prompt.reset();
    accumulated_ = 0;
}
void SimulationController::cancel_collision() {
    prompt.reset();
    accumulated_ = 0;
}
} // namespace orbitsimlite
