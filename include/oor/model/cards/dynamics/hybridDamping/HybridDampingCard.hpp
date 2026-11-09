#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The YES or NO of the KDAMP and PRTEIG fields of a HYBDAMP, both defaulting to NO. The enumerators are numbered in the
// order of HYBRID_DAMPING_OPTION_WORDS.
enum class HybridDampingOption : std::uint8_t
{
    No, // the default
    Yes
};

inline constexpr std::array<std::string_view, 2> HYBRID_DAMPING_OPTION_WORDS{"No", "Yes"};

// One HYBDAMP card: the hybrid modal damping of a direct dynamic solution. The modes of the structure come from the
// eigenvalue method METHOD, and the modal damping from the TABDMP1 SDAMP.
//
// Card layout: HYBDAMP  ID  METHOD  SDAMP  KDAMP  PRTEIG          fields 1 to 5
// ID, METHOD and SDAMP are required. A DAMPING (field HYBRID) and a ROTHYBD name the card by its ID. KDAMP YES puts the
// modal damping in the complex stiffness matrix as structural damping; PRTEIG YES prints the eigenvalue
// summary of the hybrid damping calculation.
struct HybridDampingCard
{
    base::EntityId id = 0;                  // unique among the HYBDAMP cards
    base::EntityId methodId = 0;            // the eigenvalue method that gives the modes (EIGRL, EIGR, ...)
    base::EntityId modalDampingTableId = 0; // the TABDMP1 that gives the modal damping
    HybridDampingOption dampingInStiffness = HybridDampingOption::No;
    HybridDampingOption printEigenvalues = HybridDampingOption::No;
};

} // namespace oor::model
