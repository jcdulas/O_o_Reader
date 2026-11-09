#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace oor::model
{

// One CBAR card: a simple beam element.
//
// Card layout (orientation by vector, or by a grid point in field 5 with fields 6 and 7 blank):
//   CBAR  EID  PID  GA   GB   X1   X2   X3   OFFT
//         PA   PB   W1A  W2A  W3A  W1B  W2B  W3B
//   CBAR  EID  PID  GA   GB   G0             OFFT
//
// Field 5 is an integer (G0) or a real (X1): `orientationNode` and `orientation` are the two forms, `orientationNode ==
// 0` meaning the vector form. In the vector form a blank X1, X2 or X3 is 0.0. A BAROR card gives the defaults of PID,
// the orientation and OFFT for the blank fields: they are applied when a deck is read (Bar2Table::finishRead),
// wherever the BAROR is in the deck. Without a BAROR, a blank PID is EID and a blank orientation is the zero vector.
struct Bar2Card
{
    static constexpr std::string_view DEFAULT_OFFSET_KIND = "OffsetsGlobalGlobalGlobal";
    // The bits of `blankFields`: the fields that were blank in the deck, which a BAROR card may give a default.
    static constexpr std::uint8_t BLANK_PROP_ID = 1;
    static constexpr std::uint8_t BLANK_ORIENTATION1 = 2; // field 5, X1 or G0
    static constexpr std::uint8_t BLANK_ORIENTATION2 = 4;
    static constexpr std::uint8_t BLANK_ORIENTATION3 = 8;
    static constexpr std::uint8_t BLANK_OFFSET_KIND = 16;

    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;

    std::array<double, 3> orientation =
        {}; // X1 X2 X3: the orientation vector (unused when orientationNode is given; then all zero)
    base::EntityId orientationNode = 0;          // the grid point that gives the orientation vector; 0: the vector form
    std::string offsetKind{DEFAULT_OFFSET_KIND}; // how offsets and the orientation vector are interpreted:
                                                 // a word of BAR_OFFSET_KIND_WORDS (or an old code: "1", "2")

    std::int32_t pinFlagsA = 0; // pin flags at ends A and B: digits 1..6 in ascending order, 0 for none
    std::int32_t pinFlagsB = 0;
    std::array<double, 3> offsetA = {}; // W1A W2A W3A: offset vector at end A
    std::array<double, 3> offsetB = {}; // W1B W2B W3B: offset vector at end B

    // The BLANK_* bits of the fields that are blank (left out of the card, not zero), until the BAROR defaults are
    // applied: they stay while no BAROR exists, so that one added later gives them their defaults
    // (Model::applyDefaults). It is not written; setting a field clears its bit (the Python setters do).
    std::uint8_t blankFields = 0;

    [[nodiscard]] bool usesGridPoint() const noexcept { return orientationNode != 0; }
};

} // namespace oor::model
