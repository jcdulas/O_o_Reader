#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

// How a COSMGRP describes the coupled area: by the faces of elements, by grid points or by elements.
enum class CosimulationAreaType : std::uint8_t
{
    Surface,
    Point,
    Volume,
};

// The words of TYPE, in the order of the enumeration.
inline constexpr std::array<std::string_view, 3> COSIMULATION_AREA_WORDS{"Surface", "Point", "Volume"};

// One face of the SURFACE form: an element and the number of its face, the numbering of the sides of a CHBDYE.
struct CosimulationFace
{
    base::EntityId elemId = 0;
    std::int32_t face = 0;

    [[nodiscard]] bool operator==(const CosimulationFace&) const noexcept = default;
};

// One COSMGRP card (SOL 400): the coupled area through which the solver exchanges data with a co-simulation partner. A
// COSMSRV names it twice, as the area that receives the partner's input and the one that gives the output.
//
// Card layout: COSMGRP  GRPID  TYPE                                          fields 1 and 2, fields 3 to 8 blank
//             +        EID  FaceID  EID  FaceID  EID  FaceID  EID  FaceID   TYPE=SURFACE, fields 9 on
//             +        GridID1  GridID2  ...                                TYPE=POINT, fields 9 on
//             +        EID1  EID2  EID3  ...                                TYPE=VOLUME, fields 9 on
// TYPE is SURFACE when blank. Each form lists its items over as many continuation lines as it needs, with no end
// marker; a blank field is the padding of a line. The first line holds nothing but GRPID and TYPE, as the layout has
// it, so a value in fields 3 to 8 is refused.
//
// The format does not draw THRU. The step brief asked for it, and a list of grid points or elements is the usual place
// for it, so the POINT and VOLUME lists accept "a THRU b" (upwards, no BY), kept as written like every list of IDs of
// the library; the SURFACE pairs have none. A line holds eight values: a ninth one would sit in the field that a fixed
// deck keeps for the continuation marker, so the card reads eight to a line, as the layout has it.
//
// The card is one of the three forms: `faces` for SURFACE, `runs` for POINT (grid points) and VOLUME (elements), and
// the other list is empty. The GRPID is unique among the COSMGRP cards (IdSpace::PerCardType, category
// CosimulationGroup).
struct CosimulationAreaCard
{
    base::EntityId id = 0;
    CosimulationAreaType kind = CosimulationAreaType::Surface;
    std::vector<CosimulationFace> faces; // SURFACE
    std::vector<ContactRun> runs;        // POINT and VOLUME
};

} // namespace oor::model
