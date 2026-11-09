#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <optional>

namespace oor::model
{

// One MAT8 card: an orthotropic material for shell elements.
//
// Card layout:
//   MAT8  MID      E1    E2    NU12  G12   G1Z   G2Z   RHO
//         A1       A2    TREF  Xt    Xc    Yt    Yc    S
//         GE       F12   STRN
//         "HFAIL"  HF1   HF2   HF3   HF4   HF10  HF11
//         "HTAPE"  HT1   HT2   HT3   HT4   HT5   HT6   HT10
//                  HT11  HT12
//         "HFABR"  HFB1  HFB2  HFB3  HFB4  HFB5  HFB6  HFB10
//                  HFB11 HFB12
//
// Dependent defaults are stored resolved: Xc is Xt and Yc is Yt when blank, and in the three optional groups of
// Hashin failure criteria each "compressive" or second value is the one before it when blank. A group is absent (an
// empty optional) unless the card has its keyword. G1Z and G2Z blank mean no transverse shear flexibility, i.e. an
// infinite shear modulus: they stay empty, so that no number stands for them and a value given is written as given.
struct OrthoMat2DCard
{
    base::EntityId id = 0;
    double youngsModulus1 = 0.0; // moduli of elasticity in the fiber and in the matrix direction
    double youngsModulus2 = 0.0;
    double poissonRatio12 = 0.0;
    double shearModulus12 = 0.0;          // in-plane shear modulus
    std::optional<double> shearModulus1Z; // transverse shear moduli in the 1-Z and 2-Z planes; blank is infinite
    std::optional<double> shearModulus2Z;
    double density = 0.0;
    double thermalExpansion1 = 0.0; // thermal expansion coefficients
    double thermalExpansion2 = 0.0;
    double referenceTemperature = 0.0;
    double tensionStrength1 = 0.0; // allowables in tension and compression, longitudinal and lateral, and in shear
    double compressionStrength1 = 0.0;
    double tensionStrength2 = 0.0;
    double compressionStrength2 = 0.0;
    double shearStrength = 0.0;
    double structuralDamping = 0.0;
    double interactionTerm12 = 0.0; // interaction term of the Tsai-Wu theory
    double strainAllowables = 0.0;  // 1.0 when the allowables are strains, blank for stresses

    std::optional<std::array<double, 6>> hashinUnidirectional; // HF1 HF2 HF3 HF4 HF10 HF11
    std::optional<std::array<double, 9>> hashinTape;           // HT1 HT2 HT3 HT4 HT5 HT6 HT10 HT11 HT12
    std::optional<std::array<double, 9>> hashinFabric;         // HFB1 HFB2 HFB3 HFB4 HFB5 HFB6 HFB10 HFB11 HFB12
};

} // namespace oor::model
