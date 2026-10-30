// Apsilume: Forecast on a private simulator copy; bound work and cache results so long previews remain
// interactive. Module relationships and invariants: docs/architecture.md.
#include "trajectory_prediction.hpp"
#include <algorithm>
#include <cmath>
namespace orbitsimlite {
namespace {
constexpr double pi = 3.14159265358979323846;
// Numerical work, drawn sample count and refresh cadence are separate limits.
constexpr int maximum_preview_steps = 4096;
constexpr int target_preview_samples = 700;
constexpr double fallback_horizon_steps = 600;
constexpr double maximum_horizon_steps = 100000;
constexpr auto preview_work_budget = std::chrono::milliseconds(4);
constexpr auto refresh_interval = std::chrono::milliseconds(750);
bool same_state(const std::vector<Body> &a, const std::vector<Body> &b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].mass != b[i].mass || (a[i].pos - b[i].pos).length_squared() != 0 ||
            (a[i].vel - b[i].vel).length_squared() != 0)
            return false;
    }
    return true;
}
double forecast_horizon(const Simulator &sim, std::size_t selected, double range, double span) {
    const auto &bodies = sim.get_bodies();
    const Body &body = bodies[selected];
    // Travel a view-sized distance for free flight. For orbital systems use an
    // osculating Kepler period around the strongest gravitational source.
    double horizon =
        std::max(sim.get_dt() * fallback_horizon_steps, span / std::max(body.vel.length(), 1e-30));
    double strongest = 0;
    std::size_t primary = selected;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        const double r2 = (bodies[i].pos - body.pos).length_squared();
        if (i == selected || r2 <= Physics::MinimumSeparationSquared)
            continue;
        const double attraction = sim.get_gravity() * bodies[i].mass / r2;
        if (attraction > strongest) {
            strongest = attraction;
            primary = i;
        }
    }
    if (primary != selected) {
        const double r = Vec3::distance(body.pos, bodies[primary].pos);
        const double mu = sim.get_gravity() * (body.mass + bodies[primary].mass);
        const double energy = 0.5 * (body.vel - bodies[primary].vel).length_squared() - mu / r;
        const double axis = energy < 0 ? -mu / (2 * energy) : r;
        const double period = 2 * pi * std::sqrt(axis * axis * axis / mu);
        if (std::isfinite(period) && period > 0)
            horizon = period;
    }
    if (!std::isfinite(horizon) || horizon <= 0)
        horizon = sim.get_dt() * fallback_horizon_steps;
    return std::clamp(horizon * range, sim.get_dt(), sim.get_dt() * maximum_horizon_steps);
}
} // namespace
void TrajectoryPrediction::start(const Simulator &sim, int selected, double range, double span) {
    selected_ = selected;
    range_ = range;
    base_dt_ = sim.get_dt();
    gravity_ = sim.get_gravity();
    method_ = sim.get_integrator();
    substeps_ = sim.get_substeps();
    initial_bodies_ = sim.get_bodies();
    preview_ = sim;
    horizon_ = forecast_horizon(sim, std::size_t(selected), range, span);
    // Long horizons have a bounded numerical workload. Retain solver/substeps;
    // increase only the preview timestep when more than 4096 steps are needed.
    steps_ = std::clamp(int(std::ceil(std::min(double(maximum_preview_steps), horizon_ / base_dt_))), 1,
                        maximum_preview_steps);
    preview_->set_dt(horizon_ / steps_);
    sample_stride_ = std::max(1, (steps_ + target_preview_samples - 1) / target_preview_samples);
    step_ = 0;
    elapsed_ = 0;
    completed_ = false;
    points_.assign(1, sim.get_bodies()[std::size_t(selected)].pos);
}
void TrajectoryPrediction::update(const Simulator &source, int selected, double range, double span,
                                  bool paused) {
    if (selected < 0 || std::size_t(selected) >= source.get_bodies().size() || source.get_dt() <= 0) {
        invalidate();
        return;
    }
    // A paused edit invalidates immediately. During playback, finish the current preview before refreshing.
    const bool configuration_changed =
        !preview_ || selected != selected_ || range != range_ || base_dt_ != source.get_dt() ||
        gravity_ != source.get_gravity() || method_ != source.get_integrator() ||
        substeps_ != source.get_substeps() || initial_bodies_.size() != source.get_bodies().size();
    const bool edited_while_paused = paused && preview_ && !same_state(initial_bodies_, source.get_bodies());
    const bool refresh =
        completed_ && !paused && std::chrono::steady_clock::now() - completed_at_ > refresh_interval;
    if (configuration_changed || edited_while_paused || refresh)
        start(source, selected, range, span);
    if (completed_)
        return;
    const auto deadline = std::chrono::steady_clock::now() + preview_work_budget;
    do {
        preview_->step();
        ++step_;
        const Vec3 point = preview_->get_bodies()[std::size_t(selected_)].pos;
        if (!std::isfinite(point.length_squared())) {
            completed_ = true;
            break;
        }
        if (step_ % sample_stride_ == 0 || step_ == steps_)
            points_.push_back(point);
        completed_ = step_ >= steps_;
    } while (!completed_ && std::chrono::steady_clock::now() < deadline);
    if (completed_)
        completed_at_ = std::chrono::steady_clock::now();
}
} // namespace orbitsimlite
