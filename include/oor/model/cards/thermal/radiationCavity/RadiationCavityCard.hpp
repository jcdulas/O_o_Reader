#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// One RADCAV card: the characteristics of one radiation enclosure (cavity).
//
// Card layout:
//   RADCAV  ICAVITY  ELEAMB  SHADOW  SCALE  PRTPCH  NFECI  RMAX  NCOMP
//           SET11  SET12  SET21  SET22  SET31  SET32 -etc.-
// ELEAMB is the ambient surface element (0 when blank); SHADOW is YES (blank) or NO; SCALE is 0.0 when blank; PRTPCH is
// 0 to 5 or absent (a blank PRTPCH is not 0); NFECI is FD, CONT or empty (the program chooses); RMAX is 0.1 when blank
// (the format reference) and NCOMP is 32. The SET pairs name SET1 cards that group the faces into the surfaces
// of the global view factors: up to 30 pairs, so `sets` has an even number of IDs.
struct RadiationCavityCard
{
    base::EntityId id = 0;
    base::EntityId ambientElemId = 0;
    std::string shadowing = "Yes";
    double scaleFactor = 0.0;
    std::optional<int> viewFactorOutput;
    std::string viewFactorMethod; // "" when blank
    double subdivisionAreaRatio = 0.1;
    int componentCount = 32;
    std::vector<base::EntityId> sets;
};

} // namespace oor::model
