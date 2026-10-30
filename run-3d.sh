#!/bin/sh
# Configure Apsilume from this repository root.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$project_dir"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DORBITSIMLITE_BUILD_DEMO=ON -DORBITSIMLITE_BUILD_TESTS=ON
cmake --build build -j 2
exec ./build/orbit_simulator "$@"
