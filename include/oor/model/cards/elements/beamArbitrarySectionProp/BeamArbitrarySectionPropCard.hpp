#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/ArbitrarySection.hpp"

#include <vector>

namespace oor::model
{

// One PBMSECT card: the arbitrary cross-section of a CBEAM (or, with a Core or a Layer, of a CBEAM3), described by the
// points of a set of POINT cards. Constant sections only.
//
// Card layout:
//   PBMSECT  PID  MID  FORM
//            data description of the section: items KEY=value separated by commas, over as many continuation lines as
//            it needs
//   for instance: OUTP=5,BRP=7,T=0.8,T(3)=[1.5,PT=(31,32)],NSM=0.02
//
// FORM is GS (general section: an outer perimeter, inner ones), OP (open profile) or CP (closed profile): no default.
// The items are kept in the order of the card, parsed (`ArbitrarySectionItem`); the keywords are OUTP, OUTM, INP, BRP,
// T, CORE, LAYER and NSM, each with the (id) the format reference gives it, and the form says which an item may be
// (`checkSectionItems`: INP for GS, BRP, T, CORE and LAYER for OP and CP, OUTM alone). The section needs an OUTP (or an
// OUTM); that OP and CP sections have a T without a PT as the thickness of the segments that have none, that the points
// of a segment are in the sets, and the rest of the geometry are rules the checks of the model keep. The ID is in the
// space of all the properties (the format reference asks it to be unique among PBEAM, PBEAML, PBCOMP, PBMSECT and
// PBEAM3, which is a rule between cards).
struct BeamArbitrarySectionPropCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0;
    ArbitrarySectionForm form = ArbitrarySectionForm::General;
    std::vector<ArbitrarySectionItem> items;

    [[nodiscard]] bool operator==(const BeamArbitrarySectionPropCard&) const noexcept = default;
};

} // namespace oor::model
