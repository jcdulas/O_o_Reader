#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace oor::model
{

// The hyperelastic model of a MATHE card.
enum class HyperelasticMatModel : std::uint8_t
{
    Mooney, // MOONEY: the generalised Mooney-Rivlin model (the default)
    Ogden,  // OGDEN
    Foam,   // FOAM
    Aboyce, // ABOYCE: Arruda-Boyce
    Gent,   // GENT
    Ghem1,  // GHEM1 to GHEM6: a user strain energy function, nothing else on the card
    Ghem2,
    Ghem3,
    Ghem4,
    Ghem5,
    Ghem6
};

// One MATHE card: a hyperelastic (rubber-like) material of SOL 400.
//
// Card layout (the fields are numbered across the lines, eight to a line; the format reference draws four formats
// selected by MODEL).
//
// Format 1, MOONEY (the default):
//   MATHE  MID    MODEL  (blank) K      RHO    TEXP   TREF   GE             fields 1 to 8
//          C10    C01    D1      TAB1   TAB2   TAB3   TAB4   TABD           9 to 16
//          C20    C11    C02     D2     NA     ND                           17 to 22
//          C30    C21    C12     C03    D3                                  25 to 29
//          C40    C31    C22     C13    C04    D4                           33 to 38
//          C50    C41    C32     C23    C14    C05    D5                    41 to 47
// Format 2, OGDEN and FOAM:
//   MATHE  MID    MODEL  NOT     K      RHO    TEXP   TREF   GE             1 to 8
//          MU1    ALPHA1 BETA1                                               9 to 11
//          MU2    ALPHA2 BETA2   MU3    ALPHA3 BETA3                        17 to 22
//          MU4    ALPHA4 BETA4   MU5    ALPHA5 BETA5                        25 to 30
//          D1     D2     D3      D4     D5                                   33 to 37
// Format 3, ABOYCE and GENT:
//   MATHE  MID    MODEL  (blank) K      RHO    TEXP   TREF   GE             1 to 8
//          NKT    N/E    IM                                                  9 to 11
//          D1     D2     D3      D4     D5                                   17 to 21
// Format 4, GHEM1 to GHEM6: the first line only.
//
// K is the bulk modulus: absent (blank) for the automatic nearly incompressible value, -1.0 for the volumetric strain
// energy of the Di. RHO, TEXP, TREF and GE are 0.0 when blank. What a model does not use stays at its default and is
// not written: a card that gives a value the model has no field for is an error. The constants of the Mooney-Rivlin
// model are kept in `coefficientsC` in the order C10 C01 C20 C11 C02 C30 C21 C12 C03 C40 C31 C22 C13 C04 C50 C41 C32
// C23 C14 C05; `coefficientsD` is D1 to D5 for MOONEY, OGDEN (not FOAM), ABOYCE and GENT. NA is 2 when blank, ND is 1,
// NOT is 1. The five tables are TABLES1 cards of the experiments from which the Cij and Di can be estimated (0 when
// blank). NKT and N/E are 1.0 when blank and IM (GENT only) is 0.0.
struct HyperelasticMatCard
{
    static constexpr int DEFAULT_DISTORTIONAL_ORDER = 2;

    base::EntityId id = 0;
    HyperelasticMatModel model = HyperelasticMatModel::Mooney;
    std::optional<double> bulkModulus;
    double density = 0.0;
    double thermalExpansion = 0.0;
    double referenceTemperature = 0.0;
    double structuralDamping = 0.0;
    // MOONEY
    std::array<double, 20> coefficientsC = {};
    base::EntityId tensionTableId = 0;
    base::EntityId equibiaxialTableId = 0;
    base::EntityId simpleShearTableId = 0;
    base::EntityId pureShearTableId = 0;
    base::EntityId volumetricTableId = 0;
    int distortionalOrder = DEFAULT_DISTORTIONAL_ORDER;
    int volumetricOrder = 1;
    // OGDEN and FOAM
    int termCount = 1;
    std::array<double, 5> ogdenModuli = {};
    std::array<double, 5> alpha = {};
    std::array<double, 5> beta = {}; // FOAM only
    // ABOYCE and GENT
    double rubberModulus = 1.0;
    double exponentOrModulus = 1.0;
    double invariantLimit = 0.0; // GENT only
    // MOONEY, OGDEN, ABOYCE and GENT
    std::array<double, 5> coefficientsD = {};
};

} // namespace oor::model
