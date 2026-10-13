#include "settings.hpp"

#include <cmath>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace hamclock {
int run_settings_tests() {
    int failures = 0;
    auto check = [&](bool condition, const char* name) {
        if (!condition) { ++failures; std::cerr << "Settings test failed: " << name << '\n'; }
    };
    char pattern[] = "/tmp/hamclock-settings-XXXXXX";
    char* created = ::mkdtemp(pattern);
    if (!created) { std::cerr << "Cannot create settings test directory\n"; return 1; }
    const std::string directory(created), path = directory + "/station.conf";
    auto rejects_load = [&](const std::string& data) {
        { std::ofstream file(path, std::ios::binary | std::ios::trunc); file << data; }
        try { (void)load_settings(path); } catch (const std::invalid_argument&) { return true; }
        return false;
    };
    const std::string valid = "version=1\nde_lat=37.7\nde_lon=-122.4\ndx_lat=51.5\ndx_lon=-0.1\ncallsign=N0CALL/P\nappearance=system\n";
    try {
        const StationSettings original{{37.71234567890123, -122.4}, {51.5, -0.1}, "N0CALL/P", "system", {}};
        save_new_settings(path, original);
        bool existing_new_rejected = false;
        auto replacement = original; replacement.callsign = "N1TEST";
        try { save_new_settings(path, replacement); }
        catch (const std::runtime_error&) { existing_new_rejected = true; }
        check(existing_new_rejected && load_settings(path).callsign == original.callsign,
              "new save cannot replace existing regular file");
        const auto loaded = load_settings(path);
        check(loaded.de.latitude == original.de.latitude && loaded.de.longitude == original.de.longitude &&
              loaded.dx.latitude == original.dx.latitude && loaded.dx.longitude == original.dx.longitude &&
              loaded.callsign == original.callsign && loaded.appearance == "system", "exact round trip");
        struct stat status {};
        check(::stat(path.c_str(), &status) == 0 && (status.st_mode & 0777) == 0600, "private permissions");
        for (const auto& appearance : {"light", "dark"}) {
            auto changed = original; changed.appearance = appearance;
            save_settings(path, changed);
            check(load_settings(path).appearance == appearance, "appearance round trip");
        }
        // Optional DE IANA zone id: round trip, absent in legacy files, bad ids rejected.
        {
            check(load_settings(path).de_zone.empty(), "legacy file has no DE zone");
            auto zoned = original; zoned.de_zone = "America/New_York";
            save_settings(path, zoned);
            check(load_settings(path).de_zone == "America/New_York", "DE zone round trip");
            for (const char* bad : {"bad zone", "A=B", "../../etc", "Zone\n"}) {
                auto invalid_zone = original; invalid_zone.de_zone = bad;
                bool thrown = false;
                try { save_settings(path, invalid_zone); } catch (const std::invalid_argument&) { thrown = true; }
                check(thrown, "invalid DE zone rejected on save");
            }
            auto too_long = original; too_long.de_zone = std::string(64, 'A');
            bool long_thrown = false;
            try { save_settings(path, too_long); } catch (const std::invalid_argument&) { long_thrown = true; }
            check(long_thrown, "overlong DE zone rejected on save");
            check(load_settings(path).de_zone == "America/New_York", "failed zone saves keep prior file");
            check(rejects_load(valid + "de_zone=bad zone\n"), "invalid DE zone rejected on load");
            check(rejects_load(valid + "de_zone=\n"), "empty DE zone field rejected on load");
            save_settings(path, original);
        }
        auto normal = original;
        normal.de.latitude = std::numeric_limits<double>::min();
        save_settings(path, normal);
        check(load_settings(path).de.latitude == normal.de.latitude, "smallest normal round trip");
        save_settings(path, original);
        for (const double tiny : {std::numeric_limits<double>::denorm_min(),
                                  std::numeric_limits<double>::min() / 2.0,
                                  -std::numeric_limits<double>::denorm_min()}) {
            for (int coordinate = 0; coordinate < 4; ++coordinate) {
                auto subnormal = original;
                double* coordinates[] = {&subnormal.de.latitude, &subnormal.de.longitude,
                                         &subnormal.dx.latitude, &subnormal.dx.longitude};
                *coordinates[coordinate] = tiny;
                bool rejected = false;
                try { save_settings(path, subnormal); }
                catch (const std::invalid_argument&) { rejected = true; }
                const auto unchanged = load_settings(path);
                check(rejected && unchanged.de.latitude == original.de.latitude &&
                      unchanged.de.longitude == original.de.longitude &&
                      unchanged.dx.latitude == original.dx.latitude &&
                      unchanged.dx.longitude == original.dx.longitude,
                      "subnormal rejected before replacement; prior coordinates preserved");
            }
        }
        auto dark = original; dark.appearance = "dark";
        save_settings(path, dark);
        auto invalid = original;
        invalid.de.latitude = std::numeric_limits<double>::infinity();
        bool invalid_thrown = false;
        try { save_settings(path, invalid); } catch (const std::invalid_argument&) { invalid_thrown = true; }
        check(invalid_thrown && load_settings(path).appearance == "dark", "invalid save leaves prior file intact");
        invalid = original; invalid.callsign = "$(touch hacked)";
        invalid_thrown = false;
        try { save_settings(path, invalid); } catch (const std::invalid_argument&) { invalid_thrown = true; }
        check(invalid_thrown, "callsign treated as bounded data");
        check(rejects_load(valid + "version=1\n"), "duplicate field");
        check(rejects_load(valid + "unknown=1\n"), "unknown field");
        check(rejects_load("version=1\n"), "missing fields");
        auto bad = valid; bad.replace(8, 1, "2");
        check(rejects_load(bad), "unsupported version");
        bad = valid; bad.replace(bad.find("37.7"), 4, "nan");
        check(rejects_load(bad), "nonfinite data");
        bad = valid; bad.replace(bad.find("37.7"), 4, "91");
        check(rejects_load(bad), "coordinate bounds");
        bad = valid; bad.replace(bad.find("37.7"), 4, "0x1p2");
        check(rejects_load(bad), "hex coordinate rejected");
        bad = valid; bad.replace(bad.find("system"), 6, "auto");
        check(rejects_load(bad), "unknown appearance");
        bad = valid; bad.replace(bad.find("N0CALL/P"), 8, std::string(17, 'A'));
        check(rejects_load(bad), "callsign length bounded");
        check(rejects_load(valid.substr(0, valid.size() - 1)), "truncation rejected");
        check(rejects_load(std::string(4097, 'x')), "oversize rejected");
        save_settings(path, original);
        bool io_thrown = false;
        try { save_settings(directory + "/missing/station.conf", original); }
        catch (const std::runtime_error&) { io_thrown = true; }
        check(io_thrown && load_settings(path).callsign == original.callsign, "failed path preserves prior settings");
        io_thrown = false;
        try { save_settings(directory, original); } catch (const std::runtime_error&) { io_thrown = true; }
        check(io_thrown, "directory destination rejected");
        const std::string link = directory + "/link.conf";
        check(::symlink(path.c_str(), link.c_str()) == 0, "create test symlink");
        io_thrown = false;
        try { (void)load_settings(link); } catch (const std::runtime_error&) { io_thrown = true; }
        check(io_thrown, "symlink load rejected");
        io_thrown = false;
        try { save_settings(link, original); } catch (const std::runtime_error&) { io_thrown = true; }
        struct stat link_status {};
        check(io_thrown && ::lstat(link.c_str(), &link_status) == 0 && S_ISLNK(link_status.st_mode),
              "symlink save rejected and link preserved");
        check(load_settings(path).callsign == original.callsign, "symlink target preserved");
        io_thrown = false;
        try { save_new_settings(link, replacement); } catch (const std::runtime_error&) { io_thrown = true; }
        check(io_thrown && ::lstat(link.c_str(), &link_status) == 0 && S_ISLNK(link_status.st_mode) &&
              load_settings(path).callsign == original.callsign, "new save preserves symlink and target");
        ::unlink(link.c_str());
        const std::string fifo = directory + "/pipe.conf";
        check(::mkfifo(fifo.c_str(), 0600) == 0, "create test fifo");
        io_thrown = false;
        try { (void)load_settings(fifo); } catch (const std::runtime_error&) { io_thrown = true; }
        check(io_thrown, "nonregular file rejected without blocking");
        io_thrown = false;
        try { save_settings(fifo, original); } catch (const std::runtime_error&) { io_thrown = true; }
        struct stat fifo_status {};
        check(io_thrown && ::lstat(fifo.c_str(), &fifo_status) == 0 && S_ISFIFO(fifo_status.st_mode),
              "FIFO save rejected and FIFO preserved");
        io_thrown = false;
        try { save_new_settings(fifo, original); } catch (const std::runtime_error&) { io_thrown = true; }
        check(io_thrown && ::lstat(fifo.c_str(), &fifo_status) == 0 && S_ISFIFO(fifo_status.st_mode),
              "new save preserves FIFO");
        ::unlink(fifo.c_str());
        const auto fresh_path = directory + "/new.conf";
        auto invalid_new = original; invalid_new.callsign.clear();
        bool invalid_new_rejected = false;
        try { save_new_settings(fresh_path, invalid_new); }
        catch (const std::invalid_argument&) { invalid_new_rejected = true; }
        check(invalid_new_rejected && ::access(fresh_path.c_str(), F_OK) != 0,
              "invalid new save does not publish a file");
        bool new_directory_rejected = false;
        try { save_new_settings(directory, original); }
        catch (const std::runtime_error&) { new_directory_rejected = true; }
        check(new_directory_rejected, "new save cannot replace directory");
        bool new_parent_rejected = false;
        try { save_new_settings(directory + "/missing/new.conf", original); }
        catch (const std::runtime_error&) { new_parent_rejected = true; }
        check(new_parent_rejected, "new save missing parent fails");
        ::unlink(path.c_str());
        bool missing_thrown = false;
        try { (void)load_settings(path); } catch (const std::runtime_error&) { missing_thrown = true; }
        check(missing_thrown, "missing settings reported");
        save_settings(path, original);
        check(load_settings(path).callsign == original.callsign, "explicit recovery save works");
    } catch (const std::exception& error) {
        ++failures; std::cerr << "Unexpected settings test exception: " << error.what() << '\n';
    }
    ::unlink(path.c_str());
    // Every exclusive temporary file must have been cleaned up.
    check(::rmdir(directory.c_str()) == 0, "temporary files cleaned up");
    return failures;
}
} // namespace hamclock
