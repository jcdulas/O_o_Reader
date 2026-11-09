#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace oor::model
{

// How the forces received from the co-simulation partner are applied.
enum class CosimulationForceMode : std::uint8_t
{
    NoFollow, // as received, over the whole increment (the default)
    Follow,   // as follower forces
};

// The words of FOLFORCE, in the order of the enumeration.
inline constexpr std::array<std::string_view, 2> COSIMULATION_FORCE_WORDS{"NoFollower", "Follower"};

// One COSMSRV card (SOL 400): a co-simulation service, with the coupled areas and the physical quantities it
// exchanges. A COSMSEL names it by its CSRVID.
//
// Card layout: COSMSRV  CSRVID  SERV  (blank)  FOLFORCE                         fields 1, 2 and 4
//             +        GRPID1  PHYINP  GRPID2  PHYOUT                           fields 9 to 12
// SERV is required and is kept as the deck wrote it (any characters are acceptable, upper or lower
// case or mixed). FOLFORCE is NOFOLL when blank. The four IDs are positive integers: GRPID1 is the
// COSMGRP that receives the partner's input, PHYINP the COSMINP that says what it receives, GRPID2 the COSMGRP that
// gives the output and PHYOUT the COSMOUT that says what it gives. The format asks for GRPID1 equal to GRPID2 today: a
// rule between fields that the check of the model reports (a card with two different areas is still read). The CSRVID
// is unique among the COSMSRV cards (IdSpace::PerCardType, category CosimulationService).
struct CosimulationServiceCard
{
    static constexpr CosimulationForceMode DEFAULT_FORCE_MODE = CosimulationForceMode::NoFollow;

    base::EntityId id = 0;
    std::string serviceName;
    CosimulationForceMode followerForces = DEFAULT_FORCE_MODE;
    base::EntityId inputAreaId = 0;
    base::EntityId inputId = 0;
    base::EntityId outputAreaId = 0;
    base::EntityId outputId = 0;
};

} // namespace oor::model
