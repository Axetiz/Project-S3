// Apsilume - Physics utilities (Newtonian gravity + time integrators)
//
// This module contains the core numerical methods used by the simulator:
//  - computation of pairwise Newtonian gravitational acceleration in 3D
//  - a simple symplectic Euler integrator (good energy behaviour, low cost)
//  - coupled Runge–Kutta 4 (RK4) integration for interacting bodies
//
// Notes:
//  - Values use a consistent unit system: SI in astronomical scenes, G=1 units in Figure Eight.
//  - The system RK4 step advances every body at each intermediate stage.
//  - The single-body helper below intentionally represents a fixed external field.
#pragma once

#include "body.hpp"
#include <vector>

namespace orbitsimlite {

struct Physics {
    // Universal gravitational constant in SI units (m^3 / (kg * s^2)).
    // A slightly rounded value is sufficient for visualisation.
    static constexpr double DefaultG = 6.674e-11;

    // Skip pairs at this squared separation or below, including a body itself.
    // This is a singularity cutoff, not a physical collision radius.
    static constexpr double MinimumSeparationSquared = 1e-9;

    // Compute the total gravitational acceleration acting on 'target' due to
    // all bodies in 'others' using the Newtonian point-mass model:
    //
    //   a = G * sum_i m_i * (r_i - r_target) / |r_i - r_target|^3
    //
    // where r_i is the position of body i.
    //
    // Zero/near-zero separations are skipped, so others may include target itself.
    // This cutoff prevents singular division; it is not a smooth softening force.
    static Vec3 acceleration(const Body &target, const std::vector<Body> &others, double G = DefaultG);

    // Advance a single body by one explicit symplectic Euler step, given a
    // precomputed acceleration and a timestep 'dt' (in seconds).
    //
    // The integration scheme is:
    //   v_{n+1} = v_n + a_n * dt
    //   x_{n+1} = x_n + v_{n+1} * dt
    //
    // Symplectic Euler is numerically cheap and exhibits better energy
    // behaviour than plain (non-symplectic) Euler for orbital problems.
    static void step_euler(Body &body, const Vec3 &acc, double dt);

    // One-step Runge–Kutta 4th order (RK4) integrator for a single body in the
    // gravitational field induced by 'others'. All bodies in 'others' are kept
    // fixed while evaluating intermediate stages (k1..k4). Pass external sources only;
    // including the original target would introduce a false source at its old position.
    //
    // Use this helper for a prescribed external field. Moving mutually interacting
    // bodies must use step_rk4_system() so every source advances at each RK4 stage.
    static void step_rk4(Body &body, const std::vector<Body> &others, double G, double dt);

    // Advance the complete interacting system. Each RK4 stage evaluates gravity
    // using all bodies' stage positions, rather than frozen source positions.
    // Final accelerations correspond to the final joint state.
    static void step_rk4_system(std::vector<Body> &bodies, double G, double dt);
};

} // namespace orbitsimlite
