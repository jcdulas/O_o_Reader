#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace oor::model
{

// The viscoelastic model of a MATVE card.
enum class ViscoElasticMatModel : std::uint8_t
{
    Iso,     // ISO: isotropic, the terms in the fixed layout of at most five (Gi, Tdi, Ki, Tvi)
    Iso1,    // ISO1 (the default; LINEAR is another name): isotropic, any number of terms, one line each
    Mooney,  // MOONEY: for a MATHE, the terms (Wdi, Tdi, Wvi, Tvi) in the fixed layout
    Ogden,   // OGDEN
    Foam,    // FOAM
    Mooney1, // MOONEY1: one line a term
    Ogden1,  // OGDEN1
    Foam1,   // FOAM1
    Ortho,   // ORTHO: orthotropic, two lines a term
    IsoLin   // ISOLIN: the linear isotropic model of SOL 700, one term (G0, GI, BETA, VISC)
};

// One MATVE card: the viscoelastic properties of a material of SOL 400 as a Prony series (it extends a MAT1, MAT3,
// MAT8, MATORT or MATHE).
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   ISO, MOONEY, OGDEN, FOAM (a fixed layout of up to five terms):
//     MATVE  MID    MODEL  ALPHAS  ALPHAL  G1     TD1    K1     TV1              fields 1 to 8
//            G2     TD2    G3      TD3     G4     TD4    G5     TD5              9 to 16
//            K2     TV2    K3      TV3     K4     TV4    K5     TV5              17 to 24
//     (for MOONEY, OGDEN and FOAM G is WD, the multiplier of the deviatoric term, and K is WV)
//   ISO1, MOONEY1, OGDEN1, FOAM1 (one line a term, as many as needed):
//     MATVE  MID    MODEL  ALPHAS  ALPHAL                                         1 to 4
//            G1     TD1    K1      TV1                                            9 to 12 (the next term 17 to 20, ...)
//   ORTHO (two lines a term):
//     MATVE  MID    ORTHO                                                         1 and 2
//            TD1    EXX1   EYY1    EZZ1    VXY1   VYZ1   VZX1                     9 to 15
//            GXY1   GYZ1   GZX1                                                   17 to 19 (the next term 25 to 31,
//            ...)
//
// ISOLIN (the SOL 700 form):
//     MATVE  MID    MODEL  G0      GI      BETA   VISC                            1 to 6
//     the short-time and long-time shear moduli, the decay constant and the shear viscosity; the card keeps them as the
//     single term (G0, GI, BETA, VISC) of `terms`, and has no ALPHAS and ALPHAL.
//
// ALPHAS and ALPHAL (the coefficients of thermal expansion of the solid and of the liquid) are 0.0 when blank and not
// used by ORTHO. Every blank value of a term is 0.0. `terms` keeps the terms of the models above ORTHO as (G or WD, TD,
// K or WV, TV), `orthoTerms` those of ORTHO as (TD, EXX, EYY, EZZ, VXY, VYZ, VZX, GXY, GYZ, GZX). The fixed layouts
// hold five terms at most; a card keeps the terms up to the last one that has a value.
struct ViscoElasticMatCard
{
    base::EntityId id = 0; // the material that the card extends
    ViscoElasticMatModel model = ViscoElasticMatModel::Iso1;
    double solidThermalExpansion = 0.0;
    double liquidThermalExpansion = 0.0;
    std::vector<std::array<double, 4>> terms;
    std::vector<std::array<double, 10>> orthoTerms;
};

} // namespace oor::model
