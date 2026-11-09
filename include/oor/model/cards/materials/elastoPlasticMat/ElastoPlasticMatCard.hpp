#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace oor::model
{

// The form of the stress-plastic strain function of a MATEP.
enum class ElastoPlasticMatForm : std::uint8_t
{
    Slope,  // SLOPE (the default): the yield stress Y0 and one work hardening slope H
    Table,  // TABLE: a table FID
    Perfect // PERFECT: a perfectly plastic material
};

// The yield criterion (RYIELD).
enum class ElastoPlasticMatYield : std::uint8_t
{
    VonMises, // VMISES (the default)
    Hill,     // HILL
    Barlat,   // BARLAT
    LinMohr,  // LINMOHR
    PblMohr,  // PBLMOHR
    ImpCreep  // IMPCREEP
};

// The hardening rule (WKHARD).
enum class ElastoPlasticMatHardening : std::uint8_t
{
    Isotropic,  // ISOTROP (the default)
    Kinematic,  // KINEM
    Combined,   // COMBINE
    Chaboche,   // CHABOCHE
    JohnsonCook // JHCOOK
};

// The option of the rate dependent group.
enum class ElastoPlasticMatRateOption : std::uint8_t
{
    Table, // TABLE (the default): the yield stress against strain rate is the table RTID
    Cowper // COWPER: the Cowper and Symonds model with the constants C and P
};

// The option of the pressure dependent group.
enum class ElastoPlasticMatPressureOption : std::uint8_t
{
    Linear,   // LIN (the default): linear Mohr-Coulomb, ALPHS
    Parabolic // PBL: parabolic Mohr-Coulomb, BETA
};

// The "Reffect" group: the rate dependent yield stress.
struct ElastoPlasticMatRateGroup
{
    ElastoPlasticMatRateOption option = ElastoPlasticMatRateOption::Table;
    base::EntityId strainRateTableId = 0; // TABLES1 of the yield stress against strain rate (TABLE)
    double cowperCoefficient = 1.0;       // Cowper and Symonds constants (COWPER)
    double cowperExponent = 1.0;
};

// The "Press" group: the pressure dependent yield criteria.
struct ElastoPlasticMatPressureGroup
{
    ElastoPlasticMatPressureOption option = ElastoPlasticMatPressureOption::Linear;
    std::optional<double> mohrCoulombAlpha; // the parameter alpha of the linear model
    std::optional<double> mohrCoulombBeta;  // the parameter beta of the parabolic model
};

// One MATEP card: the elasto-plastic properties of a material of SOL 400 (it extends a MAT1, MAT2, MATORT, MAT8 or
// MAT9).
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   MATEP  MID  FORM  Y0  FID  RYIELD  WKHARD  (blank)  H                         fields 1 to 8
// then the groups the card needs, each starting a line with its keyword in the first field of the line:
//   "REFFECT"   OPTION  RTID  C  P                                                  rate dependence
//   "ANISO"     (blank) R11/M R22/C1 R33/C2 R12/C3 R23/C6 R31                       Hill or Barlat anisotropy
//   "PRESS"     OPTION  ALPHS BETA                                                  pressure dependence
//   "CHABOCHE"  R0 RINF B C GAM KAP N                                               Chaboche hardening
//               (blank) QM MU ETA                                                   (second line, when it has values)
//   "JHCOOK"    A B N C M TMELT TROOM                                               Johnson-Cook hardening
//               (blank) EDOT                                                        (second line, when it has a value)
//   "IMPCREEP"  VMISES                                                              implicit creep
// The groups follow in any order, each once. The card keeps each as an optional value. A group goes with the model it
// is for (format reference: ANISO with RYIELD = HILL or BARLAT, PRESS with LINMOHR or PBLMOHR, CHABOCHE with WKHARD =
// CHABOCHE, JHCOOK with WKHARD = JHCOOK, IMPCREEP with RYIELD = IMPCREEP).
//
// Y0 is absent when blank, H is 0.0, FORM is SLOPE, RYIELD is VMISES and WKHARD is ISOTROP. FID is the TABLES1 (or the
// TABL3Di) of FORM = TABLE (it is blank otherwise). The anisotropy ratios are 1.0 when blank; a blank constant of the
// other groups is 0.0; a blank ALPHS, BETA or VMISES is absent.
struct ElastoPlasticMatCard
{
    base::EntityId id = 0; // the material that the card extends
    ElastoPlasticMatForm form = ElastoPlasticMatForm::Slope;
    std::optional<double> yieldStress;
    base::EntityId stressStrainTableId = 0;
    ElastoPlasticMatYield yieldCriterion = ElastoPlasticMatYield::VonMises;
    ElastoPlasticMatHardening workHardening = ElastoPlasticMatHardening::Isotropic;
    double hardeningSlope = 0.0;
    std::optional<ElastoPlasticMatRateGroup> rateDependence;
    std::optional<std::array<double, 6>>
        anisotropy; // R11 R22 R33 R12 R23 R31 (Hill) or M C1 C2 C3 C6 and a blank sixth (Barlat)
    std::optional<ElastoPlasticMatPressureGroup> pressureDependence;
    std::optional<std::array<double, 10>> chabocheHardening;   // R0 RINF B C GAM KAP N QM MU ETA
    std::optional<std::array<double, 8>> johnsonCookHardening; // A B N C M TMELT TROOM EDOT
    bool implicitCreep = false;
    std::optional<double> creepEquivalentStress; // the equivalent stress of the IMPCREEP group
};

} // namespace oor::model
