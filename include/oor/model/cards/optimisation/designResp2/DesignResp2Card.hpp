#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/DesignGroups.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One DRESP2 card: a response computed from design variables, constants and other responses, by a DEQATN equation or by
// one of the functions of the format reference.
//
// Card layout: DRESP2  ID  LABEL  EQID/FUNC  REGION  METHOD  C1  C2  C3                   fields 1 to 8
//                     DESVAR  DVID1 ...                                                 the lines after, from field 9
//                     DTABLE  LABL1 ...   DRESP1  NR1 ...   DNODE  G1 C1 ...   DVPREL1 ...   (`ResponseArguments.hpp`)
// The third field is the number of a DEQATN, or the name of a function applied to every argument: SUM, SFMAX, AVG, SSQ,
// RSS, MAX, MIN, BETA or MATCH. METHOD and C1 to C3 belong to BETA and MATCH: METHOD is
// MIN or MAX with BETA and LS or BETA with MATCH, and blank means MIN with BETA and LS with MATCH, so it stays blank
// here and means nothing for an equation; the constants default to 1.0, 0.005 and 10.0 (the defaults of C1 and C2 have
// changed over the life of the format; these are the current ones). The responses of DRESP1, DRESP2 and DRESP3 share
// one number space.
struct DesignResp2Card
{
    static constexpr double DEFAULT_CONSTANT1 = 1.0;
    static constexpr double DEFAULT_CONSTANT2 = 0.005;
    static constexpr double DEFAULT_CONSTANT3 = 10.0;

    base::EntityId id = 0;
    std::string entryLabel;
    base::EntityId equationId = 0; // the DEQATN, or 0 when a function is named
    std::string function;          // upper case; empty when an equation is named
    base::EntityId regionId = 0;   // 0: blank
    std::string methodName;        // upper case, empty when blank
    double constant1 = DEFAULT_CONSTANT1;
    double constant2 = DEFAULT_CONSTANT2;
    double constant3 = DEFAULT_CONSTANT3;
    std::vector<DesignGroup> groups; // in the order of DESIGN_RESP2_GROUPS
};

} // namespace oor::model
