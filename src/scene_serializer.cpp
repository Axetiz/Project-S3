// Apsilume: Validate and translate scene JSON, then replace complete files without exposing partially written
// destinations. Module relationships and invariants: docs/architecture.md.
#include "scene_serializer.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace orbitsimlite {
namespace {
using Json = nlohmann::json;
bool finite(const Vec3 &v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
FileResult validate(const ScenarioPreset &scene) {
    if (scene.name.empty())
        return {false, "Scene name cannot be empty."};
    if (!std::isfinite(scene.gravity) || scene.gravity < 0)
        return {false, "Gravity must be a finite, non-negative number."};
    if (!std::isfinite(scene.dt) || scene.dt <= 0)
        return {false, "Numerical timestep must be positive and finite."};
    if (!std::isfinite(scene.meters_to_pixels) || scene.meters_to_pixels <= 0)
        return {false, "Scene display scale must be positive and finite."};
    if (scene.substeps < 1 || scene.substeps > 4096)
        return {false, "Substeps must be between 1 and 4096."};
    std::set<BodyId> ids;
    for (const auto &b : scene.bodies) {
        if (!std::isfinite(b.mass) || b.mass <= 0 || !std::isfinite(b.radius) || b.radius <= 0 ||
            !finite(b.pos) || !finite(b.vel))
            return {false, "Body '" + b.name +
                               "' needs finite coordinates, positive mass and positive display radius."};
        if (b.id && (!ids.insert(b.id).second || b.id == std::numeric_limits<BodyId>::max()))
            return {false, "Body IDs must be unique."};
        if (b.color > 0xFFFFFF)
            return {false, "Body color is outside the RGB range."};
    }
    return {true, {}};
}
Json vector(const Vec3 &v) {
    return {{"x", v.x}, {"y", v.y}, {"z", v.z}};
}
Vec3 vector(const Json &j) {
    return {j.at("x").get<double>(), j.at("y").get<double>(), j.value("z", 0.0)};
}
} // namespace
std::string SceneSerializer::filename(const std::string &label) {
    std::string value;
    for (unsigned char c : label)
        value += c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' ||
                         c == '>' || c == '|' || c < 32
                     ? '_'
                     : char(c);
    while (!value.empty() && (value.back() == '.' || value.back() == ' '))
        value.pop_back();
    if (value.empty() || value == "." || value == "..")
        value = "Untitled";
    return value + ".json";
}
FileResult SceneSerializer::encode(const ScenarioPreset &scene, std::string &output) {
    if (auto result = validate(scene); !result)
        return result;
    try {
        Json root = {{"schema_version", schema_version},
                     {"name", scene.name},
                     {"units", scene.units},
                     {"gravity", scene.gravity},
                     {"dt", scene.dt},
                     {"substeps", scene.substeps},
                     {"meters_to_pixels", scene.meters_to_pixels},
                     {"integrator", integrator_name(scene.integrator)},
                     {"bodies", Json::array()}};
        Simulator ids;
        ids.set_bodies(scene.bodies);
        for (const auto &b : ids.get_bodies())
            root["bodies"].push_back({{"id", b.id},
                                      {"name", b.name},
                                      {"mass", b.mass},
                                      {"radius", b.radius},
                                      {"color", b.color},
                                      {"visible", b.visible},
                                      {"is_star", b.is_star},
                                      {"is_satellite", b.is_satellite},
                                      {"position", vector(b.pos)},
                                      {"velocity", vector(b.vel)}});
        output = root.dump(2) + "\n";
        return {true, {}};
    } catch (const Json::exception &error) {
        return {false, std::string("Cannot encode scene: ") + error.what()};
    }
}
FileResult SceneSerializer::decode(const std::string &text, ScenarioPreset &scene) {
    try {
        const auto root = Json::parse(text);
        if (root.contains("schema_version") && !root.at("schema_version").is_number_integer())
            return {false, "Scene format version must be a whole number."};
        const int version = root.value("schema_version", 1);
        if (version < 1 || version > schema_version)
            return {false, "This scene format version is not supported."};
        // Build a separate candidate so malformed input cannot partially replace the current scene.
        ScenarioPreset candidate;
        candidate.name = root.at("name").get<std::string>();
        candidate.gravity = root.at("gravity").get<double>();
        candidate.dt = root.at("dt").get<double>();
        if (!root.at("substeps").is_number_integer())
            return {false, "Substeps must be a whole number."};
        const auto substeps = root.at("substeps").get<double>();
        if (substeps < 1 || substeps > 4096)
            return {false, "Substeps must be between 1 and 4096."};
        candidate.substeps = int(substeps);
        candidate.meters_to_pixels = root.at("meters_to_pixels").get<double>();
        candidate.units = root.value("units", candidate.gravity == 1 ? "dimensionless" : "SI");
        const std::string method = root.at("integrator").get<std::string>();
        if (method != "Euler" && method != "Symplectic Euler" && method != "RK4")
            return {false, "Unknown integration method: " + method};
        candidate.integrator = method == "RK4" ? Integrator::RK4 : Integrator::Euler;
        if (!root.at("bodies").is_array())
            return {false, "The scene's bodies must be a list."};
        for (const auto &item : root.at("bodies")) {
            Body b;
            b.name = item.at("name").get<std::string>();
            b.mass = item.at("mass").get<double>();
            b.radius = item.at("radius").get<double>();
            if (item.contains("id") &&
                (!item.at("id").is_number_integer() || item.at("id").get<double>() < 0))
                return {false, "Body ID must be a nonnegative whole number."};
            b.id = item.value("id", BodyId{0});
            if (!item.at("color").is_number_integer() || item.at("color").get<double>() < 0 ||
                item.at("color").get<double>() > 0xFFFFFF)
                return {false, "Body color is outside the RGB range."};
            b.color = item.at("color").get<std::uint32_t>();
            b.visible = item.value("visible", true);
            b.is_star = item.value("is_star", false);
            b.is_satellite = item.value("is_satellite", false);
            if (item.contains("position")) {
                b.pos = vector(item.at("position"));
                b.vel = vector(item.at("velocity"));
            } else {
                // Legacy scenes stored flat coordinates; a missing Z means the original XY plane.
                b.pos = {item.at("pos_x").get<double>(), item.at("pos_y").get<double>(),
                         item.value("pos_z", 0.0)};
                b.vel = {item.at("vel_x").get<double>(), item.at("vel_y").get<double>(),
                         item.value("vel_z", 0.0)};
            }
            candidate.bodies.push_back(b);
        }
        if (auto result = validate(candidate); !result)
            return result;
        Simulator ids;
        ids.set_bodies(candidate.bodies);
        candidate.bodies = ids.get_bodies();
        scene = std::move(candidate);
        return {true, {}};
    } catch (const Json::parse_error &error) {
        return {false, "Invalid JSON near byte " + std::to_string(error.byte) +
                           ". Check commas, quotes and brackets."};
    } catch (const Json::exception &error) {
        return {false, std::string("Scene field is missing or has the wrong type: ") + error.what()};
    }
}
FileResult SceneSerializer::atomic_write(const std::filesystem::path &path, const std::string &text) {
    static std::atomic<unsigned> sequence{0};
    std::error_code ec;
    if (!path.parent_path().empty())
        std::filesystem::create_directories(path.parent_path(), ec);
    if (ec)
        return {false, "Cannot create save folder: " + ec.message()};
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    // Same-directory staging keeps the final rename on the destination filesystem.
    const auto temp = std::filesystem::path(path.string() + ".tmp-" + std::to_string(stamp) + "-" +
                                            std::to_string(sequence++));
    {
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        if (!file)
            return {false,
                    "Cannot write " + path.string() + ". Check folder permissions and available space."};
        file << text;
        file.flush();
        if (!file) {
            file.close();
            std::filesystem::remove(temp, ec);
            return {false, "Save failed; the previous file was kept."};
        }
        file.close();
        if (!file) {
            std::filesystem::remove(temp, ec);
            return {false, "Could not finish writing the new file; the previous file was kept."};
        }
    }
#ifdef _WIN32
    if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(temp, ec);
        return {false, "Cannot replace the scene file. Close programs holding it open and retry."};
    }
#else
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        const auto message = ec.message();
        std::filesystem::remove(temp, ec);
        return {false, "Cannot replace " + path.string() + ": " + message + ". Previous file kept."};
    }
#endif
    return {true, "Saved " + path.string()};
}
FileResult SceneSerializer::save(const std::filesystem::path &path, const ScenarioPreset &scene) {
    std::string text;
    auto result = encode(scene, text);
    return result ? atomic_write(path, text) : result;
}
FileResult SceneSerializer::load(const std::filesystem::path &path, ScenarioPreset &scene) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return {false, "Cannot open " + path.string() + ". Check that the file exists and is readable."};
    std::ostringstream text;
    text << input.rdbuf();
    if (input.bad())
        return {false, "Reading the scene failed. The current scene was kept."};
    return decode(text.str(), scene);
}
} // namespace orbitsimlite
