// Apsilume - version information
//
// This header exposes the library version both as integer components and
// as a human-readable string. It is safe to include from C or C++.
#pragma once

#define ORBITSIMLITE_VERSION_MAJOR 3
#define ORBITSIMLITE_VERSION_MINOR 0
#define ORBITSIMLITE_VERSION_PATCH 0

#define ORBITSIMLITE_VERSION_STRING "3.0.0-experimental"

namespace orbitsimlite {

inline constexpr int VersionMajor = ORBITSIMLITE_VERSION_MAJOR;
inline constexpr int VersionMinor = ORBITSIMLITE_VERSION_MINOR;
inline constexpr int VersionPatch = ORBITSIMLITE_VERSION_PATCH;

inline constexpr const char *VersionString = ORBITSIMLITE_VERSION_STRING;

} // namespace orbitsimlite
