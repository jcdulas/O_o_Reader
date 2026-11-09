#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The time unit of the repeat counts of a sequence in a random vibration analysis (the TUNIT field of SOL 108 and 111).
enum class FatigueSequenceTimeUnit : std::uint8_t
{
    Secs,
    Mins,
    Hrs,
    Days,
    Wks,
    Mths,
    Yrs
};
inline constexpr std::array<std::string_view, 7> FATIGUE_SEQUENCE_TIME_UNIT_WORDS{
    "TimeInSeconds", "TimeInMinutes", "TimeInHours", "TimeInDays", "TimeInWeeks", "TimeInMonths", "TimeInYears"};

// The longest name of an equivalent unit: fields 4 to 9 of the line, 48 characters.

// An event or a sequence of a sequence and how many times it is repeated (FIDi and Ni).
struct FatigueSequenceItem
{
    base::EntityId eventOrSequenceId =
        0;                // an FTGEVNT or another FTGSEQ (in SOL 112 a subcase number, which is not a card)
    double repeats = 1.0; // Ni: the repeats (SOL 101, 103, 112) or the duration in TUNIT units (SOL 108, 111)

    [[nodiscard]] bool operator==(const FatigueSequenceItem&) const noexcept = default;
};

// The "units" line: the equivalent unit the life is reported in.
struct FatigueSequenceUnits
{
    double equivalentUnits = 1.0; // the number of equivalent units of one repeat
    std::string name;             // EQNAME, empty when blank (the solver then writes "Repeats")

    [[nodiscard]] bool operator==(const FatigueSequenceUnits&) const noexcept = default;
};

// One FTGSEQ card: the sequence of loading events of a fatigue analysis (pseudo-static, modal, modal transient or
// random vibration), each with the number of times it is repeated, and possibly other sequences.
//
// Card layout, SOL 101, 103, 112:  FTGSEQ  ID  EVNTOUT  METHOD                          fields 1 to 3
// Card layout, SOL 108, 111:       FTGSEQ  ID  EVNTOUT  (blank)  TUNIT  LDM            fields 1 to 5
// then, for both:                         FID1  N1  FID2  N2  FID3  N3  FID4  N4       four pairs on each line, from
//                                                                                      field 2 of the continuation line
//                                         UNITS  EQUIV  EQNAME                          the last line, optional
// The two formats share the card name and the fields ID and EVNTOUT; they differ in METHOD (the third field) against
// TUNIT and LDM (the fourth and fifth), and the card reads both at once: a card with a METHOD other than 0 together
// with a TUNIT other than SECS or an LDM other than 1.0 is refused (check), and otherwise the values the card holds are
// all defaults of the format it is not. The pairs of the two formats are the same, and so is the UNITS line.
//
// The defaults are static (EVNTOUT 0, METHOD 0, TUNIT SECS, LDM 1.0, a blank Ni is 1.0) and stored resolved; the writer
// leaves them blank. At least one pair is needed. The ID is unique among the FTGSEQ and FTGEVNT cards together
// (each type is its own table, IdSpace::PerCardType, the category FatigueEvent, and the check
// `compat.fatigue-event-id-repeated` reports a clash); a FIDi names an event or a sequence of that category.
struct FatigueSequenceCard
{
    base::EntityId id = 0;
    int eventOutput = 0; // 1 asks the fatigue output of each event on its own
    int combinationMethod =
        0; // SOL 101, 103, 112: 0 independent, 1 combined full, 2 combined fast (only 0 is supported)
    FatigueSequenceTimeUnit timeUnit = FatigueSequenceTimeUnit::Secs; // SOL 108, 111
    double referenceLoadMagnitude = 1.0; // SOL 108, 111: the magnitude of the FE load of the transfer function
    std::vector<FatigueSequenceItem> items;
    std::optional<FatigueSequenceUnits> units;
};

} // namespace oor::model
