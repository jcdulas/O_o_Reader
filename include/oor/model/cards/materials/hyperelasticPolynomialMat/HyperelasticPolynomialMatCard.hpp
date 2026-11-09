#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One MATHP card: a hyperelastic (rubber-like) material, the generalised Mooney-Rivlin strain energy function.
//
// Card layout (the fields are numbered across the lines, eight to a line; the blank cells are drawn in the format
// reference):
//   MATHP  MID    A10    A01    D1     RHO    AV     TREF   GE        fields 1 to 8
//          (blank) NA    ND                                           9 to 11
//          A20    A11    A02    D2                                    17 to 20
//          A30    A21    A12    A03    D3                             25 to 29
//          A40    A31    A22    A13    A04    D4                      33 to 38
//          A50    A41    A32    A23    A14    A05    D5               41 to 47
//          TAB1   TAB2   TAB3   TAB4   (blank)(blank)(blank) TABD     49 to 56
// A Mooney-Rivlin or neo-Hookean material needs no continuation line.
//
// D1 depends on A10 and A01 when it is blank (10^3 (A10 + A01)): it is stored resolved and always
// written. The other blank values are 0.0, NA and ND are 1 when blank. The constants of the higher orders are kept in
// two arrays in the order of the card: `coefficientsA` has A20 A11 A02 A30 A21 A12 A03 A40 A31 A22 A13 A04 A50 A41 A32
// A23 A14 A05 and `coefficientsD` has D2 D3 D4 D5. A table ID is 0 when blank; the tables are TABLES1 cards of the
// experiments from which the constants can be estimated (they replace the constants given on the card).
struct HyperelasticPolynomialMatCard
{
    base::EntityId id = 0;
    double coefficientA10 = 0.0;
    double coefficientA01 = 0.0;
    double coefficientD1 = 0.0;
    double density = 0.0;          // mass density
    double thermalExpansion = 0.0; // coefficient of volumetric thermal expansion
    double referenceTemperature = 0.0;
    double structuralDamping = 0.0;
    int distortionalOrder = 1; // orders of the distortional and of the volumetric strain energy polynomials
    int volumetricOrder = 1;
    std::array<double, 18> coefficientsA =
        {};                                   // A20 A11 A02 A30 A21 A12 A03 A40 A31 A22 A13 A04 A50 A41 A32 A23 A14 A05
    std::array<double, 4> coefficientsD = {}; // D2 D3 D4 D5
    base::EntityId tensionTableId = 0;        // TABLES1 of simple tension/compression data
    base::EntityId equibiaxialTableId = 0;    // equibiaxial tension
    base::EntityId simpleShearTableId = 0;    // simple shear
    base::EntityId pureShearTableId = 0;      // pure shear
    base::EntityId volumetricTableId = 0;     // pure volumetric compression
};

} // namespace oor::model
