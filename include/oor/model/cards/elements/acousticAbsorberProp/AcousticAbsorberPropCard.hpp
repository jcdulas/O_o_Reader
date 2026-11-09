#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PAABSF card: the property of a CAABSF frequency-dependent acoustic absorber element.
//
// Card layout: PAABSF  PID  TZREID  TZIMID  S  A  B  K  RHOC
// TZREID and TZIMID are the TABLEDi of the resistance and the reactance as functions of frequency (0 when blank). S, A
// and RHOC are 1.0 when blank, B and K 0.0; at least one of TZREID, TZIMID, B and K must be given.
struct AcousticAbsorberPropCard
{
    base::EntityId id = 0;
    base::EntityId resistanceTableId = 0;
    base::EntityId reactanceTableId = 0;
    double impedanceScale = 1.0;
    double area = 1.0;
    double damping = 0.0;
    double stiffness = 0.0;
    double characteristicImpedance = 1.0;
};

} // namespace oor::model
