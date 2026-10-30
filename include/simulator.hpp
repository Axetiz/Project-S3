// Apsilume - Simulator
//
// The Simulator owns the collection of bodies and is responsible for:
//  - storing global simulation parameters (G, dt, integrator, substeps)
//  - advancing the system in time via Physics integrators
//  - exposing read/write access to the current bodies
//
// It deliberately stays agnostic of any rendering or input concerns.
// Interactive/file validation belongs to EditorController/SceneSerializer; direct
// numerical clients are responsible for providing meaningful finite parameters.
#pragma once

#include "body.hpp"
#include "physics.hpp"
#include <optional>
#include <vector>

namespace orbitsimlite {

enum class Integrator { Euler, RK4 };

class Simulator {
  public:
    // Construct a simulator with given gravitational constant G (in SI units),
    // base timestep dt (seconds) and integrator type. The actual internal step
    // can be further refined using 'set_substeps'.
    explicit Simulator(double G = Physics::DefaultG, double dt = 1.0,
                       Integrator integrator = Integrator::RK4);

    // Body management -------------------------------------------------------

    // Append a copy, assigning a fresh ID if it is absent or already in use.
    void add_body(const Body &b);

    // Replace bodies while reserving explicit IDs before allocating missing ones.
    void set_bodies(const std::vector<Body> &bs);

    // Remove all bodies.
    void clear();

    std::optional<std::size_t> index_of(BodyId id) const;

    // Time step (seconds) ---------------------------------------------------

    void set_dt(double timestep);
    double get_dt() const;

    // Choose the integration scheme used in 'step()'.
    void set_integrator(Integrator i);
    Integrator get_integrator() const;

    // Gravitational constant (SI units) ------------------------------------

    void set_gravity(double gravity);
    double get_gravity() const;

    // Substepping -----------------------------------------------------------
    //
    // Each call to 'step()' advances the simulation by dt_. Internally this
    // can be subdivided into 'n' smaller steps of size dt_/n to improve
    // stability for tight orbits (e.g., Earth–Moon) without changing the
    // externally visible timestep.
    void set_substeps(int n);
    int get_substeps() const;

    // Simulation time -------------------------------------------------------

    // Return the accumulated simulation time in seconds since construction
    // or the last call to 'reset_time()'.
    double get_time() const;

    // Reset the accumulated simulation time to zero. Does not modify bodies.
    void reset_time();

    // Advance the whole system by one external step of size dt_. Depending
    // on the configured substeps, this may internally perform multiple
    // smaller integration steps.
    void step();

    // Access the current bodies. The non-const accessor is intended for
    // editing and collision controllers. Drawing uses the const view.
    // External users should prefer the const view where possible.
    const std::vector<Body> &get_bodies() const;
    std::vector<Body> &access_bodies();

  private:
    BodyId next_id_{1};
    double G_;
    double dt_;
    Integrator integrator_;
    std::vector<Body> bodies_;
    int substeps_;
    double time_{0.0};
};

} // namespace orbitsimlite
