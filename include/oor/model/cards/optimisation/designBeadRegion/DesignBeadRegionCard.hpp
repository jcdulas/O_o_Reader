#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace oor::model
{

// BF of a BEADVAR: whether a buffer zone separates the beads of the design region from the elements outside it.
enum class BeadBuffer : std::uint8_t
{
    Yes, // YES: the default
    No   // NO
};

inline constexpr std::array<std::string_view, 2> BEAD_BUFFER_WORDS{"Yes", "No"};

// SKIP of a BEADVAR: which nodes of the elements of the property are left out of the design region.
enum class BeadSkip : std::uint8_t
{
    Bc,   // BC: the nodes of SPC and SPC1
    Load, // LOAD: the nodes of FORCE, FORCE1, FORCE2, MOMENT, MOMENT1, MOMENT2 and SPCD
    Both, // BOTH: either of them (the default)
    None  // NONE: none: every node of the elements of the property is in the region
};

inline constexpr std::array<std::string_view, 4> BEAD_SKIP_WORDS{"BoundaryCondition", "Load", "Both", "None"};

inline constexpr std::array<std::string_view, 4> DESIGN_BEAD_REGION_PROPERTY_TYPES{"ShellProp", "ShearPanelProp",
                                                                                   "CompProp", "CompPropGlobal"};

// The DESVAR line of a BEADVAR: how the bead design variables, which the solver generates, move the grid points.
struct BeadDesignLine
{
    static constexpr double DEFAULT_LOWER_BOUND = 0.0;
    static constexpr double DEFAULT_UPPER_BOUND = 1.0;
    static constexpr double DEFAULT_MOVE_LIMIT = 0.2;

    // The draw direction (XD, YD, ZD) in the system CID; empty is NORM, the normals of the elements (what a blank field
    // means too).
    std::optional<std::array<double, 3>> direction;
    base::EntityId frameId = 0; // the coordinate system of the direction; 0 is the basic system
    double lowerBound = DEFAULT_LOWER_BOUND;
    double upperBound = DEFAULT_UPPER_BOUND;
    double designVarMoveLimit = DEFAULT_MOVE_LIMIT;

    [[nodiscard]] bool operator==(const BeadDesignLine&) const noexcept = default;
};

// The GRID line of a BEADVAR: the SET1 of the grid points that are taken out of the design regions (NGSET) and the SET1
// of those that are added to them (DGSET); either is 0 when blank.
struct BeadGridLine
{
    base::EntityId excludedNodeSetId = 0;
    base::EntityId addedNodeSetId = 0;

    [[nodiscard]] bool operator==(const BeadGridLine&) const noexcept = default;
};

// One BEADVAR card: a design region of a topography (bead or stamp) optimisation: the nodes of the elements of one
// property, which the beads of the given size move.
//
// Card layout: BEADVAR  ID  PTYPE  PID  MW  MH  ANG  BF  SKIP                          fields 1 to 8
//                      DESVAR  NORM/XD  YD  ZD  CID  XLB  XUB  DELXV                  each of these lines is optional,
//                      GRID  NGSET  DGSET                                              once, and starts with its
//                      keyword
//                                                                                      (the first field of the line)
// PTYPE is PSHELL, PSHEAR, PCOMP or PCOMPG (the format lists these four) and PID the property. MW, the minimum width of
// a bead, and MH, the maximum height (the height when XUB is 1), are positive; ANG, the draw angle in degrees, is
// between 0 and 90. BF is YES when blank, SKIP is BOTH when blank: defaults that do not depend on another field, stored
// as they are and written as blanks. A DESVAR line with the direction left blank or NORM moves the grids along the
// normals of the elements, and with XD, YD and ZD (all three) along that vector; XLB is 0 and XUB 1 when blank (XLB
// below XUB, and 1 for XUB sets the height of the beads at MH), DELXV 0.2, and CID the basic system. The lines are read
// in either order and written in the order DESVAR, GRID. The ID is unique among the BEADVAR cards (several of them may
// be in a model).
struct DesignBeadRegionCard
{
    base::EntityId id = 0;
    std::string propType; // the neutral name of the property card ("ShellProp")
    base::EntityId propId = 0;
    double minWidth = 0.0;
    double maxHeight = 0.0;
    double angle = 0.0;
    BeadBuffer bufferZone = BeadBuffer::Yes;
    BeadSkip skippedNodes = BeadSkip::Both;
    std::optional<BeadDesignLine> design;
    std::optional<BeadGridLine> grids;

    [[nodiscard]] bool operator==(const DesignBeadRegionCard&) const noexcept = default;
};

} // namespace oor::model
