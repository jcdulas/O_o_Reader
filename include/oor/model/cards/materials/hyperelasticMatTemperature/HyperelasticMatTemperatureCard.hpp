#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// The hyperelastic model of a MATTHE card: the one of the MATHE it extends.
enum class HyperelasticMatTemperatureModel : std::uint8_t
{
    Mooney, // MOONEY: the generalised Mooney-Rivlin model (the default)
    Ogden,  // OGDEN
    Foam,   // FOAM (laid out like OGDEN)
    Aboyce, // ABOYCE: Arruda-Boyce
    Gent    // GENT
};

// One MATTHE card: the temperature dependence of the coefficients of a MATHE (a hyperelastic material of SOL 400). Each
// field holds the ID of a TABLEMi (0 when blank: the coefficient does not depend on the temperature) that gives the
// variation of the coefficient with the same place on the MATHE with the same MID.
//
// Card layout (the fields are numbered across the lines, eight to a line; the format reference draws three formats
// selected by MODEL).
//
// Format 1, MOONEY (the default; the lines after the second may be left out):
//   MATTHE  MID    MODEL  (blank) T(K)   T(RHO) T(TEXP) (blank) T(GE)         fields 1 to 8
//           T(C10) T(C01) T(D1)                                                9 to 11
//           T(C20) T(C11) T(C02)  T(D2)                                        17 to 20
//           T(C30) T(C21) T(C12)  T(C03) T(D3)                                 25 to 29
//           T(C40) T(C31) T(C22)  T(C13) T(C04) T(D4)                          33 to 38
//           T(C50) T(C41) T(C32)  T(C23) T(C14) T(C05) T(D5)                   41 to 47
// Format 2, OGDEN and FOAM:
//   MATTHE  MID    MODEL  (blank) T(K)   T(RHO) T(TEXP) (blank) T(GE)         1 to 8
//           T(D1)  T(D2)  T(D3)   T(D4)  T(D5)                                 9 to 13
//   (a common deck puts the five tables on the last line of the MATHE form, fields 33 to 37; both places are read and
//   the card is written in the place of the format.)
// Format 3, ABOYCE and GENT:
//   MATTHE  MID    MODEL  (blank) T(K)   T(RHO) T(TEXP) (blank) T(GE)         1 to 8
//           T(NKT) T(N/E) T(IM)                                                9 to 11
//           T(D1)  T(D2)  T(D3)   T(D4)  T(D5)                                 17 to 21
// T(IM) is for GENT only. What a model does not use stays 0 and is not written: a card that gives a table the model has
// no field for is an error. The Mooney-Rivlin tables are kept in `coefficientCTableIds` in the order T(C10) T(C01)
// T(C20) T(C11) T(C02) T(C30) T(C21) T(C12) T(C03) T(C40) T(C31) T(C22) T(C13) T(C04) T(C50) T(C41) T(C32) T(C23)
// T(C14) T(C05).
struct HyperelasticMatTemperatureCard
{
    base::EntityId id = 0; // the MATHE that the card extends
    HyperelasticMatTemperatureModel modelName = HyperelasticMatTemperatureModel::Mooney;
    base::EntityId bulkModulusTableId = 0;
    base::EntityId densityTableId = 0;
    base::EntityId thermalExpansionTableId = 0;
    base::EntityId structuralDampingTableId = 0;
    std::array<base::EntityId, 20> coefficientCTableIds{}; // MOONEY
    std::array<base::EntityId, 5> coefficientDTableIds{};  // T(D1) to T(D5): MOONEY, OGDEN, FOAM, ABOYCE and GENT
    base::EntityId rubberModulusTableId = 0;               // ABOYCE and GENT
    base::EntityId exponentOrModulusTableId = 0;
    base::EntityId invariantLimitTableId = 0; // GENT
};

} // namespace oor::model
