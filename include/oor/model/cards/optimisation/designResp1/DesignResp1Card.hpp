#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/DesignValue.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One DRESP1 card: a response of the structure (a stress, a displacement, a weight, a frequency, ...) that the design
// uses as a constraint or as the objective.
//
// Card layout: DRESP1  ID  LABEL  RTYPE  PTYPE  REGION  ATTA  ATTB  ATT1                  fields 1 to 8
//                     ATT2  ATT3  -etc.-                                                 fields 9 to 16, and so on
// RTYPE is the type of response (WEIGHT, STRESS, DISP, FREQ, FLUTTER, ...). PTYPE is
// ELEM or the name of a property card for the responses of elements (the ATTi then number elements or properties of
// that type), the number of a RANDPS for the PSD responses, or a word that the response defines (STRUC or FLUID, PANEL,
// the name of a monitor point). What ATTA, ATTB and ATT1, ATT2, ... hold depends on RTYPE: an item code, a mode or a
// LAMINA number, a frequency or a time (a real, or one of the words SUM, AVG, SSQ, RSS, MAX, MIN), grid points,
// elements, properties, a set of modes, or the word ALL (superelements). The card keeps each as the deck wrote it
// (`DesignValue`: blank, integer, real or word) and says what the integers of ATTi name from the response type
// (`DesignResp1References.hpp`). A THRU between two of them is a word of the list. REGION groups the responses for the
// screening of constraints and is blank for the default.
struct DesignResp1Card
{
    base::EntityId id = 0; // in the number space of DRESP1, DRESP2 and DRESP3
    std::string entryLabel;
    std::string responseKind; // upper case
    DesignValue ownerKind;
    base::EntityId regionId = 0; // 0: blank
    DesignValue attributeA;
    DesignValue attributeB;
    std::vector<DesignValue> attributes; // ATT1, ATT2, ...: a blank inside the list stays a blank
};

} // namespace oor::model
