#pragma once
// GPIO output interface (alarm, countdown light, ON AIR, satellite alarm). Widgets
// talk only to this interface so they build and run unchanged on macOS, iPad and
// Pi installs without GPIO. The default backend is Null: every operation reports
// "unavailable" and nothing is driven. A Linux (gpiod) backend is a later slice and
// needs on-device testing. Outputs only; no inputs, no privileged calls here.
#include <QString>
#include <map>
#include <memory>
#include <optional>

namespace hamclock::platform {

// BCM line number 2..27 (the Pi 40-pin header GPIOs). Accepts "17" or "BCM17".
std::optional<int> parse_bcm_line(const QString& text);

class GpioOutputs {
public:
    virtual ~GpioOutputs() = default;
    virtual bool available() const = 0;
    // False when unavailable, the line is invalid or the backend refuses.
    virtual bool set(int bcmLine, bool high) = 0;
    // Last level this object drove; absent if never set (never a hardware read-back claim).
    virtual std::optional<bool> lastDriven(int bcmLine) const = 0;
};

// No GPIO on this platform: never drives anything.
class NullGpioOutputs final : public GpioOutputs {
public:
    bool available() const override { return false; }
    bool set(int, bool) override { return false; }
    std::optional<bool> lastDriven(int) const override { return std::nullopt; }
};

// In-memory backend for tests and screenshot runs.
class FakeGpioOutputs final : public GpioOutputs {
public:
    bool available() const override { return true; }
    bool set(int bcmLine, bool high) override;
    std::optional<bool> lastDriven(int bcmLine) const override;
private:
    std::map<int, bool> levels_;
};

// Platform default (Null until a hardware backend lands).
std::unique_ptr<GpioOutputs> make_default_gpio_outputs();

} // namespace hamclock::platform
