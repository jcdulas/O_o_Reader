#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One PAERO1 card: the property of the CAERO1 panels that name it, which lists the aerodynamic bodies (CAERO2 cards)
// that interact with them in the Doublet-Lattice method.
//
// Card layout: PAERO1  PID  B1  B2  B3  B4  B5  B6
// Each Bi is the EID of a CAERO2. The bodies must be in the interference group of the panels (the IGID of the
// CAERO2), and each must be a CAERO2 of the model; both are rules between cards, not checked here.
// The card is required even for a panel with no bodies: all the Bi are then blank, which the card keeps as
// zeros. A blank may not sit between two bodies: the format reference forbids embedded blanks, so the bodies are the
// first fields.
struct AeroLiftingPanelProp1Card
{
    static constexpr std::size_t BODY_COUNT = 6;

    base::EntityId id = 0;
    std::array<base::EntityId, BODY_COUNT> bodies = {}; // B1 to B6: the CAERO2 bodies, 0 where blank
};

} // namespace oor::model
