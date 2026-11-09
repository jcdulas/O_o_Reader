#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// How a HADAPTL says where its criterion applies: to every element, to one superelement or to the elements of one
// property. The enumerators are numbered in the order of ADAPTIVE_REFINE_REGION_WORDS.
enum class AdaptivityRegion : std::uint8_t
{
    All,   // ALL (the default): WHEREID is ignored
    Super, // SUPER: WHEREID is a superelement number
    Prop   // PROP: WHEREID is a property ID
};

inline constexpr std::array<std::string_view, 3> ADAPTIVE_REFINE_REGION_WORDS{"All", "Superelement", "Property"};

// The REPEAT of a blank field, which is also what a blank MAXLEVEL takes.
inline constexpr std::int32_t ADAPTIVE_REFINE_DEFAULT_REPEAT = 6;

// One HADAPTL card: the control of the local adaptive mesh refinement of SOL 101 and 400 for one criterion. The Case
// Control command HADAPT selects the cards by ID.
//
// Card layout: HADAPTL  ID  (blank)  (blank)  REPEAT  CRITID  WHEREMET  WHEREID  SNAPMETH      fields 1 to 8
//                      MAXLEVEL                                                              field 9
// Fields 2 and 3 are blank in the layout and the second line holds MAXLEVEL alone. REPEAT is the largest number of
// analyses before the refinement stops, at least 0, and is 6 when blank. CRITID names the HADACRI of the criterion and
// has no default. WHEREMET is ALL, SUPER or PROP (ALL when blank); WHEREID is the superelement or the property, 0 when
// blank, and the format reference says it is ignored for ALL, so the card keeps what is written. SNAPMETH is 0 (new
// mid-side nodes in the middle of their edges) or 1 (nodes on the boundary are projected onto a smooth approximation of
// it); the format types it as any integer of 0 or more but
// gives a meaning to those two only. MAXLEVEL is the highest refinement level of an
// element, above 0, and is REPEAT when blank: a default that depends on another field, so it is stored resolved and
// always written (and a REPEAT of 0 with no MAXLEVEL gives a MAXLEVEL of 0, which the card refuses).
//
// WHEREID with SUPER is a superelement number and 0 is the residual structure, so it may be zero; with PROP it must
// name a property (a property 0 does not exist), which the card requires. The text of the format gives REPEAT,
// WHEREID and SNAPMETH as positive, its drawing as zero or more (a lost sign); the drawing decides.
//
// Several HADAPTL may have the same ID (each picks a different criterion for a different part of the
// mesh), so the ID is a set ID shared by the cards: IdSpace::Group in the category AdaptivitySet. Where they differ,
// The solver takes the largest REPEAT, SNAPMETH and MAXLEVEL of the cards of the set: a rule of the solver.
// The format reference puts the card in the main Bulk Data only, a rule of the deck.
struct AdaptiveRefineControlCard
{
    base::EntityId id = 0;                                      // the set that the Case Control HADAPT selects
    std::int32_t repeatCount = ADAPTIVE_REFINE_DEFAULT_REPEAT;  // at least 0
    base::EntityId criterionId = 0;                             // the HADACRI of the criterion
    AdaptivityRegion regionMethod = AdaptivityRegion::All;      // where the criterion applies
    base::EntityId regionId = 0;                                // the superelement or the property, 0 when blank
    std::int32_t snapMethod = 0;                                // 0 or 1
    std::int32_t maximumLevel = ADAPTIVE_REFINE_DEFAULT_REPEAT; // above 0; REPEAT when the deck leaves it blank
};

} // namespace oor::model
