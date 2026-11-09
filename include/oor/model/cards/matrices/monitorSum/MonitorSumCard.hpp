#pragma once

#include "oor/model/cards/matrices/MonitorSumTerms.hpp"

#include <string>

namespace oor::model
{

// One MONSUM card: a new monitor result that is the weighted sum of existing monitor results (the SOLs that have
// monitor points: 101, 103, 108, 109, 111, 112, 144, 146, 200 and 400). The result for the components NEWAXISj is the
// sum over the terms of COEFij times the components AXESij of the monitor result NAMEij; a MONSUM whose NAMEs are its
// own NAME scales that monitor result.
//
// Card layout (the first of two drawings):
//   MONSUM  NAME  LABEL (fields 3 to 9: up to 56 characters)
//           NEWAXISA  MTYPE1A  NAME1A  AXES1A  COEF1A  NAME2A  AXES2A  COEF2A
//                              NAME3A  AXES3A  COEF3A  etc.
//                     MTYPE2A  NAME2A  etc.
//           NEWAXISB  MTYPE1B  NAME1B  ...
// and the alternate layout, which has the first two fields of the lines the other way round (the MTYPE first, then the
// NEWAXISj):
//   MONSUM  NAME  LABEL
//           MTYPE  NEWAXISA  NAME1A  AXES1A  COEF1A  NAME2A  AXES2A  COEF2A
//                            NAME3A  AXES3A  COEF3A  etc.
//                  NEWAXISB  NAME1B  ...
// In this library's numbering (field 1 is NAME) LABEL is fields 2 to 8 and every line after the head has eight fields
// from field 9 on; `MonitorSumTerms.hpp` says what each of them holds. The first layout is told from the alternate one
// by its first field, a number (NEWAXISA) or a word (the MTYPE). The format says an AXES that is blank is the NEWAXISj
// and a blank COEF is 1.0; both are stored resolved.
//
// NAME and the names inside the card are monitor results that other entries name by name (a monitor point, a MONSUM, a
// MONSUM1 or a MONSUMT), so they are kept in upper case and matched without regard to the case, and no reference is
// visited: a name is not an ID. A rule between cards can look each NAMEij up among the monitor points of the model and
// the MONSUM, MONSUM1 and MONSUMT of the deck (a deck may keep them in a part); the card cannot.
struct MonitorSumCard
{
    std::string name;  // up to 8 characters, upper case
    std::string label; // up to 56 characters, may be empty
    MonitorSumBody body;

    [[nodiscard]] bool operator==(const MonitorSumCard&) const noexcept = default;
};

} // namespace oor::model
