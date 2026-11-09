#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

enum class PorosityType : std::uint8_t
{
    PorousFlow,                 // PORFLOW: a constant flow boundary
    PorousFlowTimed,            // PORFLWT: a flow boundary that varies with time
    Hole,                       // PORHOLE: small holes in an air bag
    Permeability,               // PERMEAB: permeable air bag material
    HoleBetweenGasBags,         // PORFGBG: flow through a hole to another GBAG
    PermeabilityBetweenGasBags, // PERMGBG: flow through a permeable area to another GBAG
    HoleBetweenCouplings,       // PORFCPL: flow through a hole to another coupling surface
    HydrostaticProfile,         // PORHYDS: a hydrostatic pressure profile
    UserDefined                 // PORUDS: a user-written porosity model
};
inline constexpr std::array<std::string_view, 9> POROSITY_TYPE_WORDS{
    "PorousFlowBoundary",     "PorousTimeFlowBoundary",    "GasBagVentHole",
    "SurfacePermeability",    "CouplingHoleToGasBag",      "GasBagPermeableLink",
    "CouplingHoleToCoupling", "PorousHydrostaticBoundary", "PorousUserBoundary"};

// COEFF and COEFFV of a LEAKAGE (fields 6 and 7): the porosity coefficient is a constant or a table of time (a
// TABLED1); a blank COEFFV is 1.0, the whole area open. The constant is given an open range from 0.0 to
// 1.0, but the description of PORTYPE says that 1.0 opens the whole area and 0.0 closes it, so both ends are
// allowed.
inline constexpr SourcedValueLayout GAS_BAG_LEAKAGE_COEFF_LAYOUT{
    6,
    7,
    "coefficientSource",
    "coefficient",
    static_cast<std::uint8_t>(sourceBit(ValueSource::Constant) | sourceBit(ValueSource::Table)),
    ValueSource::Constant,
    1.0};

// One LEAKAGE card (SOL 700): a porosity definition of a part of the surface of a GBAG or a COUPLE, which names the
// card that describes the flow (a hole, a permeable material, a flow boundary). A GBAG or a COUPLE names the set of
// LEAKAGE cards in its PORID field.
//
// Card layout: LEAKAGE  LID  PORID  SUBID  PORTYPE  PORTYPID  COEFF  COEFFV            fields 1 to 7
// LID numbers the card, PORID is the number of the set (the cards of a set share it, so it is the ID the model files
// the card under: IdSpace::Group). SUBID names the BSURF, BCBOX, BCPROP, BCMATL or BCSEG, a part of the surface of the
// GBAG or COUPLE; 0 or blank means the whole surface. PORTYPE says what kind of card PORTYPID is the number of. The
// model keeps the number as a reference to the category of the porosity cards (ReferenceTarget::Porosity) and does not
// match the PORTYPE word against the card that has the number, so a PORFLOW 5 satisfies a PORTYPID 5 of type PERMEAB.
struct GasBagLeakageCard
{
    base::EntityId id = 0;           // the number of the card
    base::EntityId setId = 0;        // the number of the set, which a GBAG or a COUPLE names
    base::EntityId subsurfaceId = 0; // a part of the surface, 0 for the whole of it
    PorosityType porosityKind = PorosityType::PorousFlow;
    base::EntityId porosityModelId = 0; // the porosity card
    SourcedValue coefficient{ValueSource::Constant, 1.0, 0};
};

} // namespace oor::model
