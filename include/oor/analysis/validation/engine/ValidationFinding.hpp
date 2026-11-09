#pragma once

#include "oor/base/diagnostics/Severity.hpp"
#include "oor/base/ids/CardKey.hpp"
#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace oor::analysis
{

// One thing the validation found, as a record: what (a stable code and a message), how serious, in which family of
// checks, and where (the card by name, ID and key, the field, the part of the deck).
struct ValidationFinding
{
    base::Severity severity = base::Severity::Error;
    // Stable identifier of the check ("ref.unresolved"): what scripts and the `select`/`ignore` lists match on.
    std::string code;
    // The family of the check ("references", "constraints", ...).
    std::string family;
    std::string message;
    // The card the finding is about; a Case Control command or the Executive Control give their statement here
    // (SUBCASE, SOL), a finding about the model as a whole leaves it empty.
    std::string cardName;
    base::EntityId cardId = 0; // the ID of the card (the set ID of a grouped card); 0 when it has none
    std::uint32_t cardKey = base::CardKey::INVALID_VALUE; // the card key; invalid when no card is concerned
    std::string field;                                    // the field of the card, when one is at fault
    std::string suggestion;                               // what to do about it, when the check knows
    std::string part; // empty for the main model; a neutral part label such as SuperelementPart 1

    [[nodiscard]] bool hasCardKey() const noexcept { return cardKey != base::CardKey::INVALID_VALUE; }
    [[nodiscard]] std::string toString() const;
};

// What a validation returns: the findings in their final order (errors first, then by part, family, card key).
struct ValidationReport
{
    std::vector<ValidationFinding> findings;

    [[nodiscard]] std::size_t count(base::Severity severity) const noexcept;
    [[nodiscard]] std::size_t errorCount() const noexcept { return count(base::Severity::Error); }
    [[nodiscard]] std::size_t warningCount() const noexcept { return count(base::Severity::Warning); }
    [[nodiscard]] std::size_t infoCount() const noexcept { return count(base::Severity::Info); }
};

} // namespace oor::analysis
