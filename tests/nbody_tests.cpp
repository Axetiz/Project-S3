// Apsilume tests: Coupled-system regressions: conservation, convergence order and final-state accelerations.
// Suite ownership and feature-change checks: docs/architecture.md.
// Regressions for interacting systems: the whole state must advance at every RK4 stage.
#include "simulator.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace orbitsimlite;
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
std::vector<Body> figure_eight() {
    return {Body(1, {.97000436, -.24308753}, {.466203685, .432365730}, 8, 0xFFCC78, false, false, "A"),
            Body(1, {-.97000436, .24308753}, {.466203685, .432365730}, 8, 0x78DCFF, false, false, "B"),
            Body(1, {0, 0}, {-.93240737, -.86473146}, 8, 0xC878FF, false, false, "C")};
}
double energy(const std::vector<Body> &bs) {
    double result = 0;
    for (std::size_t i = 0; i < bs.size(); ++i) {
        result += .5 * bs[i].mass * bs[i].vel.length_squared();
        for (std::size_t j = i + 1; j < bs.size(); ++j)
            result -= bs[i].mass * bs[j].mass / Vec3::distance(bs[i].pos, bs[j].pos);
    }
    return result;
}
void figure_eight_conservation() {
    Simulator sim(1, .01, Integrator::RK4);
    sim.set_bodies(figure_eight());
    const double e0 = energy(sim.get_bodies());
    double max_drift = 0;
    for (int step = 0; step < 1000; ++step) {
        sim.step();
        max_drift = std::max(max_drift, std::abs((energy(sim.get_bodies()) - e0) / e0));
    }
    Vec3 momentum{}, center{};
    for (const auto &b : sim.get_bodies()) {
        momentum += b.vel * b.mass;
        center += b.pos * b.mass;
    }
    std::cout << "Figure-eight max energy drift=" << max_drift << ", momentum=" << momentum.length() << '\n';
    require(max_drift < 1e-5, "figure-eight energy drifts excessively");
    require(momentum.length() < 1e-11, "isolated three-body momentum must remain conserved");
    require(center.length() < 1e-10, "three-body center of mass drifts");
    require(sim.get_bodies()[0].name == "A" && sim.get_bodies()[0].radius == 8,
            "integrator must preserve body metadata");
    for (const auto &b : sim.get_bodies())
        require(b.pos.z == 0 && b.vel.z == 0, "planar choreography must remain planar");
}
double binary_error(double dt) {
    Simulator sim(1, dt, Integrator::RK4);
    sim.add_body(Body(1, {-1, 0}, {0, -.5}, 1, 0));
    sim.add_body(Body(1, {1, 0}, {0, .5}, 1, 0));
    const int steps = int(std::round(2 / dt));
    for (int i = 0; i < steps; ++i)
        sim.step();
    const Vec3 reference{std::cos(1.0), std::sin(1.0), 0};
    return Vec3::distance(sim.get_bodies()[1].pos, reference);
}
void coupled_fourth_order() {
    const double coarse = binary_error(.1), fine = binary_error(.05);
    std::cout << "Coupled binary errors=" << coarse << ", " << fine << ", ratio=" << coarse / fine << '\n';
    require(fine < 1e-7 && coarse / fine > 12 && coarse / fine < 20,
            "coupled RK4 should converge at fourth order");
}
void final_accelerations() {
    Simulator sim(1, .01, Integrator::RK4);
    sim.set_bodies(figure_eight());
    sim.step();
    for (const auto &b : sim.get_bodies()) {
        const Vec3 expected = Physics::acceleration(b, sim.get_bodies(), 1);
        require((b.acc - expected).length() < 1e-12, "published acceleration must match final joint state");
    }
}
} // namespace
int main() {
    int failures = 0;
    const auto run = [&](const char *label, auto fn) {
        try {
            fn();
            std::cout << "[N-body] " << label << ": OK\n";
        } catch (const std::exception &e) {
            ++failures;
            std::cerr << "[N-body] " << label << ": " << e.what() << '\n';
        }
    };
    run("figure-eight conservation", figure_eight_conservation);
    run("coupled RK4 convergence", coupled_fourth_order);
    run("final joint accelerations", final_accelerations);
    return failures ? 1 : 0;
}
