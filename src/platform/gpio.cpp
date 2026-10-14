#include "gpio.hpp"

namespace hamclock::platform {

std::optional<int> parse_bcm_line(const QString& text) {
    QString t = text.trimmed();
    if (t.startsWith("BCM", Qt::CaseInsensitive)) t = t.mid(3);
    bool ok = false;
    const int n = t.toInt(&ok);
    if (!ok || n < 2 || n > 27) return std::nullopt;
    return n;
}

bool FakeGpioOutputs::set(int bcmLine, bool high) {
    if (bcmLine < 2 || bcmLine > 27) return false;
    levels_[bcmLine] = high;
    return true;
}

std::optional<bool> FakeGpioOutputs::lastDriven(int bcmLine) const {
    const auto it = levels_.find(bcmLine);
    if (it == levels_.end()) return std::nullopt;
    return it->second;
}

std::unique_ptr<GpioOutputs> make_default_gpio_outputs() {
    return std::make_unique<NullGpioOutputs>();
}

} // namespace hamclock::platform
