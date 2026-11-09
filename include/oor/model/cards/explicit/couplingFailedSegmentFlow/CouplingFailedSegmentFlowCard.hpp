#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One COUP1FL card (SOL 700): the state of the surroundings that flows through a segment of a coupling surface after
// the segment fails (the structure that carries it breaks). A COUPLE names it in its COUP1FL field. It is used with the
// fast coupling algorithm in its failure form (DYPARAM,FASTCOUP,,FAIL).
//
// Card layout: COUP1FL  CFID  RHO  SIE  XVEL  YVEL  ZVEL  PRESSURE  MATERIAL            fields 1 to 8
// RHO, SIE, the three velocities and PRESSURE are the surrounding density, specific internal energy, velocity
// components and pressure. Each may be blank, and none has a default on the card: the solver takes the reference
// density of the MATDEUL for a blank RHO and zero for the others (the Roe solver wants at least one of
// them and a blank PRESSURE), so a blank is kept as a blank (None in Python). MATERIAL is the MATDEUL of the
// multi-material Euler solver, 0 when blank. The format reference describes ZVEL as the "x-velocity" in its
// description; it is the z component.
struct CouplingFailedSegmentFlowCard
{
    base::EntityId id = 0;         // unique among the COUP1FL cards
    std::optional<double> density; // greater than zero when given
    std::optional<double> specificEnergy;
    std::optional<double> xVelocity;
    std::optional<double> yVelocity;
    std::optional<double> zVelocity;
    std::optional<double> pressure;
    base::EntityId matId = 0; // a MATDEUL, 0 when blank
};

} // namespace oor::model
