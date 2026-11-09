#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/DesignField.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// The DLINK line of a TOMVAR: the thickness of a ply as a linear function of the thickness of a ply of another TOMVAR
// (the TOMVAR `tableId`, of the same PID): this variable = constant0 + coefficient1 * (that one).
struct DesignTopometryRegionLink
{
    base::EntityId tableId = 0; // the TOMVAR it follows
    double constant0 = 0.0;
    double coefficient1 = 0.0;

    [[nodiscard]] bool operator==(const DesignTopometryRegionLink&) const noexcept = default;
};

// The SYM line of a TOMVAR: the planes of mirror symmetry (MS1 to MS3: XY, YZ or ZX), the axis of cyclic symmetry (CS:
// X, Y or Z) with its number of segments in 360 degrees (NCS), all in the coordinate system CID, and the other
// properties that are symmetric to the PID (SYMPIDn, which go on over the lines after the first).
struct DesignTopometryRegionSymmetry
{
    base::EntityId frameId = 0;              // blank: 0, the basic system
    std::array<std::string, 3> mirrorPlanes; // MS1, MS2, MS3; empty when blank
    std::string cyclicAxis;                  // CS; empty when blank
    std::int32_t cyclicSegments = 0;         // NCS; 0 when blank
    std::vector<base::EntityId> symmetricProperties;

    [[nodiscard]] bool operator==(const DesignTopometryRegionSymmetry&) const noexcept = default;
};

// One TOMVAR card: a design region of a topometry optimisation, in which the thickness or another property value of
// every element of one property is a design variable of its own.
//
// Card layout: TOMVAR  ID  TYPE  PID  PNAME/FID  XINIT  XLB  XUB  DELXV                 fields 1 to 8
//                     DLINK  TID  C0  C1                                               each of these lines is optional,
//                     DDVAL  DSVID                                                     once, and starts with its
//                     keyword STRESS  STLIM                                                    (the first field of the
//                     line: SYM  CID  MS1  MS2  MS3  CS  NCS  SYMPID1  SYMPID2  ...           field 9, 17, ...)
// TYPE is the property card (PBAR, PSHELL, PSOLID, PCOMP, ...), PID the property, which no other TOMVAR, TOPVAR or
// DVPREL may name. The target field is an exact neutral descriptor, such as "thickness"; numeric source-slot selectors
// are not accepted. XINIT, the initial value, has no default (it
// may be the value the model has: the card does not know it). A blank XLB is 0.5 times XINIT and a blank XUB 1.5 times
// XINIT, defaults that depend on another field: they are stored resolved when XINIT is given (`resolveDefaultBounds`)
// and always written, and stay blank when XINIT is blank. DELXV is 0.5 when blank, a default that does not depend on
// another field: stored as it is and written as a blank. The lines are read in any order and written in the order
// DLINK, DDVAL, STRESS, SYM. What the format asks of a line (DLINK only for TYPE PCOMP, STRESS only for PSHELL and T,
// SYM only for PSHELL and PSOLID) is not checked by the card.
struct DesignTopometryRegionCard
{
    static constexpr double DEFAULT_MOVE_LIMIT = 0.5;

    base::EntityId id = 0; // unique among the TOMVAR cards
    std::string kind;      // the neutral name of the property card type ("ShellProp"), or the word in upper case
    base::EntityId propId = 0;
    DesignField field;
    std::optional<double> initialValue;
    std::optional<double> lowerBound;
    std::optional<double> upperBound;
    double designVarMoveLimit = DEFAULT_MOVE_LIMIT;
    std::optional<DesignTopometryRegionLink> link;
    base::EntityId discreteValuesId =
        0; // the DDVAL line: the DDVAL of the discrete values; 0 when the card has no such line
    std::optional<double> stressLimit; // STLIM of the STRESS line
    std::optional<DesignTopometryRegionSymmetry> symmetry;

    // Gives a blank XLB and XUB the values the format says: 0.5 and 1.5 times XINIT. Nothing changes without an XINIT.
    void resolveDefaultBounds() noexcept
    {
        if (!initialValue)
            return;
        if (!lowerBound)
            lowerBound = 0.5 * *initialValue;
        if (!upperBound)
            upperBound = 1.5 * *initialValue;
    }

    [[nodiscard]] bool operator==(const DesignTopometryRegionCard&) const noexcept = default;
};

} // namespace oor::model
