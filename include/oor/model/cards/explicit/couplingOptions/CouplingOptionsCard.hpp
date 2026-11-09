#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

// FACTOR and FACTORV of a COUOPT (fields 4 and 5): the format reference lists the constant interaction factor only; a
// blank FACTORV is 1.0.
inline constexpr SourcedValueLayout COUPLING_OPTIONS_FACTOR_LAYOUT{
    4, 5, "factorSource", "factor", sourceBit(ValueSource::Constant), ValueSource::Constant, 1.0};

// PLCOVER and PLCOVERV (fields 9 and 10, the first two cells of the continuation line): the pressure from the covered
// side is a constant or a table of time (a TABLED1); a blank PLCOVERV is 0.0, no pressure.
inline constexpr SourcedValueLayout COUPLING_OPTIONS_COVER_PRESSURE_LAYOUT{
    9,
    10,
    "coverPressureSource",
    "coverPressure",
    static_cast<std::uint8_t>(sourceBit(ValueSource::Constant) | sourceBit(ValueSource::Table)),
    ValueSource::Constant,
    0.0};

// One COUOPT card (SOL 700): special options of one part of the coupling surface of a COUPLE: the factor that scales
// the Eulerian pressure acting on it and a pressure load from the covered side.
//
// Card layout: COUOPT  CID      OPTID     SUBID  FACTOR  FACTORV                         fields 1 to 5
//                     PLCOVER  PLCOVERV                                                fields 9 and 10
// CID numbers the card, OPTID is the number of the set of COUOPT cards that a COUPLE names in its OPTID field (the
// cards of a set share it, so it is the ID the model files the card under, a set ID: IdSpace::Group). SUBID names the
// BSURF, BCBOX, BCPROP, BCMATL or BCSEG, a part of the surface of the COUPLE, that the options apply to; 0 or blank
// means the whole surface.
struct CouplingOptionsCard
{
    base::EntityId id = 0;           // the number of the card
    base::EntityId setId = 0;        // the number of the set, which a COUPLE names
    base::EntityId subsurfaceId = 0; // a part of the coupling surface, 0 for the whole of it
    SourcedValue factor{ValueSource::Constant, 1.0, 0};
    SourcedValue coverPressure{ValueSource::Constant, 0.0, 0};
};

} // namespace oor::model
