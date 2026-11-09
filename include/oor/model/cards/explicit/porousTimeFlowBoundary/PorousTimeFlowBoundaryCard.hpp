#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cstdint>

namespace oor::model
{

// The four quantities of a PORFLWT that are an ELEMENT value (the value of the Euler element the surface cuts), a
// CONSTANT or a TABLE of time (a TABLED1). Each has a keyword field and a value field. A blank keyword means ELEMENT,
// the default for the velocity; for the other three the format gives no default, but says that a missing value comes
// from the adjacent Euler element, which is what ELEMENT means.
inline constexpr std::uint8_t POROUS_TIME_FLOW_ALL_SOURCES = static_cast<std::uint8_t>(
    sourceBit(ValueSource::Element) | sourceBit(ValueSource::Constant) | sourceBit(ValueSource::Table));

inline constexpr SourcedValueLayout POROUS_TIME_FLOW_VELOCITY_LAYOUT{
    9, 10, "velocitySource", "velocity", POROUS_TIME_FLOW_ALL_SOURCES, ValueSource::Element, 0.0};
inline constexpr SourcedValueLayout POROUS_TIME_FLOW_PRESSURE_LAYOUT{
    11, 12, "pressureSource", "pressure", POROUS_TIME_FLOW_ALL_SOURCES, ValueSource::Element, 0.0};
inline constexpr SourcedValueLayout POROUS_TIME_FLOW_DENSITY_LAYOUT{
    18, 19, "densitySource", "density", POROUS_TIME_FLOW_ALL_SOURCES, ValueSource::Element, 0.0};
// The format spells the keyword both SIETYP and SIETYPE: the card uses SIETYP, the name of the layout.
inline constexpr SourcedValueLayout POROUS_TIME_FLOW_ENERGY_LAYOUT{
    20, 21, "energySource", "specificEnergy", POROUS_TIME_FLOW_ALL_SOURCES, ValueSource::Element, 0.0};

// One PORFLWT card (SOL 700): a flow boundary through a porous part of a coupling surface whose velocity, pressure,
// density and energy vary with time. A LEAKAGE of PORTYPE PORFLWT names it; the surface can only be a general coupling
// surface. A quantity that is not given is that of the Euler element the surface cuts.
//
// Card layout: PORFLWT  FID  TYPE                                    fields 1 and 2 (fields 3 to 8 are empty cells)
//                      VELTYPE  VELOCITY  PRESTYP  PRES              fields 9 to 12
//                      MID  DENSTYP  DENSITY  SIETYP  SIE           fields 17 to 21
// TYPE is the kind of boundary: IN (only inflow), OUT (only outflow) or BOTH, BOTH when blank. VELOCITY is the velocity
// normal to the surface (positive is inflow), PRES the pressure, DENSITY and SIE the density and specific internal
// energy of the inflowing material; each is the number or the TABLED1 that its keyword says (ELEMENT takes no value).
// MID is the MATDEUL of the inflowing material: it is not allowed for TYPE OUT, and when it is given the density and
// the energy of the material must be given too, so the card then reads DENSTYP, SIETYP and, for a constant,
// DENSITY and SIE as required and writes them out even where they equal the default.
struct PorousTimeFlowBoundaryCard
{
    base::EntityId id = 0; // unique among the PORFLWT cards
    FlowDirection type = FlowDirection::Both;
    SourcedValue velocity{ValueSource::Element, 0.0, 0};
    SourcedValue pressure{ValueSource::Element, 0.0, 0};
    base::EntityId matId = 0; // a MATDEUL, 0 when blank
    SourcedValue density{ValueSource::Element, 0.0, 0};
    SourcedValue energy{ValueSource::Element, 0.0, 0};
};

} // namespace oor::model
