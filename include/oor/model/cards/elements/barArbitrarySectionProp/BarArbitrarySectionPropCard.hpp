#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/ArbitrarySection.hpp"

#include <vector>

namespace oor::model
{

// One PBRSECT card: the arbitrary cross-section of a CBAR, described by the points of a set of POINT cards. The theory
// that computes the section properties is the one of the PBMSECT.
//
// Card layout:
//   PBRSECT  PID  MID  FORM  NSM
//            data description of the section: items KEY=value separated by commas, over as many continuation lines as
//            it needs
//   for instance: OUTP=5,BRP=7,T=0.8,T(3)=[1.5,PT=(31,32)]
//
// FORM is GS (general section: an outer perimeter, inner ones), OP (open profile) or CP (closed profile): no default.
// NSM, the non-structural mass per unit length, is a field of the first line (0.0 when blank) and also a keyword of the
// description: a card gives it in one place, not both. The items are kept in the order of the card,
// parsed (`ArbitrarySectionItem`); the keywords are OUTP, INP, BRP, T and NSM, each with the (id) the format reference
// gives it, and the form says which an item may be
// (`checkSectionItems`: INP for GS, BRP and T for OP and CP). The section needs an OUTP; that OP and CP sections have a
// T without a PT as the thickness of the segments that have none, that the points of a segment are in the sets, and the
// rest of the geometry are rules the checks of the model keep. The ID is in the space of all the properties (the format
// reference asks it to be unique among PBAR, PBARL and PBRSECT, which is a rule between cards).
struct BarArbitrarySectionPropCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0;
    ArbitrarySectionForm form = ArbitrarySectionForm::General;
    double nonStructMass = 0.0; // the NSM field; an NSM item of the description is in `items`
    std::vector<ArbitrarySectionItem> items;

    [[nodiscard]] bool operator==(const BarArbitrarySectionPropCard&) const noexcept = default;
};

} // namespace oor::model
