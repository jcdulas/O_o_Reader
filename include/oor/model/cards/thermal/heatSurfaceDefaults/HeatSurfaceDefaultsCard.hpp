#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The TYPE field of a BDYOR: the default type of the surface of a CHBDYP (POINT, LINE, ELCYL, FTUBE, TUBE) or of a
// CHBDYG (REV, AREA3, AREA4, AREA6, AREA8). The enumerators are numbered in the order of HEAT_SURFACE_TYPE_WORDS, which
// is the order the format reference lists them in.
enum class HeatSurfaceDefaultsSurfaceType : std::uint8_t
{
    Point,
    Line,
    Rev,
    Area3,
    Area4,
    Elcyl,
    Ftube,
    Area6,
    Area8,
    Tube
};

inline constexpr std::array<std::string_view, 10> HEAT_SURFACE_TYPE_WORDS{"Point",
                                                                          "Line",
                                                                          "ReversedNormal",
                                                                          "AreaTria3",
                                                                          "AreaQuad4",
                                                                          "EllipticCylinder",
                                                                          "ForcedConvectionTube",
                                                                          "AreaTria6",
                                                                          "AreaQuad8",
                                                                          "Tube"};

// One BDYOR card: the default values of the CHBDYP, CHBDYG and CHBDYE surface elements. A deck may have one.
//
// Card layout: BDYOR  TYPE  IVIEWF  IVIEWB  RADMIDF  RADMIDB  (blank)  PID  GO          fields 1 to 8
//                    CE  E1  E2  E3                                                    fields 9 to 12
// Every field may be blank. TYPE has no default of its own (blank: nothing, `std::nullopt`) and E1, E2 and E3 stay
// absent when blank. The IDs are 0 when blank (a VIEW, RADM, PHBDY, grid point or system that no default names): IVIEWF
// and IVIEWB name VIEW cards, RADMIDF and RADMIDB RADM cards, PID a PHBDY, GO a grid point (0 by default), CE a
// coordinate system. A CE of 0 is the basic system, the same as a blank. The orientation vector E1, E2, E3 is given in
// system CE and is not used when GO is given; the format says GO is only a default when it is positive, which
// is the business of the card that takes the default. This card only holds the values; the CHBDYE, CHBDYG and CHBDYP
// give a blank VIEW, RADM, GO, CE and orientation vector (0 and absent: no card has the ID 0) the default of the BDYOR
// when a deck is read (their `finishRead`, and Model::applyDefaults for a BDYOR added later), and a CHBDYG with a blank
// TYPE takes the TYPE of the BDYOR when it is a type of a CHBDYG (REV, AREA3, AREA4, AREA6, AREA8). The TYPE and the
// PID of a CHBDYP stay required on that card.
struct HeatSurfaceDefaultsCard
{
    std::optional<HeatSurfaceDefaultsSurfaceType> kind;
    base::EntityId frontViewId = 0;
    base::EntityId backViewId = 0;
    base::EntityId frontRadiationPropId = 0;
    base::EntityId backRadiationPropId = 0;
    base::EntityId propId = 0;
    base::EntityId orientationNode = 0;
    base::EntityId directionFrameId = 0;
    std::optional<double> direction1;
    std::optional<double> direction2;
    std::optional<double> direction3;

    [[nodiscard]] bool operator==(const HeatSurfaceDefaultsCard&) const noexcept = default;
};

} // namespace oor::model
