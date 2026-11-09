#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace oor::model
{

// One CSLOT4 card: a four-point slot element of the acoustic cavity analysis (an evenly spaced radial slot that solves
// the wave equation in two dimensions). It needs an AXSLOT card in the deck: a rule between cards, not checked here.
//
// Card layout:
//   CSLOT4  EID  IDS1  IDS2  IDS3  IDS4  RHO  B  M
//
// The four points are GRIDS slot points, which no card of the library defines yet: they share the node ID space, so
// they are visited as nodes (the reference stays unresolved until a card of that type exists). The element makes four
// plot elements, IDS1 to IDS2, IDS2 to IDS3, IDS3 to IDS4 and IDS4 to IDS1.
//
// EID is a positive integer below 100,000,000, in the element ID space shared by the structural and the fluid elements.
// RHO is the fluid density (positive), B the bulk modulus (not negative: 0.0 is an incompressible
// fluid) and M the number of slots in the circumferential direction (not negative: 0 generates no
// matrices). A blank one is given by the RHOD, BD or MD field of the AXSLOT card, which only the model can
// tell, so a blank field is nothing here and not a default.
struct AcousticSlot4Card
{
    static constexpr std::size_t NODE_COUNT = 4;

    base::EntityId id = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {}; // IDS1 to IDS4
    std::optional<double> density;                     // blank: the RHOD of the AXSLOT
    std::optional<double> bulkModulus;                 // blank: the BD of the AXSLOT
    std::optional<std::int32_t> slotCount;             // blank: the MD of the AXSLOT
};

} // namespace oor::model
