#pragma once
// Minimal non-validating XML reader for the Qt-free core (stand-in for QXmlStreamReader on small
// feeds). Builds a tree; nullopt for anything that is not well formed: unclosed or mismatched tags,
// a missing root, text or a second element after the root, bad attributes or entities.
// Handles the XML declaration, processing instructions, comments, CDATA, DOCTYPE (skipped, no
// internal subset), the five predefined entities and numeric character references.
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hcp::xml {

struct Element {
    std::string name;
    std::vector<std::pair<std::string, std::string>> attributes;
    std::vector<Element> children; // in document order
    std::string text;              // all character data directly inside this element, concatenated
    bool mixed = false;            // has both child elements and non-blank text

    // Attribute value, or "" when absent.
    std::string attribute(std::string_view key) const;
};

std::optional<Element> parse(std::string_view body);

} // namespace hcp::xml
