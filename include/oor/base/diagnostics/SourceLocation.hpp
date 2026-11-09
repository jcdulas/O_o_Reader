#pragma once

#include <cstdint>
#include <string>

namespace oor::base
{

// Where a problem was found. All parts are optional: a diagnostic about a whole file has no line.
struct SourceLocation
{
    std::string file;
    std::uint32_t line = 0; // 1-based; 0 when unknown
    std::string cardName;   // Bulk Data card or Case Control command, when known
    std::string fieldName;  // field of the card, when known

    // "file:line (CARD, field)", with the missing parts left out.
    [[nodiscard]] std::string toString() const;
};

} // namespace oor::base
