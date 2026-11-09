#pragma once

#include "oor/base/diagnostics/Severity.hpp"
#include "oor/base/diagnostics/SourceLocation.hpp"

#include <string>

namespace oor::base
{

// One problem found while reading a file. `code` is a stable short identifier ("deck.duplicate-id") that tests and
// users can match on; `message` is for humans and says what was expected, what was found, and where.
struct Diagnostic
{
    Severity severity = Severity::Error;
    std::string code;
    std::string message;
    SourceLocation location;

    // "error[deck.duplicate-id] file:line (CARD): message"
    [[nodiscard]] std::string toString() const;
};

} // namespace oor::base
