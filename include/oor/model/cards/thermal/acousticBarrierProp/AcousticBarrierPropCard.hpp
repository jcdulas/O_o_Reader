#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One PACBAR card: the property of a CHACBR acoustic barrier element.
//
// Card layout: PACBAR  PID  MBACK  MSEPTM  FRESON  KRESON
// MBACK and MSEPTM are the masses per unit area of the backing and of the septum. Exactly one of FRESON (the resonant
// frequency of the sandwich in hertz) and KRESON (its resonant stiffness) is given.
struct AcousticBarrierPropCard
{
    base::EntityId id = 0;
    double backingMass = 0.0;
    double septumMass = 0.0;
    std::optional<double> resonanceFrequency;
    std::optional<double> resonanceStiffness;
};

} // namespace oor::model
