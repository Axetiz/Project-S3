# Apsilume user guide

Apsilume is the Project-S3 continuation of OrbitSimLite: interactive Newtonian N-body experiments in XYZ. Run the commands in this guide from the repository root. Existing executable and data-directory names remain compatible.

## Run

```sh
./run-3d.sh
```

The graphical chooser offers Solar System, Binary Stars, Figure Eight, Sandbox and Empty scenes. Choose live JSON/CSV outputs and click **Start scene**. No terminal input is needed. The original demo launchers remain available and preselect their corresponding scene.

Build requirements: C++17 compiler, CMake 3.16+, SFML (3 preferred, compatibility branches for 2.5+), OpenGL with a 2.1 compatibility context. JSON parsing and the DejaVu font are bundled, with their licenses; no network connection or system font lookup is needed at runtime.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
./build/orbit_simulator
```

## Editor workflow

Buttons and field labels use measured font bounds for centered alignment. Long labels fit their controls with Unicode-safe truncation, and active input fields scroll horizontally to keep the caret visible. Dialog buttons reflow into rows as needed; the desktop editor has a minimum window size of 760×600.

The toolbar has Pause/Play, Step, Reset, Fit, Predict, Save, Open, Scenes and Help. Hover a button for its meaning and shortcut. Change playback speed with the visible −/+ controls.

Select a sphere or a body-list entry. The inspector has separate labeled name, mass, display radius, position X/Y/Z and velocity X/Y/Z fields. Values accept scientific notation, for example `1.5e11`. Click a field, type a replacement, then Enter to apply. Escape cancels; Ctrl+A/C/V and caret navigation work. Invalid input stays in the field with an explanatory error. Simulation advancement is suspended while a field or dialog is active.

Drag a selected body's red X, green Y or blue Z handle to move along that world axis. For an axis viewed end-on, rotate the camera or use its numeric field. **Move handles** toggles the editing handles; **World origin axes** independently toggles the large arrows at (0,0,0). A small orientation indicator stays in the corner.

Body-list Hide/Show affects visualization only; hidden bodies continue exerting gravity. Add body uses the editing plane at the camera target. The N shortcut uses the pointer's intersection with that plane. Focus/follow centers on the selected body. Inspector, body list, simulation/output and camera/visualization sections can collapse; scroll over the sidebar to browse controls. The toolbar wraps when the window becomes narrower.

The simulation section exposes numerical timestep, substeps, gravity, integrator, collision policy, JSON and CSV. Camera controls expose top/front/side/3D views and perspective/orthographic projection. Forecast controls shorten or lengthen the selected body's horizon; the displayed duration is in simulation time units. The horizon is bounded to 100,000 numerical timesteps and the preview uses at most 4,096 steps, so repeated range increases can reach the cap. Forecasts are approximate, particularly for close encounters and chaotic systems.

| Shortcut | Action |
|---|---|
| Right drag / middle drag / wheel | Orbit / pan / zoom |
| Shift + right drag | Pan |
| Home; F1; F2/F3/F4; O | Fit; 3D angle; XY/XZ/YZ views; projection |
| Tab; F5 | Next body; focus/follow |
| Arrows; PageUp/PageDown | Move X/Y; move Z |
| A/D, W/S, U/J | Edit velocity X, Y, Z |
| Shift + movement/velocity key | Fine adjustment |
| Enter | Focus selected body's X field |
| Q/E; Z/X; T/Y | Mass; display radius; star/satellite role |
| N; Delete | Add at pointer; remove selected body |
| B; [ / ] | Editing plane; plane offset |
| Space; .; R | Play/pause; one fixed step; reset |
| −/+ | Playback speed, independently of numerical timestep |
| H; G; V; F | Grid; origin axes; body vectors; field mode |
| P; F6/F7 | Forecast; shorter/longer horizon |
| I; M; C | Integrator; collision policy; confirm smaller-body removal |
| K or Ctrl+S; L or Ctrl+O | Save; open |
| F8; Esc | Help; cancel field/dialog or exit |

## Time and physics

The solver uses double-precision XYZ Newtonian pairwise gravity, symplectic Euler or fully coupled RK4. Each RK4 stage advances all interacting bodies together. Original 2D conditions remain planar when Z position/velocity start at zero.

Numerical `dt` and substeps control integration accuracy. Playback speed controls how many fixed steps the clock requests per real second. The initial rate is 60 steps/second at x1. Changing numerical dt leaves the requested simulation-time rate unchanged; changing speed leaves dt unchanged. Rendering frame rate does not set the timestep. An accumulator retains elapsed simulation time; bounded batches keep input responsive during catch-up. Pause clears pending time, and Step advances exactly one dt.

Display radii are scene-scale educational values, not physical planetary radii. XYZ collision distances use radius / reference display scale, independent of camera zoom. Satellites are excluded from collision handling. Policies are prompt/remove smaller, automatic removal, merge, or ignore. Merge conserves mass and momentum and combines sphere volumes. Headless runs use Ignore.

The infinite reference/deformed field plane is reconstructed from camera rays, with adaptive spacing and horizon fading. Field arrows sample Newtonian acceleration on the chosen cross-section. The deformed visual grid uses up to the 64 largest masses to bound GPU cost; physics uses every body. These are visualization aids, not general relativistic spacetime curvature.

## Saved scenes and output

Data locations do not depend on the working directory:

| Platform | Default application data root |
|---|---|
| Linux | `$XDG_DATA_HOME/OrbitSimLite/Experimental` or `~/.local/share/OrbitSimLite/Experimental` |
| macOS | `~/Library/Application Support/OrbitSimLite/Experimental` |
| Windows | `%LOCALAPPDATA%/OrbitSimLite/Experimental` |

Reusable scenes live in `scenes/`; live snapshots in `exports/`. Override the root with `--data-dir PATH` or `ORBITSIMLITE_DATA_DIR`. The graphical Save/Open dialogs show the path. Legacy JSON files in nearby `Exports/` folders are copied into the scene library on startup, without replacing existing files or removing originals.

Scene format version 2 preserves double precision, Unicode/escaped names, stable body IDs, visibility, units, XYZ position/velocity and solver settings. Old flat `pos_x`/`vel_x` scenes remain readable; absent Z defaults to zero. Unsupported versions and invalid values produce visible errors. Loading is transactional. Saving writes a temporary file beside the destination, then atomically replaces it; a failed replacement keeps the old file. Atomic replacement prevents partial files during normal readers/writers; it does not promise persistence after power loss. Overwrites and deletion require an explicit click. Unsaved edits are protected before reset, switching scenes or exit. Undo/redo is outside this update.

JSON and CSV are current snapshots, updated at most 10 times per second during normal playback, plus final writes. They include body IDs and all XYZ coordinates, velocities and accelerations. Read CSV by column name; extra columns can be added in future versions. These files do not record a complete trajectory history.

## Headless and command-line use

```sh
./build/orbit_simulator --headless --scene 2 --steps 1000 --json --csv --data-dir /tmp/orbit-experiment
./build/orbit_simulator --headless --scene 0 --duration 10 --speed 2
./build/orbit_simulator --scene 1 --run
./build/orbit_simulator --help
```

`--steps` is deterministic. `--duration` uses wall-clock seconds; `INF` runs until interrupted. `--json`, `--no-json` and `--csv` set initial outputs. Build without graphics dependencies for servers:

```sh
cmake -S . -B build-headless -DORBITSIMLITE_BUILD_GUI=OFF
cmake --build build-headless -j 4
./build-headless/orbit_simulator --headless --scene 2 --steps 100
ctest --test-dir build-headless --output-on-failure
```

## Ownership and maintenance

The [developer guide](architecture.md) contains the detailed file composition, reading order and model invariants.

| Component | Responsibility |
|---|---|
| `Simulator`, `Physics`, `Vec3` | Numerical state, fixed steps and XYZ mathematics |
| `SceneDocument` | Scene metadata, stable selection identity, initial state and dirty tracking |
| `SimulationController` | Fixed-step accumulator, playback and collision decisions |
| `EditorController` | Validated property edits, adding/removing bodies and spatial manipulation |
| `Renderer3D`, `Camera3D` | GPU resources, spatial drawing, projection and picking geometry |
| `EditorUI` | Reusable fields/buttons, focus, layout, tooltips and dialogs |
| `ActionRegistry`, `Application` | Shared shortcut/button commands and service coordination |
| `SceneSerializer` | Versioned schema, validation, migration and atomic scene writes |
| `AppPaths`, `SnapshotExporter` | User directories and throttled output |
| `TrajectoryPrediction` | Incremental cached previews on a simulation copy |

`orbitsimlite_core` has no SFML/OpenGL dependency. `orbitsimlite` adds the GUI; `Renderer` remains a small source-compatibility facade for existing clients. The core umbrella is `<orbitsimlite/core.hpp>`; the graphics umbrella remains `<orbitsimlite/orbitsimlite.hpp>`. `.clang-format` supplies consistent formatting. Body IDs survive vector reordering, deletion of other bodies and scene roundtrips; array indices are transient.

The UI renders the bundled font using its glyph atlas directly, avoiding a reproduced `sf::Text` teardown deadlock on the current workstation. The JSON dependency is the vendored [nlohmann/json](https://json.nlohmann.me/) library under `third_party/`.

See the [validation record](validation.md) for the checks executed on this workstation.

## Validation and packaging

```sh
ctest --test-dir build --output-on-failure
python3 tests/gui_workflow.py build/orbit_simulator
cmake --install build --prefix /tmp/orbitsim-install
cpack --config build/CPackConfig.cmake -B build/packages
```

CTest covers physics, coupled N-body conservation/convergence, spatial camera/picking/forecasts, Unicode and legacy persistence, failed atomic writes, stable selection, property validation, collision identity, snapshot cadence and fixed-step playback at 30/60/144 FPS. The separate desktop workflow sends real X11 input; it requires libX11/libXtst and a desktop (niri/Xwayland is supported). It checks startup, fields, XYZ drag, shortcuts, dt/speed separation, prediction, saving/reopening and unsaved protection. It writes a screenshot to `/tmp/orbitsim-product-editor.png`.

For bounded visual smoke checks:

```sh
ORBITSIMLITE_FRAME_LIMIT=30 ORBITSIMLITE_SCREENSHOT=/tmp/orbitsim-editor.png ./build/orbit_simulator --run
```

Install/CPack includes executables, font and licenses. Runtime SFML/OpenGL libraries must still be installed on the target system; the archive is not a universally static executable. Linux GUI behavior is exercised on the current desktop; Windows/macOS path and replace-file branches require platform validation. The headless core CI runs without graphics libraries. See [the usability protocol](usability-test.md) for human evaluation tasks; automated checks do not substitute for those sessions.
