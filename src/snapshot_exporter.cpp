// Apsilume: Publish the latest physical state to one stable JSON/CSV pair per session, at a bounded real-time
// cadence. Module relationships and invariants: docs/architecture.md.
#include "snapshot_exporter.hpp"
#include "scene_serializer.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
namespace orbitsimlite {
namespace {
std::string quote(const std::string &value) {
    std::string result = "\"";
    for (char c : value)
        result += c == '"' ? "\"\"" : std::string(1, c);
    return result + '"';
}
} // namespace
void SnapshotExporter::begin(const AppPaths &paths, const std::string &name) {
    const auto now = std::chrono::system_clock::now();
    const auto stamp = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &stamp);
#else
    localtime_r(&stamp, &local);
#endif
    std::ostringstream base;
    base << "bodies_" << std::put_time(&local, "%Y%m%d_%H%M%S") << '_'
         << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000
         << '_' << SceneSerializer::filename(name);
    json_path = paths.exports / base.str();
    csv_path = json_path;
    csv_path.replace_extension(".csv");
    elapsed_ = 1;
    error.clear();
}
void SnapshotExporter::update(const SceneDocument &document, const SimulationController &controller,
                              double seconds, bool force) {
    // Cadence is real time, even while paused. Each output is a latest-state snapshot, not history.
    elapsed_ += seconds;
    if (!force && elapsed_ < 0.1)
        return;
    elapsed_ = 0;
    using Json = nlohmann::json;
    const auto vector = [](const Vec3 &v) { return Json{{"x", v.x}, {"y", v.y}, {"z", v.z}}; };
    if (options.enable_json) {
        Json root = {{"simulation_name", document.name},
                     {"time_seconds", document.simulation.get_time()},
                     {"time_scale", controller.speed},
                     {"collision_mode", collision_name(controller.collisions)},
                     {"body_count", document.simulation.get_bodies().size()},
                     {"bodies", Json::array()}};
        for (const auto &b : document.simulation.get_bodies())
            root["bodies"].push_back({{"id", b.id},
                                      {"name", b.name},
                                      {"mass", b.mass},
                                      {"radius", b.radius},
                                      {"color", b.color},
                                      {"visible", b.visible},
                                      {"is_star", b.is_star},
                                      {"is_satellite", b.is_satellite},
                                      {"position", vector(b.pos)},
                                      {"velocity", vector(b.vel)},
                                      {"acceleration", vector(b.acc)}});
        auto result = SceneSerializer::atomic_write(json_path, root.dump(2) + "\n");
        if (!result)
            error = result.message;
    }
    if (options.enable_csv) {
        std::ostringstream csv;
        csv << std::setprecision(17)
            << "time_seconds,body_id,name,mass,radius,color,is_satellite,is_star,pos_x,pos_y,pos_z,vel_x,vel_"
               "y,vel_z,acc_x,acc_y,acc_z\n";
        for (const auto &b : document.simulation.get_bodies())
            csv << document.simulation.get_time() << ',' << b.id << ',' << quote(b.name) << ',' << b.mass
                << ',' << b.radius << ',' << b.color << ',' << b.is_satellite << ',' << b.is_star << ','
                << b.pos.x << ',' << b.pos.y << ',' << b.pos.z << ',' << b.vel.x << ',' << b.vel.y << ','
                << b.vel.z << ',' << b.acc.x << ',' << b.acc.y << ',' << b.acc.z << '\n';
        auto result = SceneSerializer::atomic_write(csv_path, csv.str());
        if (!result)
            error = result.message;
    }
}
} // namespace orbitsimlite
