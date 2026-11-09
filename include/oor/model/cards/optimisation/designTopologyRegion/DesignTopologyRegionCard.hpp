#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace oor::model
{

// The symmetry line of a TOPVAR ("SYM"): the planes of mirror symmetry (MS1 to MS3: XY, YZ or ZX) and the axis of
// cyclic symmetry (CS: X, Y or Z) with its number of segments in 360 degrees (NCS), in the coordinate system CID.
struct DesignTopologyRegionSymmetry
{
    base::EntityId frameId = 0;              // blank: 0
    std::array<std::string, 3> mirrorPlanes; // MS1, MS2, MS3; empty when blank
    std::string cyclicAxis;                  // CS; empty when blank
    std::int32_t cyclicSegments = 0;         // NCS; 0 when blank

    [[nodiscard]] bool operator==(const DesignTopologyRegionSymmetry&) const noexcept = default;
};

// The casting line ("CAST"): the draw direction of the die (DD: X, Y, Z, or with a minus sign the opposite direction),
// the number of dies (DIE: 1 when blank, or 2) and whether the mesh is aligned with the direction (ALIGN: YES, or NO
// when blank).
struct DesignTopologyRegionCasting
{
    base::EntityId frameId = 0;
    std::string direction;        // DD
    std::int32_t dies = 1;        // DIE
    std::string alignment = "No"; // ALIGN: YES or NO

    [[nodiscard]] bool operator==(const DesignTopologyRegionCasting&) const noexcept = default;
};

// The extrusion line ("EXT"): the direction of the extrusion (ED: X, Y or Z) and the alignment of the mesh.
struct DesignTopologyRegionExtrusion
{
    base::EntityId frameId = 0;
    std::string direction;        // ED
    std::string alignment = "No"; // ALIGN: YES or NO

    [[nodiscard]] bool operator==(const DesignTopologyRegionExtrusion&) const noexcept = default;
};

// The overhang line ("PRINT"): the printing direction (PD: X, Y, Z or with a minus sign the opposite).
struct DesignTopologyRegionOverhang
{
    base::EntityId frameId = 0;
    std::string direction; // PD

    [[nodiscard]] bool operator==(const DesignTopologyRegionOverhang&) const noexcept = default;
};

// The member size line ("TDMIN"): the smallest and the largest member, either of which may be blank.
struct DesignTopologyRegionMemberSize
{
    std::optional<double> minimum; // TVMIN, >= 0
    std::optional<double> maximum; // TVMAX, > TVMIN

    [[nodiscard]] bool operator==(const DesignTopologyRegionMemberSize&) const noexcept = default;
};

// One TOPVAR card: a region of a topology optimisation, the elements of one property whose density the optimisation
// designs.
//
// Card layout: TOPVAR  ID  LABEL  PTYPE  XINIT  XLB  DELXV  POWER  PID                    fields 1 to 8
//                     SYM  CID MS1 MS2 MS3 CS NCS                                         each of these lines is
//                     optional, once, CAST  CID DD DIE ALIGN                                              starts with
//                     its keyword in field 9, EXT   CID ED ALIGN                                                  and
//                     takes its values from field 10 PRINT CID PD TDMIN TVMIN TVMAX STRESS STLIM
// PTYPE is the property card (PROD, PBAR, PSHELL, PSOLID, ...) and PID the property, which no other TOPVAR, DVPREL1 or
// DVPREL2 may name (topology and sizing variables cannot share a property). XINIT, the initial density, is
// blank when the format's rule gives it (the constraint value, 0.9 or 0.6, which depend on the responses of the deck),
// DELXV when the DELX of the DOPTPRM gives it (0.2) and POWER when 3.0 (4.0 with symmetry constraints) is meant: none
// of them is resolved here, since the value depends on other cards.
struct DesignTopologyRegionCard
{
    static constexpr double DEFAULT_LOWER_BOUND = 0.001;

    base::EntityId id = 0; // unique among the TOPVAR cards
    std::string entryLabel;
    std::string propType; // the neutral name of the property card type ("SolidProp"), or the word in upper case
    std::optional<double> initialValue;
    double lowerBound = DEFAULT_LOWER_BOUND;
    std::optional<double> designVarMoveLimit;
    std::optional<double> penaltyPower;
    base::EntityId propId = 0;
    std::optional<DesignTopologyRegionSymmetry> symmetry;
    std::optional<DesignTopologyRegionCasting> casting;
    std::optional<DesignTopologyRegionExtrusion> extrusion;
    std::optional<DesignTopologyRegionOverhang> overhang;
    std::optional<DesignTopologyRegionMemberSize> memberSize;
    std::optional<double> stressLimit; // STLIM
};

} // namespace oor::model
