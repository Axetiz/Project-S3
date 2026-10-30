// Apsilume: Resolve installation assets and persistent user data separately, so launches do not depend on the
// working directory. Module relationships and invariants: docs/architecture.md.
#include "app_paths.hpp"
#include <cstdlib>
#include <system_error>
namespace orbitsimlite {
AppPaths AppPaths::discover() {
    std::filesystem::path root;
    if (const char *override_path = std::getenv("ORBITSIMLITE_DATA_DIR"); override_path && *override_path)
        root = override_path;
    else {
#ifdef _WIN32
        const char *base = std::getenv("LOCALAPPDATA");
        root = base ? std::filesystem::path(base) : std::filesystem::temp_directory_path();
#elif defined(__APPLE__)
        const char *home = std::getenv("HOME");
        root = home ? std::filesystem::path(home) / "Library/Application Support"
                    : std::filesystem::temp_directory_path();
#else
        const char *xdg = std::getenv("XDG_DATA_HOME");
        const char *home = std::getenv("HOME");
        root = xdg ? std::filesystem::path(xdg)
                   : (home ? std::filesystem::path(home) / ".local/share"
                           : std::filesystem::temp_directory_path());
#endif
        // Keep the on-disk compatibility name: changing the brand must not hide existing user scenes.
        root /= "OrbitSimLite/Experimental";
    }
    root = std::filesystem::absolute(root);
    return {root, root / "scenes", root / "exports"};
}
std::filesystem::path AppPaths::executable(const std::string &argument) {
    std::error_code ec;
#ifdef __linux__
    const auto running = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec)
        return running;
#endif
    // argv[0] may contain only a command name, so asset discovery needs a PATH search fallback.
    const auto candidate = std::filesystem::path(argument);
    if (candidate.has_parent_path() || std::filesystem::is_regular_file(candidate, ec))
        return std::filesystem::weakly_canonical(std::filesystem::absolute(candidate));
    if (const auto *path = std::getenv("PATH")) {
        const std::string search = path;
#ifdef _WIN32
        const char separator = ';';
#else
        const char separator = ':';
#endif
        std::size_t begin = 0;
        while (begin <= search.size()) {
            const auto end = search.find(separator, begin);
            const auto file = std::filesystem::path(search.substr(begin, end - begin)) / candidate;
            if (std::filesystem::is_regular_file(file, ec))
                return std::filesystem::weakly_canonical(std::filesystem::absolute(file));
            if (end == std::string::npos)
                break;
            begin = end + 1;
        }
    }
    return std::filesystem::absolute(candidate);
}
bool AppPaths::ensure(std::string &error) const {
    std::error_code ec;
    for (const auto &path : {scenes, exports}) {
        std::filesystem::create_directories(path, ec);
        if (ec) {
            error = "Cannot create " + path.string() + ": " + ec.message();
            return false;
        }
    }
    return true;
}
} // namespace orbitsimlite
