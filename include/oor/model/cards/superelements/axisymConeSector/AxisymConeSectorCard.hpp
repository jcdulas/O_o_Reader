#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One SECTAX card: a sector of a conical shell.
//
// Card layout: SECTAX  ID  RID  R  PHI1  PHI2                                       fields 1 to 5
// ID is the number of the sector. It must be unique among the POINTAX, RINGAX and SECTAX numbers: the card
// is filed under it with an ID space per card type, and `superelement.axisym-cone-sector-ring` checks it against the
// RINGAX and POINTAX cards, which share the node ID space. RID is the number of the RINGAX ring the sector belongs to
// (a node reference). R is the effective radius and PHI1 and PHI2 the azimuthal limits of the sector in degrees, all
// reals the card requires. The card is allowed only when the deck has an AXIC card (
// `compat.axisymmetric-flag-card`).
struct AxisymConeSectorCard
{
    base::EntityId id = 0;
    base::EntityId ringId = 0;
    double radius = 0.0;
    double phiAngle1 = 0.0;
    double phiAngle2 = 0.0;

    [[nodiscard]] bool operator==(const AxisymConeSectorCard&) const noexcept = default;
};

} // namespace oor::model
