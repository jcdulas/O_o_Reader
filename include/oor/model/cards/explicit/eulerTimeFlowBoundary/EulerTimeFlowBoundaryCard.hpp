#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/BoundaryBox.hpp"
#include "oor/model/cards/explicit/EulerFlowBoundaryProperty.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cstdint>
#include <optional>

namespace oor::model
{

// The four quantities of a FLOWT each take a keyword and a value: ELEMENT (the value of the Euler element next to the
// boundary, no value), CONSTANT (a number) or TABLE (a TABLED1 of time). A blank keyword is ELEMENT. A CONSTANT with no
// value is zero, which the format reference does not say; the card needs some number for it.
inline constexpr std::uint8_t EULER_TIME_FLOW_ALL_SOURCES = static_cast<std::uint8_t>(
    sourceBit(ValueSource::Element) | sourceBit(ValueSource::Constant) | sourceBit(ValueSource::Table));

// VELTYPE and VELOCITY (fields 17 and 18): the velocity normal to the boundary, positive for an inflow.
inline constexpr SourcedValueLayout EULER_TIME_FLOW_VELOCITY_LAYOUT{
    17, 18, "velocitySource", "velocity", EULER_TIME_FLOW_ALL_SOURCES, ValueSource::Element, 0.0};
// PRESTYP and PRES (fields 19 and 20).
inline constexpr SourcedValueLayout EULER_TIME_FLOW_PRESSURE_LAYOUT{
    19, 20, "pressureSource", "pressure", EULER_TIME_FLOW_ALL_SOURCES, ValueSource::Element, 0.0};
// DENSTYP and DENSITY (fields 26 and 27). The format names the keyword DESTYP in its description and DENSTYP in its
// grid.
inline constexpr SourcedValueLayout EULER_TIME_FLOW_DENSITY_LAYOUT{
    26, 27, "densitySource", "density", EULER_TIME_FLOW_ALL_SOURCES, ValueSource::Element, 0.0};
// SIETYPE and SIE (fields 28 and 29): the specific internal energy.
inline constexpr SourcedValueLayout EULER_TIME_FLOW_ENERGY_LAYOUT{
    28, 29, "energySource", "specificEnergy", EULER_TIME_FLOW_ALL_SOURCES, ValueSource::Element, 0.0};

// One FLOWT card (SOL 700): a flow boundary of an Euler mesh whose inflow velocity, pressure and material properties
// may depend on time. No other entry of the format reference has this name.
//
// Card layout: FLOWT  FID  BCID  TYPE  MESH  DIR                              fields 1 to 5
//             +      XMIN  XMAX  YMIN  YMAX  ZMIN  ZMAX                     fields 9 to 14
//             +      VELTYPE  VELOCITY  PRESTYP  PRES                       fields 17 to 20
//             +      MID  DENSTYP  DENSITY  SIETYPE  SIE                    fields 25 to 29
// FID numbers the card and TYPE (IN, OUT or BOTH) is required. The faces are chosen as for the other flow cards: BCID
// (a set of BCSEG cards), else DIR, else the box. MID is the MATDEUL of the material that flows in. The format's rules
// about the fields of the card: MID is not allowed for an OUT boundary; a BOTH boundary may define the pressure only
// (no velocity, no material, no density, no energy); and a material that flows in needs its density and energy, which
// the card reads as DENSTYP and SIETYPE that are not ELEMENT when MID is given (the format makes them required in that
// case). The format ties the card to a TLOAD1 through a LID that this card does not have, so the FID is taken as the
// number a TLOAD1 excites.
struct EulerTimeFlowBoundaryCard
{
    base::EntityId id = 0;            // unique among the FLOWT cards
    base::EntityId contactBodyId = 0; // a set of BCSEG cards, 0 when blank
    FlowBoundaryType type = FlowBoundaryType::In;
    base::EntityId meshId = 0; // an Euler MESH, 0 when blank
    std::optional<BoundaryFace> meshFace;
    BoundaryBox box;
    SourcedValue velocity{ValueSource::Element, 0.0, 0};
    SourcedValue pressure{ValueSource::Element, 0.0, 0};
    base::EntityId matId = 0; // a MATDEUL, 0 when blank
    SourcedValue density{ValueSource::Element, 0.0, 0};
    SourcedValue specificEnergy{ValueSource::Element, 0.0, 0};
};

} // namespace oor::model
