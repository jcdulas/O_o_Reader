#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/solution_control/rotordynamicParams/RotordynamicParamsCard.hpp"

#include <cstdint>

namespace oor::model
{

// The SPTID of an RSPINR: a real is a constant relative spin rate, an integer is the number of a DDVAL card that lists
// the rates (the format of DDVAL names the SPTID of an RSPINR among the fields that take a DDVAL; the format of RSPINR
// itself takes SPTID as a table of relative spin rates, or as a constant rate when it is a real).
struct RelativeSpinRate
{
    double constant = 0.0;    // the rate, when the field is a real
    base::EntityId table = 0; // the DDVAL, when the field is an integer (0 when it is a real)

    [[nodiscard]] bool isTable() const noexcept { return table != 0; }
    [[nodiscard]] bool operator==(const RelativeSpinRate&) const noexcept = default;
};

// One RSPINR card: the spin rate of a rotor relative to the others, for a complex eigenvalue, a frequency response or a
// static analysis, and the damping of the rotor.
//
// Card layout: RSPINR  ROTORID  GRIDA  GRIDB  SPDUNIT  SPTID  ROTRSEID
//                     GR  ALPHAR1  ALPHAR2  WR3R  WR4R  WRHR            fields 9 to 14 (the second line)
// The rotor turns about the axis from GRIDA to GRIDB, in the unit SPDUNIT (RPM or FREQ, the unit RGYRO already has).
// ROTRSEID is the superelement in which the rotor is defined (0, the residual structure, when blank): the rotor and
// the two grid points are looked up in the model only then, since a rotor of another superelement is a card of its
// part. GR is the structural damping of the rotor, ALPHAR1 and ALPHAR2 the Rayleigh scale factors of its mass and
// stiffness matrices, and WR3R, WR4R and WRHR the "average" excitation frequencies, in radians per unit time, of the
// damping of the GR field, of the materials and of the ROTHYBD; all six are 0 when blank (a default that does not
// depend on another field: stored as 0 and written as blanks). The pair ROTORID and ROTRSEID is unique among the
// RSPINR cards (the check `solution.rotor-spin-repeated`). The card has no ID of its own, so there may be
// several of them (IdSpace::None).
struct RotorRelativeSpinCard
{
    base::EntityId rotorId = 0;
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    RotorSpeedUnit speedUnit = RotorSpeedUnit::Rpm;
    RelativeSpinRate spinTableId;
    std::int32_t rotorSuperelementId = 0;
    double node = 0.0;
    double massDampingFactor = 0.0;
    double stiffnessDampingFactor = 0.0;
    double viscousDampingFrequency = 0.0;
    double structuralDampingFrequency = 0.0;
    double hybridDampingFrequency = 0.0;

    [[nodiscard]] bool operator==(const RotorRelativeSpinCard&) const noexcept = default;
};

} // namespace oor::model
