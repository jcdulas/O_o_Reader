#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/LabeledValues.hpp"

#include <vector>

namespace oor::model
{

// One UXVEC card: a vector of values of the aerodynamic extra points (the control surfaces, rigid body motions and
// general controllers of a trim), which the user defined forces of an AEDW, AEFORCE or AEPRESS card are evaluated at.
//
// Card layout: UXVEC  ID  (fields 2 to 8 blank)
//                    LABEL1  UX1  LABEL2  UX2  -etc.-
// The first line holds the ID alone. The pairs of a label and a magnitude start on the continuation line, in field 9,
// two fields to a pair and four pairs to a line, with no limit; the card refuses a value in fields 2 to 8 (the format
// draws them blank) and a pair with only one of its two fields. There may be no pair: a variable that no pair names is
// 0.0. A label is the LABEL of an AESURF, AESTAT or AEPARM, or INTERCPT, the word that scales the
// intercept aerodynamics (it is a name like the others, the card does not single it out). The labels are
// kept as written and compared without regard to the case by a check between cards. The same label may be given more
// than once: the format reference says that several magnitudes of one controller are listed that way.
//
// The ID is referenced by the Case Control command AEUXREF and by the UXID field of an AEDW, AEFORCE or AEPRESS card
// (ReferenceTarget::ControlVector).
struct AeroControlVectorCard
{
    base::EntityId id = 0;
    std::vector<LabeledValue> variables; // the label and the magnitude (UX) of each variable

    [[nodiscard]] bool operator==(const AeroControlVectorCard&) const noexcept = default;
};

} // namespace oor::model
