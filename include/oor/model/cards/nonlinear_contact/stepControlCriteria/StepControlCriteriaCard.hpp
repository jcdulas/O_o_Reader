#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

// ICRITi of a TABSCTL: the quantity whose increment limits (or aims at) the load step, and the kind of entity it is
// computed for. The enumerators are numbered in the order of LOAD_STEP_CRITERION_WORDS.
enum class LoadStepCriterion : std::uint8_t
{
    Esi,   // ESI: element strain increment
    Epsi,  // EPSI: element plastic strain increment
    Ecsi,  // ECSI: element creep strain increment
    Encsi, // ENCSI: element normalized creep strain increment
    Estri, // ESTRI: element stress increment
    Esri,  // ESRI: element strain energy increment
    Gti,   // GTI: grid temperature increment
    Gdi,   // GDI: grid displacement increment
    Gri,   // GRI: grid rotation increment
    Enstri // ENSTRI: element normalized stress increment
};

inline constexpr std::array<std::string_view, 10> LOAD_STEP_CRITERION_WORDS{
    "StrainIncrement",   "PlasticStrainIncrement",   "CreepStrainIncrement", "NormalizedCreepStrainIncrement",
    "StressIncrement",   "StrainEnergyIncrement",    "TemperatureIncrement", "DisplacementIncrement",
    "RotationIncrement", "NormalizedStressIncrement"};

// A criterion has up to four ranges.
inline constexpr std::size_t STEP_CONTROL_MAX_RANGES = 4;

// One range of a criterion: the limit or target value YTk and the largest total value MTk of the quantity for which
// it is used. A blank MTk is not stored (a blank, the default, keeps the range always active), and
// the last MTk used should be zero, which makes its YTk the value for all larger totals.
struct StepControlCriteriaRange
{
    double limit = 0.0;         // YTk
    std::optional<double> upTo; // MTk: the end of the range of applicability, blank when always active

    [[nodiscard]] bool operator==(const StepControlCriteriaRange&) const noexcept = default;
};

// One criterion: its kind, the SET3 that limits it to some elements or grid points (0 is ALL, the default: a blank
// field and the word ALL are the same) and its one to four ranges.
struct StepControlCriteriaCriterion
{
    LoadStepCriterion kind = LoadStepCriterion::Esi;
    base::EntityId setId = 0; // SET3_IDi: a SET3, or 0 for every element or grid point
    std::vector<StepControlCriteriaRange> ranges;

    [[nodiscard]] bool operator==(const StepControlCriteriaCriterion&) const noexcept = default;
};

// One TABSCTL card (SOL 400): the user criteria of load stepping control, which an NLSTEP names in the CRITTID of its
// ADAPT line. Used as limits the criteria reduce the step when one would be violated; used as targets they
// also increase it when the calculated results are below them.
//
// Card layout: TABSCTL  TID                                                         fields 1 of the first line
//                      ICRIT_1  SET3_ID1  YT1_1  MT1_1  YT2_1  MT2_1  YT3_1  MT3_1   fields 9 to 16 of the next line
//                      YT4_1  MT4_1                                                 fields 1 and 2 of one more line
//                      ICRIT_2  SET3_ID2  ...                                       the next criterion, and so on
// A criterion starts at a line whose first field is a criterion word; the line that follows it holds the fourth range
// when it has one, and the criteria are placed one after the other: a criterion with no fourth range may have no second
// line (the next criterion is then on the line after it), which a layout of fixed pairs of lines would not read. So the
// card is cut into criteria at the lines that start with a criterion word (`ContactSections`), and a line without one
// carries the fourth range of the criterion above it. The writer gives a criterion with three ranges or fewer one line
// and a criterion with four two. TID is unique among the TABSCTL cards.
//
// Choices of the card: the format gives YT1 and the other YTk no default and no range (any real), so a YTk is
// required for each range and only finite numbers are checked; a range without a YTk but with an MTk is refused; the
// ranges are consecutive (a blank pair ends the list); a criterion word that is not on the format is refused.
struct StepControlCriteriaCard
{
    base::EntityId id = 0;
    std::vector<StepControlCriteriaCriterion> criteria;

    [[nodiscard]] bool operator==(const StepControlCriteriaCard&) const noexcept = default;
};

} // namespace oor::model
