// Apsilume: Newtonian acceleration and numerical integrators. System RK4 advances every interacting source at
// every stage. Module relationships and invariants: docs/architecture.md. Apsilume - Physics implementation
//
// The routines in this file implement:
//  - Newtonian gravitational acceleration between point masses
//  - a symplectic Euler integrator
//  - coupled RK4 for N-body systems and a fixed-field single-body RK4 helper
//
// Values are doubles in one consistent unit system: SI or a dimensionless scene such as G=1.
#include "physics.hpp"

#include <algorithm>
#include <array>

namespace orbitsimlite {

Vec3 Physics::acceleration(const Body &target, const std::vector<Body> &others, double G) {
    Vec3 acc{0.0, 0.0};
    for (const auto &o : others) {
        // Skip exact same position to avoid singularity; also covers the case
        // where 'target' might accidentally be present in 'others'.
        Vec3 r = o.pos - target.pos;
        double dist2 = r.length_squared();
        if (dist2 <= MinimumSeparationSquared)
            continue;
        double invDist = 1.0 / std::sqrt(dist2);
        double invDist3 = invDist * invDist * invDist;
        // a = G * m / r^2 * r_hat = G * m * r / |r|^3
        acc += (G * o.mass) * (r * invDist3);
    }
    return acc;
}

void Physics::step_euler(Body &body, const Vec3 &acc, double dt) {
    body.acc = acc;
    // Symplectic Euler: update velocity using current acceleration, then
    // update position using the new velocity.
    body.vel += body.acc * dt;
    body.pos += body.vel * dt;
}

void Physics::step_rk4(Body &body, const std::vector<Body> &others, double G, double dt) {
    // Educational RK4 using 'others' at their current positions.
    //
    // We treat the velocity v and position x of a single body as the state
    // and integrate according to:
    //   x' = v
    //   v' = a(x),  where a(x) is computed from Newtonian gravity.
    //
    // For a concise derivation and stability discussion see:
    //   https://en.wikipedia.org/wiki/Runge%E2%80%93Kutta_methods
    auto acc_at = [&](const Vec3 &pos) {
        Body temp = body;
        temp.pos = pos;
        return Physics::acceleration(temp, others, G);
    };

    const Vec3 x0 = body.pos;
    const Vec3 v0 = body.vel;

    const Vec3 k1_v = acc_at(x0);
    const Vec3 k1_x = v0;

    const Vec3 k2_v = acc_at(x0 + 0.5 * dt * k1_x);
    const Vec3 k2_x = v0 + 0.5 * dt * k1_v;

    const Vec3 k3_v = acc_at(x0 + 0.5 * dt * k2_x);
    const Vec3 k3_x = v0 + 0.5 * dt * k2_v;

    const Vec3 k4_v = acc_at(x0 + dt * k3_x);
    const Vec3 k4_x = v0 + dt * k3_v;

    body.vel = v0 + (dt / 6.0) * (k1_v + 2.0 * k2_v + 2.0 * k3_v + k4_v);
    body.pos = x0 + (dt / 6.0) * (k1_x + 2.0 * k2_x + 2.0 * k3_x + k4_x);
    body.acc = acc_at(body.pos);
}

void Physics::step_rk4_system(std::vector<Body> &bodies, double G, double dt) {
    const std::size_t count = bodies.size();
    if (count == 0)
        return;
    // State is (position, velocity): dx/dt = velocity and dv/dt = joint gravitational acceleration.
    // Each array holds all bodies at one RK4 stage, avoiding frozen-source approximations.
    std::array<std::vector<Vec3>, 4> position_derivative, velocity_derivative;
    for (int stage = 0; stage < 4; ++stage) {
        position_derivative[stage].resize(count);
        velocity_derivative[stage].resize(count);
    }
    std::vector<Vec3> positions(count);
    // Evaluate each pair once. Both members receive the same interaction at
    // the same stage, preserving Newton's action/reaction symmetry.
    const auto accelerations = [&](std::vector<Vec3> &output) {
        std::fill(output.begin(), output.end(), Vec3{});
        for (std::size_t i = 0; i < count; ++i)
            for (std::size_t j = i + 1; j < count; ++j) {
                const Vec3 delta = positions[j] - positions[i];
                const double r2 = delta.length_squared();
                if (r2 <= MinimumSeparationSquared)
                    continue;
                const double inverse = 1 / std::sqrt(r2);
                const Vec3 interaction = delta * (G * inverse * inverse * inverse);
                output[i] += interaction * bodies[j].mass;
                output[j] -= interaction * bodies[i].mass;
            }
    };
    for (std::size_t i = 0; i < count; ++i) {
        positions[i] = bodies[i].pos;
        position_derivative[0][i] = bodies[i].vel;
    }
    accelerations(velocity_derivative[0]);
    // Stage offsets are dt/2, dt/2 and dt, all measured from the original state.
    for (int stage = 1; stage < 4; ++stage) {
        const double interval = stage == 3 ? dt : dt * 0.5;
        for (std::size_t i = 0; i < count; ++i) {
            positions[i] = bodies[i].pos + position_derivative[stage - 1][i] * interval;
            position_derivative[stage][i] = bodies[i].vel + velocity_derivative[stage - 1][i] * interval;
        }
        accelerations(velocity_derivative[stage]);
    }
    for (std::size_t i = 0; i < count; ++i) {
        bodies[i].pos += (position_derivative[0][i] + 2 * position_derivative[1][i] +
                          2 * position_derivative[2][i] + position_derivative[3][i]) *
                         (dt / 6);
        bodies[i].vel += (velocity_derivative[0][i] + 2 * velocity_derivative[1][i] +
                          2 * velocity_derivative[2][i] + velocity_derivative[3][i]) *
                         (dt / 6);
        positions[i] = bodies[i].pos;
    }
    // Store acceleration at the final joint state for display/export, not the fourth trial state.
    accelerations(velocity_derivative[0]);
    for (std::size_t i = 0; i < count; ++i)
        bodies[i].acc = velocity_derivative[0][i];
}

} // namespace orbitsimlite
