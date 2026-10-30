// Apsilume - compatibility umbrella header
//
// Including this header pulls in the public C++ interface of the library:
//  - version information
//  - 3D vector math (Vec3)
//  - Body, Physics, Simulator (core physics)
//  - SFML renderer/application and utility helpers
//
// Use <orbitsimlite/core.hpp> for clients that do not link the graphical target.
//
// Typical usage:
//   #include <orbitsimlite/orbitsimlite.hpp>
//   orbitsimlite::Simulator sim(...);
//
#pragma once

#include <orbitsimlite/version.hpp>

#include "body.hpp"
#include "camera3d.hpp"
#include "physics.hpp"
#include "renderer.hpp"
#include "simulator.hpp"
#include "utils.hpp"
#include "vec2.hpp"
#include "vec3.hpp"
