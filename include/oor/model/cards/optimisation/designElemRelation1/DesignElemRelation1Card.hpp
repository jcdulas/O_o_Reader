#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/DesignTerms.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// One DVCREL1 card: a field of an element as a linear function of design variables.
//
// Card layout: DVCREL1  ID  TYPE  EID  CPNAME  CPMIN  CPMAX  C0                  fields 1 to 7
//                      DVID1  COEF1  DVID2  COEF2  DVID3  COEF3  ...               fields 9 to 16 and the lines after
//   connectivity field = C0 + sum of COEFi * (design variable DVIDi)                        (the continuation
//   is required)
// TYPE is the name of the element card (CBAR, CQUAD4, CELAS2, ...); CPNAME is the name the element card gives the field
// in the format reference ("X1", "ZOFFS", "K"). CPMIN, the lowest value the field may take, is blank when the default
// depends on the field (1.0E-15 for a field that can only be positive, else -1.0E35: the format reference gives the
// rule and the library has no table of the fields of these cards), so it is not resolved here; CPMAX is 1.0E20 when
// blank. The first coefficient may be the word PVAL, which takes it from the value the field holds in the deck.
struct DesignElemRelation1Card
{
    static constexpr double NO_UPPER_LIMIT = 1.0e20;

    base::EntityId id = 0;     // unique among the DVCREL1 cards
    std::string kind;          // upper case
    base::EntityId elemId = 0; // an element
    std::string name;          // CPNAME, upper case
    std::optional<double> minValue;
    double maxValue = NO_UPPER_LIMIT;
    double constant0 = 0.0;
    std::vector<DesignTerm> terms; // at least one; the first coefficient is PVAL when `fromProperty`
    bool fromProperty = false;
};

} // namespace oor::model
