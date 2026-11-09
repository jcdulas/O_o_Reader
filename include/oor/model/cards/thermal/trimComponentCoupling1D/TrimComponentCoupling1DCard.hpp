#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The modeling method of an analytical trim component (the METHOD field): the format names one, 1D. The enumerators are
// numbered in the order of TRIM_COUPLING_1D_METHOD_WORDS.
enum class TrimComponentCoupling1DMethod : std::uint8_t
{
    OneDimensional // 1D
};

inline constexpr std::array<std::string_view, 1> TRIM_COUPLING_1D_METHOD_WORDS{"OneDimensional"};

// One ACPMCP1 card: how a 1D analytical trim component (a layered porous material, TCOMPG, that has no finite element
// mesh of its own) is coupled to the structure and to the cavities. It is required for each such component that Case
// Control TRIMGRP names, and goes in the main Bulk Data (`BEGIN BULK`). The coupling conditions and the
// degrees of freedom of the reduced impedance matrices come from it.
//
// Card layout: ACPMCP1  TID  SGLUED  SSLIDE  SOPEN  SIMPER                          fields 1 to 5
//                      RID                                                         field 16 (the last of the second
//                      line) METHOD                                                      field 17 (the first of the
//                      third line)
// RID is in the last column of the second line and METHOD in the first of the third, which is where the card reads
// and writes them; the rest of the second line is blank.
//
// TID is the number of the trim component, a plain integer. SGLUED, SSLIDE, SOPEN and SIMPER name SET3 cards of PLTSURF
// surface meshes of the component: glued to the structure, in sliding contact with it, open to the cavities, impervious
// to them; a blank (0) one is an interface the component does not have. Because the 1D component is
// analytic, a SSLIDE is taken as SGLUED and a SIMPER as SOPEN. RID is the region of the component, 0 when blank. METHOD
// is 1D or blank, and a blank one stays blank. Like an ACPEMCP, a component may have several cards told apart by their
// RID, so the cards share the TID (IdSpace::Group).
struct TrimComponentCoupling1DCard
{
    base::EntityId id = 0; // the trim component, shared by its cards
    base::EntityId gluedSetId = 0;
    base::EntityId slidingSetId = 0;
    base::EntityId openSetId = 0;
    base::EntityId imperviousSetId = 0;
    base::EntityId regionId = 0;
    std::optional<TrimComponentCoupling1DMethod> method;

    [[nodiscard]] bool operator==(const TrimComponentCoupling1DCard&) const noexcept = default;
};

} // namespace oor::model
