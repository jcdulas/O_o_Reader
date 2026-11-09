#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/elements/connectorSearchParams/ConnectorSearchParamsCard.hpp"

#include <vector>

namespace oor::model
{

// One CONCTL card: values of the parameters of the connector search that apply to a group of connector elements (CFAST,
// CSEAM, CWELD) and override the ones of the SWLDPRM card for them.
//
// Card layout: CONCTL  SET3ID  (blank)  PARAM1  VALUE1  PARAM2  VALUE2  PARAM3  VALUE3        fields 1 to 8
//                     PARAM4  VALUE4  -etc.-                                                 fields 9 to 16, and so on
// SET3ID is a SET3 (its DES must be ELEM, a rule between two cards that the reader does not check) that lists
// the connector elements; the elements are looked up when the parameters are used, so the card names a set (`Set`). The
// parameters are NAME and VALUE pairs from field 3 on; their names and values are those of the table of the SWLDPRM
// entry (`ConnectorSearchParamsCatalogue.hpp`: the two cards share one set of parameters), so a listed name takes the
// values the table gives it and a name that the table does not list is kept as written, as the SWLDPRM card does.
// CHKRUN can be set on a SWLDPRM only and is ignored on a CONCTL: the card keeps it. Several CONCTL cards are allowed
// and all of them are used, so the card has no ID (IdSpace::None); two cards that give different values for one
// parameter of one element are a fatal error of the solver, which is a rule between cards and not checked here. A card
// with no parameter changes nothing, so it is refused as a likely mistake.
struct ConnectorSearchControlCard
{
    base::EntityId elementSetId = 0;
    std::vector<ConnectorSearchParamsParameter> parameters; // in the order of the card

    [[nodiscard]] bool operator==(const ConnectorSearchControlCard&) const noexcept = default;
};

} // namespace oor::model
