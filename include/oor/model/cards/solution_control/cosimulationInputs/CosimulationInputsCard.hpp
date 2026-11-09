#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

// A physical quantity that the co-simulation partner gives the solver: forces, or temperatures as a temperature load.
enum class CosimulationInputQuantity : std::uint8_t
{
    Force,
    Temp,
};

// The words of PHYQUAi, in the order of the enumeration.
inline constexpr std::array<std::string_view, 2> COSIMULATION_INPUT_WORDS{"Force", "Temperature"};

// One COSMINP card (SOL 400): the physical quantities of input that the solver receives from the co-simulation partner.
// A COSMSRV names it in its PHYINP.
//
// Card layout: COSMINP  PHYIN                          field 1, fields 2 to 8 blank
//             +        PHYQUA1  PHYQUA2               fields 9 and 10
// PHYQUA1 is FORCE when blank (the format's default for a quantity); PHYQUA2 is blank when there is no second quantity.
// The format names FORCE and TEMP only; a quantity given twice has no meaning and is refused (the format does
// not say so: it lists the quantities as a set). The PHYIN is unique among the COSMINP cards (IdSpace::PerCardType,
// category CosimulationInput).
struct CosimulationInputsCard
{
    base::EntityId id = 0;
    std::vector<CosimulationInputQuantity> quantities{CosimulationInputQuantity::Force}; // one or two, distinct
};

} // namespace oor::model
