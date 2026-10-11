#include "../src/app/instrument_catalog.hpp"

#include <cstring>
#include <iostream>
#include <set>
#include <string>

namespace {
int checks = 0;
int failures = 0;
void check(bool condition, const char* message) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}
void bounded_text(const char* text, std::size_t limit, const char* message) {
    check(text != nullptr && text[0] != '\0' && std::strlen(text) <= limit,
          message);
}
} // namespace

int main() {
    using namespace hamclock;
    const auto& catalog = instrument_catalog();
    check(catalog.size() == 24, "known manual-family catalog size");
    std::set<unsigned> ids;
    std::set<std::string> keys;
    for (const auto& item : catalog) {
        check(ids.insert(static_cast<unsigned>(item.id)).second, "unique ID");
        bounded_text(item.key, 64, "bounded nonempty stable key");
        if (item.key != nullptr) {
            check(keys.insert(item.key).second, "unique key");
            for (const unsigned char c : std::string(item.key)) {
                check((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-',
                      "key is bounded plain ASCII token");
            }
        }
        bounded_text(item.label, 128, "bounded nonempty plain label");
        bounded_text(item.unavailable_reason, 256, "honest dependency reason");
        check(item.availability == CatalogAvailability::Unavailable,
              "catalog does not claim an integrated reading");
        check(find_instrument(item.id) == &item, "lookup returns stable descriptor");
    }
    check(find_instrument(InstrumentId::Countdown) != nullptr,
          "Countdown remains a distinct known pane family");
    check(find_instrument(InstrumentId::Moon) != nullptr, "Moon family retained");
    check(find_instrument(static_cast<InstrumentId>(0)) == nullptr,
          "zero unknown ID rejected");
    check(find_instrument(static_cast<InstrumentId>(25)) == nullptr,
          "value after bounded family list rejected");
    check(find_instrument(static_cast<InstrumentId>(65535)) == nullptr,
          "maximum underlying ID rejected");

    const std::array<InstrumentSlot, 3> slots{
        InstrumentSlot::First, InstrumentSlot::Second, InstrumentSlot::Third};
    for (const auto slot : slots) {
        check(InstrumentSelection::valid_slot(slot), "documented slot accepted");
    }
    for (const auto bad : {static_cast<InstrumentSlot>(3),
                           static_cast<InstrumentSlot>(255)}) {
        check(!InstrumentSelection::valid_slot(bad), "cast-in invalid slot rejected");
        InstrumentSelection selection;
        const auto before = selection.assignments();
        check(selection.assign(bad, InstrumentId::SolarFlux) == SelectionResult::InvalidSlot,
              "invalid assignment reports invalid slot");
        check(selection.clear(bad) == SelectionResult::InvalidSlot,
              "invalid clear reports invalid slot");
        check(!selection.selected(bad).has_value(), "invalid query safely empty");
        check(selection.assignments() == before, "invalid operations preserve state");
    }

    for (const auto& item : catalog) {
        for (const auto slot : slots) {
            InstrumentSelection selection;
            check(selection.assignments() == std::array<std::optional<InstrumentId>, 3>{},
                  "initial preparation state is empty");
            check(selection.clear(slot) == SelectionResult::Unchanged,
                  "clearing empty slot is unchanged");
            check(selection.assign(slot, item.id) == SelectionResult::Applied,
                  "known unavailable family can be configured without data");
            check(selection.selected(slot) == item.id, "selected value round trips");
            check(selection.assign(slot, item.id) == SelectionResult::Unchanged,
                  "repeated assignment is unchanged");
            const auto before = selection.assignments();
            for (const auto other : slots) {
                if (other != slot) {
                    check(selection.assign(other, item.id) == SelectionResult::AlreadyAssigned,
                          "same family cannot occupy another slot");
                    check(selection.assignments() == before,
                          "duplicate rejection is atomic and does not transfer");
                }
            }
            for (const auto unknown : {static_cast<InstrumentId>(0),
                                      static_cast<InstrumentId>(65535)}) {
                check(selection.assign(slot, unknown) == SelectionResult::UnknownInstrument,
                      "unknown ID rejection explicit");
                check(selection.assignments() == before, "unknown rejection preserves choice");
            }
            check(selection.clear(slot) == SelectionResult::Applied, "clear removes choice");
            check(!selection.selected(slot).has_value(), "clear returns to empty state");
            check(selection.clear(slot) == SelectionResult::Unchanged, "clear is idempotent");
        }
    }
    InstrumentSelection selection;
    check(selection.assign(InstrumentSlot::First, InstrumentId::SolarFlux) == SelectionResult::Applied,
          "first independent assignment");
    check(selection.assign(InstrumentSlot::Second, InstrumentId::Propagation) == SelectionResult::Applied,
          "second independent assignment");
    check(selection.assign(InstrumentSlot::Third, InstrumentId::Countdown) == SelectionResult::Applied,
          "third independent assignment");
    const auto full = selection.assignments();
    check(selection.assign(InstrumentSlot::First, InstrumentId::Countdown) == SelectionResult::AlreadyAssigned,
          "full-state collision rejected");
    check(selection.assignments() == full, "full-state collision preserves all three");
    check(selection.assign(InstrumentSlot::First, InstrumentId::Moon) == SelectionResult::Applied,
          "different family replaces only requested slot");
    check(selection.selected(InstrumentSlot::Second) == InstrumentId::Propagation &&
          selection.selected(InstrumentSlot::Third) == InstrumentId::Countdown,
          "unrelated choices survive replacement");
    std::cout << checks << " instrument catalog checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
