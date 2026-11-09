#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <string>

namespace oor::model
{

// How many fields the card keeps after FIELD_ID without reading them: fields 5 to 12, eight of them.
inline constexpr std::size_t ELEM_GENERATION_OPTION_COUNT = 8;

// One CGEN card: the generation of elements in a field of grid points (the geometry-based mesh generation cards of
// the format, with GRIDG and EGRID). It makes elements, it is not an element, and the library does not expand it.
//
// Layout:
//   CGEN  TYPE  FIELD_EID  PID  FIELD_ID  DIR  TH_GEOM_OPT  EIDL  EIDH
//         TA    TB         TC   TD
//
// TYPE is the element type to generate (TRIA3, TRIA6, QUAD4, QUAD8, HEXA8, HEXA20, ...; no list is known here, so any
// name is kept), FIELD_EID the first element ID of the elements the card makes, PID their property and FIELD_ID the
// GRIDG field they are generated in (a plain ID: GRIDG is not typed). The card is not in the format reference list and
// the format reference says no more.
//
// What is not known is kept as the deck wrote it. The fields after FIELD_ID are `options`, in the order of the layout
// (DIR, TH_GEOM_OPT, EIDL, EIDH, then TA to TD of the second line), each the text of its field in upper case, empty
// when the field is blank. A deck may put an integer in DIR (CGEN QUAD4 110001 2 1 1), so the types of those fields are
// not settled and the card does not decide them. A field after the twelfth is refused.
struct ElemGenerationCard
{
    std::string kind; // upper case, never blank
    base::EntityId firstElemId = 0;
    base::EntityId propId = 0;
    base::EntityId gridFieldId = 0;
    std::array<std::string, ELEM_GENERATION_OPTION_COUNT> options;

    [[nodiscard]] bool operator==(const ElemGenerationCard&) const noexcept = default;
};

} // namespace oor::model
