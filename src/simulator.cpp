// Apsilume: Own bodies/IDs and advance one outer dt by subdividing it into synchronized numerical substeps.
// Module relationships and invariants: docs/architecture.md.
// Apsilume - Simulator implementation
#include "simulator.hpp"
#include <algorithm>

namespace orbitsimlite {

Simulator::Simulator(double G, double dt, Integrator integrator)
    : G_(G), dt_(dt), integrator_(integrator), substeps_(1), time_(0.0) {}

void Simulator::add_body(const Body &b) {
    Body copy = b;
    if (copy.id == 0 || index_of(copy.id))
        copy.id = next_id_++;
    next_id_ = std::max(next_id_, copy.id + 1);
    bodies_.push_back(copy);
}

void Simulator::set_bodies(const std::vector<Body> &bs) {
    // The argument may alias get_bodies(); copy it before clearing the owned vector.
    const auto copy = bs;
    bodies_.clear();
    // Reserve explicit IDs before assigning missing ones, regardless of body order.
    for (const auto &body : copy)
        next_id_ = std::max(next_id_, body.id + 1);
    for (const auto &body : copy)
        add_body(body);
}
std::optional<std::size_t> Simulator::index_of(BodyId id) const {
    for (std::size_t i = 0; i < bodies_.size(); ++i)
        if (bodies_[i].id == id)
            return i;
    return {};
}

void Simulator::clear() {
    bodies_.clear();
}

void Simulator::set_dt(double timestep) {
    dt_ = timestep;
}
double Simulator::get_dt() const {
    return dt_;
}

void Simulator::set_integrator(Integrator i) {
    integrator_ = i;
}
Integrator Simulator::get_integrator() const {
    return integrator_;
}

void Simulator::set_gravity(double gravity) {
    G_ = gravity;
}
double Simulator::get_gravity() const {
    return G_;
}

void Simulator::step() {
    // Simulation time still advances in an empty workspace.
    if (bodies_.empty()) {
        time_ += dt_;
        return;
    }

    // Substeps improve stability without forcing the caller to change the
    // public timestep; the simulator simply slices one dt into smaller pieces.
    const int n = (substeps_ > 0) ? substeps_ : 1;
    const double h = dt_ / static_cast<double>(n);

    for (int s = 0; s < n; ++s) {
        if (integrator_ == Integrator::Euler) {
            // Freeze all source positions while gathering acceleration, then move every body.
            // Updating one body before gathering the next would introduce order-dependent forces.
            std::vector<Vec3> accs(bodies_.size());
            for (std::size_t i = 0; i < bodies_.size(); ++i) {
                accs[i] = Physics::acceleration(bodies_[i], bodies_, G_);
            }
            for (std::size_t i = 0; i < bodies_.size(); ++i) {
                Physics::step_euler(bodies_[i], accs[i], h);
            }
        } else { // Coupled RK4: every source moves at every intermediate stage.
            Physics::step_rk4_system(bodies_, G_, h);
        }
    }

    // Advance simulation time by one full step
    time_ += dt_;
}

const std::vector<Body> &Simulator::get_bodies() const {
    return bodies_;
}
std::vector<Body> &Simulator::access_bodies() {
    return bodies_;
}

void Simulator::set_substeps(int n) {
    substeps_ = (n > 0) ? n : 1;
}
int Simulator::get_substeps() const {
    return substeps_;
}

double Simulator::get_time() const {
    return time_;
}
void Simulator::reset_time() {
    time_ = 0.0;
}

} // namespace orbitsimlite
