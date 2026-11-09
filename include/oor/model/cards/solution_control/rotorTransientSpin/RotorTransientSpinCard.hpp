#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/solution_control/rotordynamicParams/RotordynamicParamsCard.hpp"

#include <cstdint>

namespace oor::model
{

// One RSPINT card: the spin rate of a rotor in a transient analysis, and the damping of the rotor.
//
// Card layout: RSPINT  ROTORID  GRIDA  GRIDB  SPDUNIT  SPTID  SPDOUT  ROTRSEID
//                     GR  ALPHAR1  ALPHAR2  WR3R  WR4R  WRHR            fields 9 to 14 (the second line)
// The rotor turns about the axis from GRIDA to GRIDB, in the unit SPDUNIT (RPM or FREQ, the unit RGYRO already has),
// at the rate that the TABLED1 SPTID gives over time. SPDOUT is an optional extra point (an EPOINT) that
// receives the speed of the rotor over time, in SPDUNIT; it is 0 when blank. ROTRSEID is the superelement in which the
// rotor is defined (0, the residual structure, when blank): the rotor and the two grid points are looked up in the
// model only then, since a rotor of another superelement is a card of its part. The six damping fields mean what they
// do on the RSPINR (GR, ALPHAR1, ALPHAR2, WR3R, WR4R, WRHR; each 0 when blank, a default that does not depend on
// another field: stored as 0 and written as blanks). The pair ROTORID and ROTRSEID is unique among the RSPINT cards
// (the check `solution.rotor-spin-repeated`). The card has no ID of its own (IdSpace::None).
struct RotorTransientSpinCard
{
    base::EntityId rotorId = 0;
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    RotorSpeedUnit speedUnit = RotorSpeedUnit::Rpm;
    base::EntityId spinTableId = 0; // the TABLED1 of the spin rate over time
    base::EntityId speedOutput = 0;
    std::int32_t rotorSuperelementId = 0;
    double node = 0.0;
    double massDampingFactor = 0.0;
    double stiffnessDampingFactor = 0.0;
    double viscousDampingFrequency = 0.0;
    double structuralDampingFrequency = 0.0;
    double hybridDampingFrequency = 0.0;

    [[nodiscard]] bool operator==(const RotorTransientSpinCard&) const noexcept = default;
};

} // namespace oor::model
