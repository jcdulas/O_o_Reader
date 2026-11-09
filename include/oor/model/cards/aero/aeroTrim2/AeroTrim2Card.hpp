#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace oor::model
{

// The words that stand for a value on a TRIM2 card: what the trim does with a variable that has no number. FREE is
// solved for; LINKED is set by an AELINK card and SCHED by a CSSCHD card (the two are a convenience: the solver finds
// the linked and scheduled variables from those cards itself).
enum class TrimState : std::uint8_t
{
    Free,
    Linked,
    Scheduled
};

// The words of the format reference, in the order of the enumerators (the third is SCHED, not the whole word).
inline constexpr std::array<std::string_view, 3> TRIM_STATE_WORDS{"Free", "Linked", "Scheduled"};

// A trim variable of a TRIM2 card: the label of an AESTAT, AESURF or AEPARM and its value, which is a number (the
// variable is fixed to it) or one of the words of TrimState.
struct AeroTrim2Variable
{
    std::string label;
    std::variant<double, TrimState> value = 0.0;

    [[nodiscard]] bool operator==(const AeroTrim2Variable&) const noexcept = default;
};

// One TRIM2 card: the flight condition of a static aeroelastic trim and the state of its trim variables, an alternative
// to TRIM for a model with many variables.
//
// Card layout: TRIM2  SID     MACH    Q       (blank)  (blank)  (blank)  (blank)  AEQR
//                    LABEL1  VALUE1  LABEL2  VALUE2   -etc.-
// The first line holds SID, MACH and Q in fields 1 to 3 and AEQR in field 8; fields 4 to 7 are blank in the drawing of
// the format and the card refuses a value there. The pairs
// of a label and a value start on the continuation line, in field 9, two fields to a pair, four pairs to a line; there
// may be none. A TRIM card is laid out differently (it has pairs in fields 4 to 7), so a TRIM written as a TRIM2 is
// refused. A variable that no pair names is fixed to zero.
//
// The Case Control TRIM selects the card by its SID, which is unique among all the TRIM and TRIM2 cards:
// the two types share the category AeroTrimSet and a check between cards reports a SID that both have. MACH is not
// negative and not 1.0 (Doublet-Lattice below 1.0, ZONA51 above it), Q is positive. AEQR is 1.0 for the elastic trim
// (blank), 0.0 for the rigid trim; values between have no physical meaning but are allowed. The labels name a variable
// by its LABEL, not by its ID, so a renumbering does not touch them; they are kept as written and compared without
// regard to the case. A variable may be given once.
struct AeroTrim2Card
{
    base::EntityId id = 0;
    double machNumber = 0.0;
    double dynamicPressure = 0.0;
    double equivalentAspectRatio = 1.0;
    std::vector<AeroTrim2Variable> variables;

    [[nodiscard]] bool operator==(const AeroTrim2Card&) const noexcept = default;
};

} // namespace oor::model
