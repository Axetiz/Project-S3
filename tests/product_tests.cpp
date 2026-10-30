// Apsilume tests: Headless workflow contracts: fixed clocks, stable selection, validated edits, atomic files
// and exports. Suite ownership and feature-change checks: docs/architecture.md.
#include "editor_controller.hpp"
#include "scene_catalog.hpp"
#include "scene_serializer.hpp"
#include "snapshot_exporter.hpp"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
using namespace orbitsimlite;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
SceneDocument free_scene() {
    ScenarioPreset scene;
    scene.name = "Free flight";
    scene.units = "dimensionless";
    scene.gravity = 0;
    scene.dt = 0.01;
    scene.meters_to_pixels = 100;
    scene.bodies.emplace_back(1, Vec3{}, Vec3{1, 2, 3}, 1, 0xFFFFFF, false, false, "Free");
    SceneDocument document;
    document.apply(scene);
    return document;
}
std::string read(const std::filesystem::path &path) {
    std::ifstream file(path);
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}
void fixed_clock() {
    for (int fps : {30, 60, 144}) {
        auto doc = free_scene();
        SimulationController clock;
        clock.configure(doc);
        clock.collisions = CollisionMode::Ignore;
        for (int frame = 0; frame < fps * 2; ++frame)
            clock.advance(doc, 1.0 / fps);
        require(std::abs(doc.simulation.get_time() - 1.2) < 1e-12, "render FPS changed simulated time");
        require((doc.selected_body()->pos - Vec3{1.2, 2.4, 3.6}).length() < 1e-10,
                "render FPS changed spatial trajectory");
        clock.set_paused(true);
        clock.advance(doc, 50);
        clock.set_paused(false);
        clock.advance(doc, 1.0 / 60);
        require(std::abs(doc.simulation.get_time() - 1.21) < 1e-12, "paused time leaked into clock");
        clock.speed = 4;
        clock.advance(doc, 1.0 / 60);
        require(doc.simulation.get_dt() == 0.01 && std::abs(doc.simulation.get_time() - 1.25) < 1e-12,
                "speed changed dt instead of work rate");
        clock.set_paused(true);
        clock.single_step(doc);
        require(std::abs(doc.simulation.get_time() - 1.26) < 1e-12, "step should advance exactly dt");
        clock.advance(doc, std::numeric_limits<double>::quiet_NaN());
        require(std::isfinite(doc.simulation.get_time()), "nonfinite wall clock corrupted time");
    }
}
void empty_clock() {
    auto document = free_scene();
    document.simulation.clear();
    SimulationController clock;
    clock.configure(document);
    clock.single_step(document);
    require(document.simulation.get_time() == document.simulation.get_dt(),
            "empty scenes must still advance their clock");
}
void stable_selection_and_edits() {
    auto doc = free_scene();
    EditorController edit;
    const auto old = doc.selection;
    edit.add(doc, {2, 3, 4});
    const auto selected = doc.selection;
    require(selected != old && selected.has_value(), "new bodies need unique stable IDs");
    doc.simulation.access_bodies().erase(doc.simulation.access_bodies().begin());
    require(doc.selection == selected && doc.selected_index() == 0,
            "deleting another body changed selection identity");
    require(edit.set_property(doc, Property::PositionZ, "7.5e2").empty() && doc.selected_body()->pos.z == 750,
            "XYZ scientific notation edit failed");
    require(edit.set_property(doc, Property::VelocityZ, "-3.25").empty() &&
                doc.selected_body()->vel.z == -3.25,
            "velocity Z edit failed");
    const double mass = doc.selected_body()->mass;
    for (const auto *bad : {"nan", "inf", "-1", "0", "3kg", ""})
        require(!edit.set_property(doc, Property::Mass, bad).empty() && doc.selected_body()->mass == mass,
                "invalid edit changed mass");
    require(!edit.set_property(doc, Property::Substeps, "1.5").empty(), "fractional substeps accepted");
    require(!edit.set_property(doc, Property::Substeps, "4097").empty(), "unbounded substeps accepted");
    require(edit.set_property(doc, Property::Timestep, "0.002").empty() && doc.simulation.get_dt() == 0.002,
            "dt inspector edit failed");
    require(doc.dirty, "edits must mark document dirty");
    Body fresh(1, {}, {}, 1, 0xFFFFFF), existing = fresh;
    existing.id = 1;
    Simulator mixed;
    mixed.set_bodies({fresh, existing});
    require(mixed.get_bodies()[1].id == 1 && mixed.get_bodies()[0].id != 1,
            "missing IDs stole an existing identity");
    const auto before = doc.selected_body()->pos;
    edit.move(doc, {0, 0, 10});
    require(doc.selected_body()->pos.z == before.z + 10, "gizmo Z edit failed");
    edit.remove(doc);
    require(!doc.selected_body() && doc.simulation.get_bodies().empty(), "delete left a dangling selection");
}
void collision_selection() {
    auto doc = free_scene();
    auto &b = doc.simulation.access_bodies()[0];
    b.mass = 2;
    b.vel = {};
    doc.simulation.add_body(Body(1, {}, {}, 1, 0xFFFFFF));
    doc.select_index(1);
    SimulationController clock;
    clock.configure(doc);
    clock.collisions = CollisionMode::Merge;
    const auto keep = doc.simulation.get_bodies()[0].id;
    clock.single_step(doc);
    require(doc.simulation.get_bodies().size() == 1 && doc.selection == keep,
            "merge lost selection identity");
    require(doc.selected_body()->mass == 3, "merge did not conserve mass");
}
void persistence(const std::filesystem::path &folder) {
    auto doc = free_scene();
    doc.name = "Quoted \\\"scene\\\"\nՀայերեն";
    doc.selected_body()->name = "Body, \\\"1\\\"";
    doc.selected_body()->visible = false;
    const auto path = folder / "scene.json";
    require(bool(SceneSerializer::save(path, doc.snapshot())), "scene save failed");
    const auto original = read(path);
    ScenarioPreset loaded;
    require(bool(SceneSerializer::load(path, loaded)) && loaded.name == doc.name &&
                !loaded.bodies[0].visible && loaded.bodies[0].id == *doc.selection,
            "versioned scene roundtrip failed");
    const auto valid = loaded;
    loaded.dt = 0;
    require(!SceneSerializer::save(path, loaded) && read(path) == original,
            "invalid save damaged previous scene");
    loaded = valid;
    require(!SceneSerializer::decode("{broken", loaded) && loaded.name == valid.name,
            "invalid load changed current document");
    require(!SceneSerializer::decode(R"({"schema_version":999})", loaded), "unsupported format accepted");
    auto duplicate = valid;
    duplicate.bodies.push_back(duplicate.bodies[0]);
    std::string output;
    require(!SceneSerializer::encode(duplicate, output), "duplicate IDs accepted");
    const auto blocked = folder / "occupied";
    std::filesystem::create_directories(blocked);
    std::ofstream(blocked / "keep") << "original";
    require(!SceneSerializer::atomic_write(blocked, "replacement") && read(blocked / "keep") == "original",
            "failed atomic replacement removed original");
    require(SceneSerializer::filename("../../danger").find('/') == std::string::npos,
            "scene name escaped its folder");
    for (const auto &entry : std::filesystem::directory_iterator(folder))
        require(entry.path().filename().string().find(".tmp-") == std::string::npos,
                "failed save left a temporary file");
}
void exports(const std::filesystem::path &folder) {
    AppPaths paths{folder, folder / "scenes", folder / "exports"};
    std::string error;
    require(paths.ensure(error), "data folder creation failed");
    auto doc = free_scene();
    SimulationController clock;
    clock.configure(doc);
    SnapshotExporter exporter;
    exporter.options = {true, true};
    exporter.begin(paths, doc.name);
    exporter.update(doc, clock, 0, true);
    const auto initial = read(exporter.json_path);
    clock.single_step(doc);
    exporter.update(doc, clock, 0.01);
    require(read(exporter.json_path) == initial, "snapshot writer ignored 10Hz cadence");
    exporter.update(doc, clock, 0.1);
    require(read(exporter.json_path) != initial, "snapshot did not refresh after cadence");
    require(read(exporter.csv_path).find("pos_z,vel_x,vel_y,vel_z") != std::string::npos &&
                exporter.error.empty(),
            "CSV omitted spatial coordinates");
}
} // namespace
int main() {
    const auto folder =
        std::filesystem::temp_directory_path() /
        ("orbitsim-product-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);
    int failed = 0;
    const auto test = [&](const char *name, auto run) {
        try {
            run();
            std::cout << "[Product] " << name << ": OK\n";
        } catch (const std::exception &e) {
            ++failed;
            std::cerr << name << ": " << e.what() << '\n';
        }
    };
    test("Fixed-step playback at 30/60/144 FPS", fixed_clock);
    test("Empty workspace clock", empty_clock);
    test("Stable identity and validated XYZ edits", stable_selection_and_edits);
    test("Selection across collision merge", collision_selection);
    test("Transactional Unicode persistence and failed writes", [&] { persistence(folder); });
    test("Throttled JSON/CSV output", [&] { exports(folder); });
    test("All bundled scenes roundtrip", [] {
        for (const auto &scene : make_builtin_scenes()) {
            std::string text;
            ScenarioPreset result;
            require(bool(SceneSerializer::encode(scene, text)) && bool(SceneSerializer::decode(text, result)),
                    "bundled scene failed schema validation");
        }
    });
    std::filesystem::remove_all(folder);
    return failed ? 1 : 0;
}
