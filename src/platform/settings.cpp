#include "settings.hpp"

#include <cerrno>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace hamclock {
namespace {
constexpr std::size_t limit = 4096;
void validate(const StationSettings& settings) {
    if (!valid_location(settings.de) || !valid_location(settings.dx))
        throw std::invalid_argument("Invalid station coordinates");
    // The standard iostream parser can reject subnormal decimal values on
    // supported libraries. Reject them before saving rather than create a file
    // this implementation cannot reliably reload or silently round to zero.
    for (const double coordinate : {settings.de.latitude, settings.de.longitude,
                                    settings.dx.latitude, settings.dx.longitude})
        if (std::fpclassify(coordinate) == FP_SUBNORMAL)
            throw std::invalid_argument("Subnormal coordinates are not supported in settings");
    if (settings.callsign.empty() || settings.callsign.size() > 16)
        throw std::invalid_argument("Callsign must contain 1 to 16 ASCII characters");
    for (const unsigned char c : settings.callsign)
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '/' || c == '-'))
            throw std::invalid_argument("Invalid callsign character");
    if (settings.appearance != "system" && settings.appearance != "light" &&
        settings.appearance != "dark")
        throw std::invalid_argument("Invalid appearance");
    if (!settings.de_zone.empty()) {
        if (settings.de_zone.size() > 63) throw std::invalid_argument("DE zone id too long");
        for (const unsigned char c : settings.de_zone)
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                  c == '_' || c == '+' || c == '-' || c == '/'))
                throw std::invalid_argument("Invalid DE zone id character");
        if (settings.de_zone.find("..") != std::string::npos)
            throw std::invalid_argument("Invalid DE zone id");
    }
}
void validate_path(const std::string& path) {
    if (path.empty() || path.find('\0') != std::string::npos)
        throw std::invalid_argument("Invalid settings path");
}
std::runtime_error io_error(const char* operation) {
    return std::runtime_error(std::string(operation) + ": " + std::strerror(errno));
}
double number(const std::string& value) {
    if (value.empty() || value.size() > 64)
        throw std::invalid_argument("Invalid coordinate encoding");
    for (const unsigned char c : value)
        if (!((c >= '0' && c <= '9') || c == '+' || c == '-' ||
              c == '.' || c == 'e' || c == 'E'))
            throw std::invalid_argument("Invalid coordinate encoding");
    std::istringstream stream(value);
    stream.imbue(std::locale::classic());
    double result;
    stream >> result;
    if (!stream || stream.peek() != std::char_traits<char>::eof())
        throw std::invalid_argument("Invalid coordinate encoding");
    return result;
}
StationSettings parse(const std::string& data) {
    if (data.empty() || data.back() != '\n')
        throw std::invalid_argument("Settings require a final newline");
    std::map<std::string, std::string> fields;
    std::istringstream lines(data);
    std::string line;
    while (std::getline(lines, line)) {
        const auto equal = line.find('=');
        if (equal == std::string::npos || equal == 0 ||
            line.find('=', equal + 1) != std::string::npos)
            throw std::invalid_argument("Malformed settings field");
        const auto key = line.substr(0, equal);
        if (key != "version" && key != "de_lat" && key != "de_lon" &&
            key != "dx_lat" && key != "dx_lon" && key != "callsign" &&
            key != "appearance" && key != "de_zone")
            throw std::invalid_argument("Unknown settings field");
        if (!fields.emplace(key, line.substr(equal + 1)).second)
            throw std::invalid_argument("Duplicate settings field");
    }
    // A file without the appearance line (written by hand or an older helper) follows the system.
    fields.emplace("appearance", "system");
    const bool has_zone = fields.count("de_zone") != 0;
    if (fields.size() != (has_zone ? 8u : 7u) ||
        fields.count("version") == 0 || fields.at("version") != "1")
        throw std::invalid_argument("Missing settings field or unsupported version");
    if (has_zone && fields.at("de_zone").empty())
        throw std::invalid_argument("Empty DE zone field");
    StationSettings settings{{number(fields.at("de_lat")), number(fields.at("de_lon"))},
                             {number(fields.at("dx_lat")), number(fields.at("dx_lon"))},
                             fields.at("callsign"), fields.at("appearance"),
                             has_zone ? fields.at("de_zone") : std::string()};
    validate(settings);
    return settings;
}
} // namespace

namespace {
void save_impl(const std::string& path, const StationSettings& settings, bool replace) {
    validate_path(path);
    validate(settings);
    if (replace) {
        struct stat destination {};
        if (::lstat(path.c_str(), &destination) == 0) {
            if (!S_ISREG(destination.st_mode))
                throw std::runtime_error("Settings destination must be a regular file");
        } else if (errno != ENOENT) {
            throw io_error("Inspect settings destination");
        }
    }
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::setprecision(17) << "version=1\nde_lat=" << settings.de.latitude
           << "\nde_lon=" << settings.de.longitude << "\ndx_lat=" << settings.dx.latitude
           << "\ndx_lon=" << settings.dx.longitude << "\ncallsign=" << settings.callsign
           << "\nappearance=" << settings.appearance << '\n';
    if (!settings.de_zone.empty()) output << "de_zone=" << settings.de_zone << '\n';
    const std::string data = output.str();
    std::string pattern = path + ".tmp.XXXXXX";
    std::vector<char> temporary(pattern.begin(), pattern.end());
    temporary.push_back('\0');
    int fd = ::mkstemp(temporary.data());
    if (fd < 0) throw io_error("Create settings temporary file");
    try {
        if (::fchmod(fd, S_IRUSR | S_IWUSR) != 0) throw io_error("Set settings permissions");
        std::size_t written = 0;
        while (written < data.size()) {
            const ssize_t count = ::write(fd, data.data() + written, data.size() - written);
            if (count < 0 && errno == EINTR) continue;
            if (count < 0) throw io_error("Write settings");
            if (count == 0) throw std::runtime_error("Settings write made no progress");
            written += static_cast<std::size_t>(count);
        }
        if (::fsync(fd) != 0) throw io_error("Synchronize settings");
        const int closed = ::close(fd);
        fd = -1;
        if (closed != 0) throw io_error("Close settings");
        if (replace) {
            if (::rename(temporary.data(), path.c_str()) != 0) throw io_error("Replace settings");
        } else {
            // Same-directory hardlink publishes only if the final name is absent.
            // This is atomic even if another writer creates it during our save.
            if (::link(temporary.data(), path.c_str()) != 0) throw io_error("Create settings destination");
            // Publication succeeded. A cleanup failure must not report that the
            // user's save failed; the private duplicate can be removed later.
            (void)::unlink(temporary.data());
        }
    } catch (...) {
        const int saved_errno = errno;
        if (fd >= 0) ::close(fd);
        ::unlink(temporary.data());
        errno = saved_errno;
        throw;
    }
}

} // namespace

void save_settings(const std::string& path, const StationSettings& settings) {
    save_impl(path, settings, true);
}
void save_new_settings(const std::string& path, const StationSettings& settings) {
    save_impl(path, settings, false);
}

StationSettings load_settings(const std::string& path) {
    validate_path(path);
    const int fd = ::open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) throw io_error("Open settings");
    std::string data;
    try {
        struct stat status {};
        if (::fstat(fd, &status) != 0) throw io_error("Inspect settings");
        if (!S_ISREG(status.st_mode)) throw std::runtime_error("Settings must be a regular file");
        if (status.st_size < 0 || status.st_size > static_cast<off_t>(limit))
            throw std::invalid_argument("Settings exceed 4096 bytes");
        char buffer[512];
        while (true) {
            const ssize_t count = ::read(fd, buffer, sizeof(buffer));
            if (count < 0 && errno == EINTR) continue;
            if (count < 0) throw io_error("Read settings");
            if (count == 0) break;
            if (data.size() + static_cast<std::size_t>(count) > limit)
                throw std::invalid_argument("Settings exceed 4096 bytes");
            data.append(buffer, static_cast<std::size_t>(count));
        }
    } catch (...) {
        ::close(fd);
        throw;
    }
    if (::close(fd) != 0) throw io_error("Close settings");
    return parse(data);
}
} // namespace hamclock
