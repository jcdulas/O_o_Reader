#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

// A physical quantity that the solver gives the co-simulation partner: the displacement, velocity or acceleration of
// the coupled area, always in the basic coordinate system.
enum class CosimulationOutputQuantity : std::uint8_t
{
    Disp,
    Velo,
    Acce,
};

// The words of PHYQUAi, in the order of the enumeration.
inline constexpr std::array<std::string_view, 3> COSIMULATION_OUTPUT_WORDS{"Displacement", "Velocity", "Acceleration"};

// One COSMOUT card (SOL 400): the physical quantities of output that the solver provides to the co-simulation partner.
// A COSMSRV names it in its PHYOUT.
//
// Card layout: COSMOUT  PHYOUT                                   field 1, fields 2 to 8 blank
//             +        PHYQUA1  PHYQUA2  PHYQUA3                fields 9 to 11
// One or any combination of DISP, VELO and ACCE, in the order the deck gives them. The format gives no default, so at
// least one is required; a quantity given twice is not a combination and is refused (the format lists the three as a
// set). A blank field between two quantities is skipped. The PHYOUT is unique among the COSMOUT cards
// (IdSpace::PerCardType, category CosimulationOutput).
struct CosimulationOutputsCard
{
    base::EntityId id = 0;
    std::vector<CosimulationOutputQuantity> quantities; // one to three, distinct
};

} // namespace oor::model
