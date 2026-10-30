// Incremental spatial forecasts keep long-range prediction off the frame's hot path.
#pragma once
#include "simulator.hpp"
#include <chrono>
#include <optional>
namespace orbitsimlite {
class TrajectoryPrediction {
  public:
    void invalidate() {
        preview_.reset();
        points_.clear();
        horizon_ = 0;
        completed_ = false;
    }
    // range multiplies an estimated period/travel time; visible_span is a world-length scale.
    // paused also covers suspended editing/dialogs so changed initial conditions restart the cache.
    void update(const Simulator &source, int selected, double range, double visible_span, bool paused);
    const std::vector<Vec3> &points() const { return points_; }
    double horizon() const { return horizon_; }
    bool complete() const { return completed_; }

  private:
    void start(const Simulator &source, int selected, double range, double visible_span);
    std::optional<Simulator> preview_; // Private copy: forecasting must never advance live bodies.
    std::vector<Body> initial_bodies_; // Detect paused edits and remember the preview starting state.
    std::vector<Vec3> points_;
    int selected_{-1};
    double range_{1}, horizon_{0}, elapsed_{0}, base_dt_{0};
    double gravity_{0};
    // Configuration cache: a solver/selection/range change starts a fresh preview.
    Integrator method_{Integrator::RK4};
    int substeps_{1}, step_{0}, steps_{0}, sample_stride_{1};
    bool completed_{false};
    std::chrono::steady_clock::time_point completed_at_{};
};
} // namespace orbitsimlite
