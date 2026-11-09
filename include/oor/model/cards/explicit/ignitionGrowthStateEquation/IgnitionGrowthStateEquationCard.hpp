#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The unit systems that EOSIG names in UNITDEF (the system the inputs are written in) and UNITCNV (the system the
// parameters are converted to): centimeter-gram-microsecond, SI, metric (cm, g, s), imperial (inch, slug, s) and
// millimeter-milligram-microsecond. The format spells the first word CCGMS and the last MMMG in the list of UNITDEF,
// and CGMS and MMMGS in the list of UNITCNV and in its table of units: the card knows the second spellings.
enum class IgnitionGrowthStateEquationUnitSystem : std::uint8_t
{
    Cgms,
    Si,
    Metric,
    Imperial,
    Mmmgs
};
inline constexpr std::array<std::string_view, 5> IGNITION_GROWTH_UNIT_WORDS{
    "CentimeterGramMicrosecond", "SiUnits", "MetricUnits", "ImperialUnits", "MillimeterMilligramMicrosecond"};

// The explosives of the database that DBEXPL selects: PBX-9404 (two versions), TATB, PETN, cast TNT, the LANL and the
// military COMP B, and LX-17. NO (not used) is the default.
enum class IgnitionGrowthStateEquationDatabase : std::uint8_t
{
    NotUsed,
    Pbx9404A,
    Tatb,
    Petn,
    CastTnt,
    LanlCompB,
    MilitaryCompB,
    Pbx9404B,
    Lx17
};
inline constexpr std::array<std::string_view, 9> IGNITION_GROWTH_DATABASE_WORDS{"No",
                                                                                "Explosive94A",
                                                                                "TriaminotrinitrobenzeneData",
                                                                                "PentaShape",
                                                                                "TrinitrotolueneData",
                                                                                "LengthCompressionB",
                                                                                "MassCompressionB",
                                                                                "Explosive94B",
                                                                                "InsensitiveExplosive17"};

// The defaults of the format (static: a blank field and the default are the same card).
inline constexpr double IGNITION_GROWTH_BURN_EXPONENT_X_DEFAULT = 2.0 / 9.0;
inline constexpr double IGNITION_GROWTH_BURN_EXPONENT_Y_DEFAULT = 2.0 / 3.0;
inline constexpr double IGNITION_GROWTH_DENSITY_EXPONENT_DEFAULT = 0.4;
inline constexpr double IGNITION_GROWTH_PRESSURE_TOLERANCE_DEFAULT = 1.0E-6;
inline constexpr int IGNITION_GROWTH_MAX_ITERATIONS_DEFAULT = 16;

// One EOSIG card (SOL 700): the ignition and growth equation of state of a high explosive, with its reaction rate: a
// JWL form for the unreacted explosive (the constants end in E), another for the reaction products (the constants end
// in P), and the rate that turns one into the other. A MATDEUL names it in its EID field.
//
// Card layout (the continuation lines start at fields 9, 17 and 25):
//   EOSIG  EID  UNITDEF  DBEXPL  UNITCNV  (fields 5 to 9 blank)                  fields 1 to 4
//          AE  BE  R1E  R2E  OMGE  I  G                                           fields 10 to 16 (field 9 is blank)
//          A  AP  BP  R1P  R2P  OMGP  X  Y                                        fields 17 to 24
//          Z  R  ECHEM  PRSTOL  ITRMAX                                            fields 25 to 29
// DBEXPL names an explosive of the database (anything but NO): the values of the database then replace everything
// given in fields 10 to 29, so in that form the card may leave them blank (and the card keeps what is written there).
// With DBEXPL = NO, or blank, the fifteen constants AE to OMGP, Z and ECHEM are required: they are std::optional here
// so that the database form can leave them out, and check reports the first one missing. X, Y, R, PRSTOL and ITRMAX
// have the defaults above (the format prints X as 2/9 and Y as 2/3), stored resolved. The format gives no range for the
// constants (the database has a negative BE), so only PRSTOL and ITRMAX must be positive. UNITDEF is the system the
// inputs are written in. The format needs it only to convert (UNITCNV): with a database the data are in the CGMS system
// and UNITDEF is not needed, but a UNITCNV without a database needs it, so a card with a UNITCNV, no UNITDEF
// and DBEXPL = NO is refused. The density that the material of a MATDEUL gives is read in the unit system of UNITDEF,
// and with a database the density is the one of the database; none of this is checked here.
struct IgnitionGrowthStateEquationCard
{
    base::EntityId id = 0; // unique among the equations of state
    std::optional<IgnitionGrowthStateEquationUnitSystem> inputUnits;
    IgnitionGrowthStateEquationDatabase explosiveDatabase = IgnitionGrowthStateEquationDatabase::NotUsed;
    std::optional<IgnitionGrowthStateEquationUnitSystem> outputUnits;
    std::optional<double> unreactedA; // constants A, B, R1, R2 and omega of the unreacted explosive
    std::optional<double> unreactedB;
    std::optional<double> unreactedR1;
    std::optional<double> unreactedR2;
    std::optional<double> unreactedOmega;
    std::optional<double> ignitionCoefficient;        // first ignition coefficient
    std::optional<double> growthCoefficient;          // second ignition coefficient
    std::optional<double> ignitionDensityCoefficient; // density ignition coefficient
    std::optional<double> productA;                   // constants A, B, R1, R2 and omega of the reaction products
    std::optional<double> productB;
    std::optional<double> productR1;
    std::optional<double> productR2;
    std::optional<double> productOmega;
    double burnExponentX = IGNITION_GROWTH_BURN_EXPONENT_X_DEFAULT;    // surface burning exponent
    double burnExponentY = IGNITION_GROWTH_BURN_EXPONENT_Y_DEFAULT;    // surface burning exponent
    std::optional<double> pressureExponent;                            // pressure exponent
    double densityExponent = IGNITION_GROWTH_DENSITY_EXPONENT_DEFAULT; // relative density exponent
    std::optional<double> chemicalEnergy;                              // chemical energy of the explosive per unit mass
    double pressureTolerance = IGNITION_GROWTH_PRESSURE_TOLERANCE_DEFAULT; // tolerance of the pressure equilibrium
                                                                           // iterations, greater than zero
    int maxIterations =
        IGNITION_GROWTH_MAX_ITERATIONS_DEFAULT; // most iterations of the pressure equilibrium, greater than zero
};

} // namespace oor::model
