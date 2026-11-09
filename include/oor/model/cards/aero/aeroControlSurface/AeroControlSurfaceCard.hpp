#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>

namespace oor::model
{

// The LDW field: whether the downwash caused by a unit deflection of the surface is computed by the program (LDW, the
// default) or has to be given by the user with AEDW, AEFORCE and AEPRESS cards.
enum class AeroControlSurfaceDownwash : std::uint8_t
{
    Computed, // LDW
    Given     // NOLDW
};

// The words of the format reference, in the order of the enumerators.
inline constexpr std::array<std::string_view, 2> AERO_CONTROL_SURFACE_DOWNWASH_WORDS{"LinearDownwash",
                                                                                     "NoLinearDownwash"};

// One AESURF card: an aerodynamic control surface, a variable of the trim that rotates up to two groups of aerodynamic
// boxes about their hinge lines.
//
// Card layout: AESURF  ID     LABEL  CID1   ALID1  CID2   ALID2  EFF    LDW
//                     CREFC  CREFS  PLLIM  PULIM  HMLLIM HMULIM TQLLIM TQULIM
// The first component is the pair CID1 (a rectangular coordinate system whose y axis is the hinge line) and ALID1 (the
// AELIST of the boxes that rotate about it); the second component, CID2 and ALID2, is optional, so the
// two are both given or both left out. A deck may write CID2 as 0 with no ALID2: that is no
// second component, like a blank. The continuation line is optional.
//
// LABEL names the variable: a TRIM, an AELINK or a UXVEC finds it by the label (compared without regard to the case,
// the label is kept as written), and a CSSCHD by the ID (the ID is otherwise ignored by the solver). EFF is the
// effectiveness of the surface (any number but 0.0, default 1.0), LDW the downwash word, CREFC and CREFS the reference
// chord and area of the hinge moment coefficients (positive, default 1.0).
//
// The deflection limits are in radians. PLLIM and PULIM default to minus and plus a quarter turn (pi/2): the card keeps
// them resolved, so a blank field and the default are the same card and a blank is written whenever a limit is the
// default. HMLLIM and HMULIM, the limits of the hinge moment, have no default (no limit when blank). TQLLIM and TQULIM
// name a TABLED1 each that gives the lower and the upper deflection limit as a function of the dynamic pressure (0 when
// blank: no table). A limit is a constant or a table, not both: a table may be given for a side only
// when the constant of that side is still the default. The check is made side by side (TQLLIM against PLLIM, TQULIM
// against PULIM): the format reference does not say whether a lower limit from a table may go with an upper constant,
// and the stricter reading would refuse a combination the solver may accept. A lower limit above the upper one is not
// refused either (no rule of the format reference says it).
struct AeroControlSurfaceCard
{
    static constexpr double DEFAULT_EFFECTIVENESS = 1.0;
    static constexpr double DEFAULT_REFERENCE_CHORD = 1.0;
    static constexpr double DEFAULT_REFERENCE_AREA = 1.0;
    static constexpr double DEFAULT_DEFLECTION_LOWER_LIMIT = -std::numbers::pi / 2.0;
    static constexpr double DEFAULT_DEFLECTION_UPPER_LIMIT = std::numbers::pi / 2.0;

    base::EntityId id = 0;
    std::string label; // up to eight characters
    base::EntityId frameId1 = 0;
    base::EntityId boxListId1 = 0;
    base::EntityId frameId2 = 0;   // 0 when blank (or written 0): no second component
    base::EntityId boxListId2 = 0; // 0 when blank
    double effectiveness = DEFAULT_EFFECTIVENESS;
    AeroControlSurfaceDownwash linearDownwash = AeroControlSurfaceDownwash::Computed;
    double referenceChord = DEFAULT_REFERENCE_CHORD;
    double referenceArea = DEFAULT_REFERENCE_AREA;
    double deflectionLowerLimit = DEFAULT_DEFLECTION_LOWER_LIMIT;
    double deflectionUpperLimit = DEFAULT_DEFLECTION_UPPER_LIMIT;
    std::optional<double> hingeMomentLowerLimit; // no limit when blank
    std::optional<double> hingeMomentUpperLimit;
    base::EntityId lowerLimitTableId = 0; // 0 when blank: the lower limit is the constant PLLIM
    base::EntityId upperLimitTableId = 0;
};

} // namespace oor::model
