#pragma once
// Internal helpers shared by the hcpcore*.cpp C ABI wrappers (not part of the public header).
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>

namespace hcp::abi {

inline std::string_view view(const char* body, size_t len) {
    return body ? std::string_view(body, len) : std::string_view();
}

// NUL-terminated copy truncated to cap, never splitting a UTF-8 sequence. Returns 1 when non-empty.
inline int copy(const std::string& s, char* buf, size_t cap) {
    if (!buf || cap == 0) return 0;
    size_t n = s.size() < cap - 1 ? s.size() : cap - 1;
    if (n < s.size())
        while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) --n;
    std::memcpy(buf, s.data(), n);
    buf[n] = '\0';
    return n > 0;
}

} // namespace hcp::abi
