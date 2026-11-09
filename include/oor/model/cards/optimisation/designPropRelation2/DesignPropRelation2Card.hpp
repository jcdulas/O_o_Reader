#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/DesignField.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// One DVPREL2 card: a field of a property as the value of a DEQATN equation of design variables and constants.
//
// Card layout: DVPREL2  ID  TYPE  PID  PNAME/FID  PMIN  PMAX  EQID                  fields 1 to 7
//                      DESVAR  DVID1  DVID2  ...                                      the lines after, from field 9
//                      DTABLE  LABL1  LABL2  ...
// The equation receives the DVIDs and then the constants named by the LABLs, in that order, as its arguments.
// TYPE, PID, PNAME/FID, PMIN and PMAX are those of the DVPREL1 (a PMIN left blank is 1.0E-15, or -1.0E35 for a
// field that is a stress recovery location: it depends on the field, so it stays blank here). The format reference says
// that PBEND, PBARL and PBEAML are not supported by this card; the card does not refuse them.
struct DesignPropRelation2Card
{
    static constexpr double NO_UPPER_LIMIT = 1.0e20;

    base::EntityId id = 0;     // unique among the DVPREL2 cards
    std::string kind;          // upper case
    base::EntityId propId = 0; // the property
    DesignField field;
    std::optional<double> minValue;
    double maxValue = NO_UPPER_LIMIT;
    base::EntityId equationId = 0;            // the DEQATN
    std::vector<base::EntityId> designVarIds; // DVIDi
    std::vector<std::string> labels; // LABLi: constants of a DTABLE or DTABLE2 card, matched without regard to case
};

} // namespace oor::model
