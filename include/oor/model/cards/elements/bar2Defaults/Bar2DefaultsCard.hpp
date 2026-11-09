#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <string>
#include <string_view>

namespace oor::model
{

// One BAROR card: the default PID, orientation and OFFT of the CBAR cards that leave them blank.
//
// Card layout (orientation by vector, or by a grid point in field 5 with fields 6 and 7 blank; fields 1, 3 and 4 are
// blank):
//   BAROR       PID            X1   X2   X3   OFFT
//   BAROR       PID            G0             OFFT
//
// The defaults are applied when a deck is read, to the blank PID, orientation and OFFT of every CBAR, wherever the
// BAROR is in the deck (Bar2Table::finishRead): a blank PID is `propId` (EID when the BAROR gives none), a blank
// orientation is the grid point or the vector of the BAROR, a blank OFFT is `offsetKind`. A BAROR added to a model
// afterwards gives them with Model::applyDefaults (Model.apply_defaults in Python), to the CBARs that still note blank
// fields (those read before it existed, and the CBARs added from Python without a PID). The format reference gives no
// default for the pin flags, so there is none. Only one BAROR may appear.
struct Bar2DefaultsCard
{
    static constexpr std::string_view DEFAULT_OFFSET_KIND = "OffsetsGlobalGlobalGlobal";

    base::EntityId propId = 0; // 0: no default PID (a blank PID stays EID)
    std::array<double, 3> orientation =
        {}; // X1 X2 X3: the default orientation vector (unused when orientationNode is given)
    base::EntityId orientationNode = 0; // the default grid point for the orientation vector; 0: the vector form
    std::string offsetKind{DEFAULT_OFFSET_KIND};

    [[nodiscard]] bool usesGridPoint() const noexcept { return orientationNode != 0; }
    // False for a BAROR that gives no orientation (all blank): the CBARs keep the zero vector.
    [[nodiscard]] bool givesOrientation() const noexcept
    {
        return orientationNode != 0 || orientation[0] != 0.0 || orientation[1] != 0.0 || orientation[2] != 0.0;
    }
};

} // namespace oor::model
