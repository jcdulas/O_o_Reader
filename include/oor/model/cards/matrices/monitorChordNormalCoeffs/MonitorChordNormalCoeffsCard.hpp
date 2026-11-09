#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One MONCNCM card: a monitor point of the stripwise normal force and pitching moment coefficients of the chordwise
// strips of CAERO1 panels (flutter and aeroelastic response). A strip is made of the boxes of a CAERO1 along a span
// station; strips that continue one another across panels are one strip.
//
// Card layout: MONCNCM  NAME  LABEL (columns 3 to 9: up to 56 characters)
//                      .     MREF  CAERID1  CAERID2  -etc.-  CAERIDn
// In this library's numbering (field 1 is NAME): LABEL is fields 2 to 8, MREF field 9 and the CAERIDs fields 10 and on
// (the list goes on over the next fields and lines). NAME is kept in upper case, the LABEL as written. MREF is the
// fractional chord position the moment is taken about, from 0.0 to 1.0, and the quarter chord (0.25) when blank. The
// first CAERID may be the word ALL, which stands for every strip: the card then holds no ID, and an ID
// after ALL is refused (the format does not say what it would mean). The first CAERID is in the third column of the
// second line, after MREF in the second, and MREF may be blank. The monitor point has no numeric ID (IdSpace::None): it
// is found by its NAME, which the format reference says is unique.
struct MonitorChordNormalCoeffsCard
{
    std::string name;  // up to 8 characters
    std::string label; // up to 56 characters, may be empty
    double momentChordFraction = 0.25;
    bool allStrips = false;                   // CAERID1 is the word ALL
    std::vector<base::EntityId> aeroPanelIds; // CAERO1 elements; empty when allStrips

    [[nodiscard]] bool operator==(const MonitorChordNormalCoeffsCard&) const noexcept = default;
};

} // namespace oor::model
