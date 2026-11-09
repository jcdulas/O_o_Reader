#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// TYPE (field 2): what VALUE is, the specific gas constant of the gas or its molar weight. A blank TYPE is RSPEC. With
// the molar weight the model needs the universal gas constant, PARAM,UGASC (a rule of the model, not of the card).
enum class InflatorGasConstant : std::uint8_t
{
    SpecificGasConstant,
    MolarWeight
};
inline constexpr std::array<std::string_view, 2> INFLATOR_GAS_CONSTANT_WORDS{"ResponseSpectrum", "MolecularWeight"};

// CPGAS (field 4): how the specific heat at constant pressure varies: it is a constant, it follows a TABLED1 of the
// temperature, or it is a polynomial of the temperature. A blank CPGAS is CONSTANT.
enum class InflatorGasHeatLaw : std::uint8_t
{
    Constant,
    Table,
    Polynomial
};
inline constexpr std::array<std::string_view, 3> INFLATOR_GAS_HEAT_LAW_WORDS{"Constant", "Table", "Polynomial"};

// One INFLGAS card (SOL 700): a thermally ideal gas that an INFLTR, INFLHB or INITGAS names (INFLCG and INFLTR name it
// in their GASNAM field).
//
// Card layout: INFLGAS  GASNAM  TYPE  VALUE  CPGAS  V1  V2  V3  V4            fields 1 to 8
// (the common decks breaks the word CONSTANT in two over fields 4 and 5; it is the word in field 4 and V1 in 5.)
// GASNAM numbers the card, an integer although the format reference names it. VALUE is the number that TYPE says,
// greater than zero and required. V1 depends on CPGAS: the constant specific heat (CONSTANT), the number of a TABLED1
// (TABLE) or the first coefficient of the polynomial (POLY); it is required and greater than zero. V2, V3 and V4 are
// the other coefficients of the polynomial, 0.0 when blank, and belong to POLY only. The card keeps V1 in `v1` or
// `table` according to CPGAS and ignores the other one.
struct InflatorGasCard
{
    base::EntityId id = 0; // unique number of the gas
    InflatorGasConstant type = InflatorGasConstant::SpecificGasConstant;
    double constantValue = 0.0; // the gas constant or the molar weight, greater than zero
    InflatorGasHeatLaw heatLaw = InflatorGasHeatLaw::Constant;
    double heatCoefficient1 = 0.0;  // CONSTANT: the specific heat; POLY: the first coefficient
    base::EntityId heatTableId = 0; // TABLE: the TABLED1 of the specific heat against temperature
    double heatCoefficient2 = 0.0;  // POLY: the other coefficients
    double heatCoefficient3 = 0.0;
    double heatCoefficient4 = 0.0;
};

} // namespace oor::model
