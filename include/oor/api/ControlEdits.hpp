#pragma once

#include "oor/api/Deck.hpp"

#include <span>

namespace oor::api
{

// A catalogue command and its separate fields. No delimiter escaping or model-file sections cross this interface.
struct ControlRecord
{
    std::string name;
    std::vector<std::string> fields;
    std::string comment;
};

// Replaces executive statements and external file assignments together, after validating every record.
void setExecutive(const Deck& deck, std::span<const ControlRecord> records);
[[nodiscard]] std::vector<ControlRecord> executiveRecords(const Deck& deck);
void setSolution(const Deck& deck, const std::string& solution);
// Complete structured Case Control, including section and output-block headers. Replacing invalidates old keys.
void setCaseControl(const Deck& deck, std::span<const ControlRecord> records);
[[nodiscard]] std::vector<ControlRecord> caseRecords(const Deck& deck);

[[nodiscard]] const model::Subcase& caseSection(const Deck& deck, std::uint32_t key);
// parentKey is zero for a top-level section; step/substep keys are scoped to their parent. Global has its own key.
std::uint32_t addCaseSection(const Deck& deck, const std::string& kind, std::int64_t id, std::uint32_t parentKey,
                             std::span<const ControlRecord> commands = {});
void renumberCaseSection(const Deck& deck, std::uint32_t key, std::int64_t id);
bool removeCaseSection(const Deck& deck, std::uint32_t key);
// Upserts ordinary commands, output requests (name/options), sets (ID) and parameters (name), all or none.
std::vector<std::uint32_t> setCaseCommands(const Deck& deck, std::uint32_t sectionKey,
                                           std::span<const ControlRecord> commands);
bool removeCaseEntry(const Deck& deck, std::uint32_t entryKey);

} // namespace oor::api
