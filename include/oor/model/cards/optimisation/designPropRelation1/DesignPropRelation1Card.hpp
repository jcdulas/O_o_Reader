#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/DesignField.hpp"
#include "oor/model/cards/optimisation/DesignTerms.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// One DVPREL1 card: a field of a property as a linear function of design variables.
//
// Card layout: DVPREL1  ID  TYPE  PID  PNAME/FID  PMIN  PMAX  C0                  fields 1 to 7
//                      DVID1  COEF1  DVID2  COEF2  DVID3  COEF3  ...               fields 9 to 16 and the lines after
//   property field = C0 + sum of COEFi * (design variable DVIDi)                   (the continuation is
//   required)
// TYPE is the name of the property card (PBAR, PSHELL, ...), or GPLY for a ply of the laminates, in which case PID is a
// global ply ID and not a property. PNAME/FID says which field (`DesignField`). PMIN, the lowest value the field may
// take, is blank when the default depends on the field (1.0E-15 for a field that can only be positive, else -1.0E35:
// the format reference gives the rule and the library has no table of the fields of the property cards), so it is not
// resolved here; PMAX is 1.0E20 when blank. The first coefficient may be the word PVAL, which takes it from the value
// the property holds in the deck.
struct DesignPropRelation1Card
{
    static constexpr double NO_UPPER_LIMIT = 1.0e20;

    base::EntityId id = 0;     // unique among the DVPREL1 cards (a DRESP2 names the two kinds in lists of their own)
    std::string kind;          // upper case
    base::EntityId propId = 0; // the property (a global ply ID when TYPE is GPLY)
    DesignField field;
    std::optional<double> minValue;
    double maxValue = NO_UPPER_LIMIT;
    double constant0 = 0.0;
    std::vector<DesignTerm> terms; // at least one; the first coefficient is PVAL when `fromProperty`
    bool fromProperty = false;
};

} // namespace oor::model
