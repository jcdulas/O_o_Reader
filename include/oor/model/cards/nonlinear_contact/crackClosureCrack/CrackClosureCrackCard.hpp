#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// The defaults of the integer options of a VCCT. The format lists the numbers it gives a meaning to: ITYPE 0 (no crack
// propagation) or 2 (direct propagation), IGROW 1 (remeshing, said to be unavailable) or 2 (release the tyings or the
// glued contact), INCM 1 (a fixed increment or a user subroutine) and METHOD 1 (the maximum hoop stress criterion). The
// format is not consistent about them (an INCM of 2 also appears), so the lists are not read as the complete set: the
// card keeps any non-negative integer and a check of the model may say which ones the solver knows.
inline constexpr std::int32_t CRACK_CLOSURE_DEFAULT_PROPAGATION = 0;
inline constexpr std::int32_t CRACK_CLOSURE_DEFAULT_GROWTH = 2;
inline constexpr std::int32_t CRACK_CLOSURE_DEFAULT_INCREMENT = 1;
inline constexpr std::int32_t CRACK_CLOSURE_DEFAULT_METHOD = 1;

// One VCCT card (SOL 400): a crack for the virtual crack closure technique, with the options of its growth, the
// resistance of the material and the grid points of the crack front.
//
// Card layout: VCCT  ID  IDCR  ITYPE  IGROW  INCM  METHOD                                  fields 1 to 6
//                   CGI  GC  (4 blanks)  GC-II  GC-III                                      fields 9 to 16
//                   TABCGI  TABGC  (4 blanks)  TABGC-II  TABGC-III                          fields 17 to 24
//                   G1  G2  G3  G4  G5  G6  G7  G8  -etc.-                                  fields 25 on
// ID is the number of the crack set that a Case Control VCCT command names; many cards share it, so it is a set ID
// (IdSpace::Group). IDCR numbers the crack inside the set (1 when blank); that two cracks of a set have different IDCRs
// is a rule between the cards. ITYPE, IGROW, INCM and METHOD default to 0, 2, 1 and 1. CGI is the growth increment, GC
// the resistance to growth (mode I; GC-II and GC-III are modes II and III) and each may be scaled by a table, TABCGI,
// TABGC, TABGC-II and TABGC-III (a TABLEMi; the format reference also allows a TABL3Di, which no step types, so such a
// table is reported unresolved). A blank real is 0.0 and a blank table is 0 (no table).
//
// The crack front is either a list of grid points (`grids`: IDs, "a THRU b" and "a THRU b BY n", kept as written) or
// the SET3 that lists them (`gridSet`, written as G1 = -ID and then nothing else on the card): exactly one of the two.
// The format reference does not limit the direction of a range, so a range may go down.
//
// Lines 2 and 3 are always written, because the format reference forbids a blank line there: a zero stands in the first
// field of each when nothing else does. The seventh and eighth fields of the first line are unused; a common deck puts
// a value in the eighth one, so a value there is read and dropped (the card does not keep it). A value in the unused
// fields of lines 2 and 3 is refused.
struct CrackClosureCrackCard
{
    base::EntityId setId = 0;
    base::EntityId crackId = 1;
    std::int32_t propagationKind = CRACK_CLOSURE_DEFAULT_PROPAGATION;
    std::int32_t growthMethod = CRACK_CLOSURE_DEFAULT_GROWTH;
    std::int32_t incrementMethod = CRACK_CLOSURE_DEFAULT_INCREMENT;
    std::int32_t growthCriterion = CRACK_CLOSURE_DEFAULT_METHOD;
    double growthIncrement = 0.0;
    double growthResistance = 0.0;
    double growthResistanceII = 0.0;
    double growthResistanceIII = 0.0;
    base::EntityId growthIncrementTableId = 0;
    base::EntityId growthResistanceTableId = 0;
    base::EntityId growthResistanceIITableId = 0;
    base::EntityId growthResistanceIIITableId = 0;
    base::EntityId nodeSetId = 0;  // the SET3 of the crack front, 0 when the front is `grids`
    std::vector<ContactRun> nodes; // the crack front, empty when it is the SET3 `gridSet`

    [[nodiscard]] bool operator==(const CrackClosureCrackCard&) const noexcept = default;
};

} // namespace oor::model
