#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <limits>

namespace oor::model
{

// How a CREEP card gives the creep characteristics.
enum class CreepMatForm : std::uint8_t
{
    Blank, // FORM left blank: the creep law of TYPE and the coefficients a to g (the format reference names no
           // default)
    CrLaw, // CRLAW: an empirical creep law
    Table  // TABLE: the creep model parameters Kp, Cp and Cs as TABLES1 tables
};

// One CREEP card: the creep characteristics of a MAT1, MAT2 or MAT9, from an empirical creep law or from tables.
//
// Card layout:
//   CREEP  MID    T0    EXP   FORM   TIDKP   TIDCP   TIDCS   THRESH
//          TYPE   a     b     c      d       e       f       g
// T0 is the temperature at which the characteristics are defined (0.0 when blank), EXP the temperature dependent term
// of the creep rate (0 < EXP < 1, 1.0E-9 when blank) and THRESH the threshold of the creep process, a fraction of
// Young's modulus (0 < THRESH < 1.0E-3, 1.0E-5 when blank). With FORM = TABLE the three tables TIDKP, TIDCP and TIDCS
// of the parameters are TABLES1 cards and the second line must be blank; otherwise TYPE is one of
// 111, 112, 121, 122, 211, 212, 221, 222 and 300, the coefficients that the law does not use are blank (g
// for the types ending in 2; c, e, f and g for 300) and no table is given. The coefficients are kept in `coefficients`
// in the order a to g, NaN where blank.
struct CreepMatCard
{
    static constexpr double DEFAULT_EXPONENT = 1.0e-9;
    static constexpr double DEFAULT_THRESHOLD = 1.0e-5;
    static constexpr double BLANK = std::numeric_limits<double>::quiet_NaN(); // a coefficient left blank

    base::EntityId id = 0; // the MAT1, MAT2 or MAT9 that the card extends
    double referenceTemperature = 0.0;
    double exponent = DEFAULT_EXPONENT;
    CreepMatForm form = CreepMatForm::Blank;
    base::EntityId primaryStiffnessTableId = 0;
    base::EntityId primaryDampingTableId = 0;
    base::EntityId secondaryDampingTableId = 0;
    double threshold = DEFAULT_THRESHOLD;
    int kind = 0;                                                                           // 0 where blank
    std::array<double, 7> coefficients = {BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK}; // a b c d e f g
};

} // namespace oor::model
