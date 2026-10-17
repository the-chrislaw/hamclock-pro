#include "astro_qr.hpp"
#include "astro_qr/qrcodegen.hpp"
#include <stdexcept>
namespace hamclock {
QrMatrix qr_encode(const std::string& text) {
    QrMatrix m;
    try {
        const auto qr = qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
        m.size = qr.getSize();
        m.dark.assign(size_t(m.size) * m.size, false);
        for (int y = 0; y < m.size; ++y)
            for (int x = 0; x < m.size; ++x) m.dark[size_t(y) * m.size + x] = qr.getModule(x, y);
    } catch (const std::length_error&) {
        m = QrMatrix{};
    }
    return m;
}
} // namespace hamclock
