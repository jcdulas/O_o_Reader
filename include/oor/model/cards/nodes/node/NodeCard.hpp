#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"

#include <cstdint>

namespace oor::model
{

// One GRID card: a geometric grid point.
//
// Card layout:
//   GRID  ID  CP  X1  X2  X3  CD  PS  SEID
//
// A GRDSET card gives the defaults of CP, CD, PS and SEID for the GRIDs whose field is blank (not zero), wherever it is
// in the deck: the reader notes which fields were blank in `blankFields` and applies the defaults once the whole Bulk
// Data is read (NodeTable::finishRead). The fields stay noted while no GRDSET exists, so that one added later gives
// them their defaults (Model::applyDefaults); a GRID made in memory notes the fields its maker leaves out.
struct NodeCard
{
    static constexpr std::uint8_t BLANK_POSITION_FRAME = 1;
    static constexpr std::uint8_t BLANK_DISPLACEMENT_FRAME = 2;
    static constexpr std::uint8_t BLANK_PERMANENT_CONSTRAINTS = 4;
    static constexpr std::uint8_t BLANK_SUPERELEMENT = 8;

    base::EntityId id = 0;
    base::EntityId positionFrameId = 0;     // coordinate system of the location (0 = basic)
    base::Vec3 xyz;                         // X1 X2 X3 in the system CP
    base::EntityId displacementFrameId = 0; // coordinate system of the displacements (0 = basic)
    std::int32_t permanentConstraints = 0;  // permanent single-point constraints, as digits 1..6 (0 = none)
    base::EntityId superelementId = 0;

    // The BLANK_* bits of the fields that are blank (left out of the card, not zero), until the GRDSET defaults are
    // applied. It is not written; setting a field clears its bit (the Python setters do), so that a default added
    // later never replaces a value that was set.
    std::uint8_t blankFields = 0;
};

} // namespace oor::model
