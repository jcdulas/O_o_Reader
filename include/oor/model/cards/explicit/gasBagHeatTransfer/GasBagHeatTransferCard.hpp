#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

enum class HeatTransferType : std::uint8_t
{
    Convection, // HTRCONV: heat transfer through convection
    Radiation   // HTRRAD: heat transfer through radiation
};
inline constexpr std::array<std::string_view, 2> HEAT_TRANSFER_WORDS{"GasBagConvection", "GasBagRadiation"};

// COEFF and COEFFV of a HEATLOS (fields 6 and 7): the area coefficient is a constant or a table of time (a TABLED1); a
// blank COEFFV is 1.0, the whole area of the subsurface exposed. The format gives the range of the constant as 0.0 to
// 1.0 (0.0 means no heat transfer through the subsurface, 1.0 the complete area), so both ends are allowed.
inline constexpr SourcedValueLayout GAS_BAG_HEAT_TRANSFER_COEFF_LAYOUT{
    6,
    7,
    "coefficientSource",
    "coefficient",
    static_cast<std::uint8_t>(sourceBit(ValueSource::Constant) | sourceBit(ValueSource::Table)),
    ValueSource::Constant,
    1.0};

// One HEATLOS card (SOL 700): a heat transfer definition of a part of the surface of a GBAG or a COUPLE, which names
// the card that describes the transfer (a convection HTRCONV, a radiation HTRRAD). A GBAG or a COUPLE names the set of
// HEATLOS cards in its HTRID field. Several HEATLOS of different types may be combined and different types
// may act on the same BSURF; a model can then be switched from the uniform pressure to the Eulerian model
// with a GBAGCOU.
//
// Card layout: HEATLOS  HID  HTRID  SUBID  HTRTYPE  HTRTYPID  COEFF  COEFFV             fields 1 to 7
// HID numbers the card (unique among the HEATLOS cards, a number of the card only: the library does not index it);
// HTRID is the number of the set (the cards of a set share it, so it is the ID the model files the card under:
// IdSpace::Group). SUBID names the BSURF, BCBOX, BCPROP, BCMATL or BCSEG that is a part of the surface of the GBAG or
// COUPLE; 0 or blank means the whole surface. HTRTYPE says what kind of card HTRTYPID is the number of. The model keeps
// the number as a reference to the category of the heat transfer cards (ReferenceTarget::HeatTransferModel) and does
// not match the HTRTYPE word against the card that has the number: the check `explicit.gas-bag-heat-transfer-type`
// does. The structure is that of the LEAKAGE card and the format words it alike (an area coefficient of a subsurface).
struct GasBagHeatTransferCard
{
    base::EntityId id = 0;           // the number of the card
    base::EntityId setId = 0;        // the number of the set, which a GBAG or a COUPLE names
    base::EntityId subsurfaceId = 0; // a part of the surface, 0 for the whole of it
    HeatTransferType modelKind = HeatTransferType::Convection;
    base::EntityId modelId = 0; // the HTRCONV or HTRRAD
    SourcedValue coefficient{ValueSource::Constant, 1.0, 0};
};

} // namespace oor::model
