// Checks of the equation of state and shear model cards of the explicit solver: EOSDEF, EOSIG,
// EOSJWL, EOSMG, EOSNA, EOSPOL, EOSUDS, SHREL, SHRPOL and SHRUDS. Per card: the fields in their format reference
// positions, the defaults, the refused values and the round trip. The cards refer to no other card; what they share
// (two ID spaces that a MATDEUL names) is tested at the end.

#include "oor/model/cards/explicit/deflagrationStateEquation/DeflagrationStateEquationTable.hpp"
#include "oor/model/cards/explicit/elasticShearModel/ElasticShearModelTable.hpp"
#include "oor/model/cards/explicit/ignitionGrowthStateEquation/IgnitionGrowthStateEquationTable.hpp"
#include "oor/model/cards/explicit/jonesWilkinsLeeStateEquation/JonesWilkinsLeeStateEquationTable.hpp"
#include "oor/model/cards/explicit/mieGruneisenStateEquation/MieGruneisenStateEquationTable.hpp"
#include "oor/model/cards/explicit/nobleAbelStateEquation/NobleAbelStateEquationTable.hpp"
#include "oor/model/cards/explicit/polynomialShearModel/PolynomialShearModelTable.hpp"
#include "oor/model/cards/explicit/polynomialStateEquation/PolynomialStateEquationTable.hpp"
#include "oor/model/cards/explicit/userShearModel/UserShearModelTable.hpp"
#include "oor/model/cards/explicit/userStateEquation/UserStateEquationTable.hpp"
#include "oor/model/cards/materials/eulerMat/EulerMatTable.hpp"
#include "support/ExplicitTestSupport.hpp"
#include "support/SlotRecord.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The record of a card from the text of each of its slots (slot 1 first, slotRecord). Slots blank at the end are
// left out.
std::string freeCard(const std::string& name, std::vector<std::string> fields)
{
    while (!fields.empty() && fields.back().empty())
        fields.pop_back();
    return slotRecord(name, fields);
}

// The fields of a valid card with one of them replaced (the card grows when the field is beyond its end).
std::string withField(const std::string& name, std::vector<std::string> fields, int field, const std::string& value)
{
    if (fields.size() < static_cast<std::size_t>(field))
        fields.resize(static_cast<std::size_t>(field));
    fields[static_cast<std::size_t>(field - 1)] = value;
    return freeCard(name, fields);
}

const std::vector<std::string> DEFLAGRATION_FIELDS{"11",    "1.2", "0.001", "",    "",      "",    "1.E6",
                                                   "1550.", "",    "3.E-8", "0.8", "5400.", "0.6", "0.1"};

// An EOSIG with the constants of the unreacted explosive in fields 10 to 16, of the products in 17 to 22 and Z and
// ECHEM in 25 and 27, which is the card that gives its constants (no database).
const std::vector<std::string> IGNITION_GROWTH_FIELDS{
    "21",  "",     "",     "",    "",     "",    "",    "",    "", "650.", "-0.04", "10.", "1.1", "0.85",
    "38.", "350.", "0.01", "4.8", "0.07", "4.2", "1.1", "0.3", "", "",     "2.",    "",    "0.05"};

const std::vector<std::string> JONES_WILKINS_LEE_FIELDS{"31",   "5.E11", "7.E10", "4.",  "1.", "0.3",
                                                        "1.E5", "1.E3",  "1.E-4", "0.5", "0.7"};

const std::vector<std::string> POLYNOMIAL_STATE_FIELDS{"61",  "8.E7", "1.E8", "2.E8", "0.3",
                                                       "0.4", "0.5",  "0.6",  "1.3",  "2.E-3"};

template <typename TableT>
void requireNoReferences(Model& model)
{
    RecordingVisitor visitor;
    model.table<TableT>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
}

} // namespace

TEST_CASE("an EOSDEF has its fields over two lines in the order of the format reference", "[model][cards][explicit]")
{
    Model model = readBulk(
        "DeflagrationStateEquation;11;1.2;0.001;;;;1.E+6;1550.;0.2;3.E-8;0.8;5400.;0.6;0.\n"
        "DeflagrationStateEquation;12;1.25;0.0012;300.;700.;1.E+3;1.1E+6;1650.;;5.E-8;0.9;5500.;0.5;0.3");
    const DeflagrationStateEquationCard first = cardAt<DeflagrationStateEquationTable>(model, 0);
    REQUIRE((first.id == 11 && first.gammaCoefficient == 1.2 && first.coVolume == 0.001));
    REQUIRE((!first.gasConstant && !first.specificHeatVolume &&
             !first.specificHeatPressure)); // the gas constants are the ones the card does not give
    REQUIRE((first.chemicalEnergy == 1.0E6 && first.referenceDensity == 1550.0 && first.powderDensityRatio == 0.2));
    REQUIRE((first.burnRateCoefficient == 3.0E-8 && first.burnRateExponent == 0.8 &&
             first.surfaceToVolumeRatio == 5400.0 && first.formParameterX == 0.6 && first.formParameterY == 0.0));
    const DeflagrationStateEquationCard second = cardAt<DeflagrationStateEquationTable>(model, 1);
    REQUIRE((second.gasConstant == 300.0 && second.specificHeatVolume == 700.0 &&
             second.specificHeatPressure == 1000.0 && !second.powderDensityRatio));
    REQUIRE((second.burnRateCoefficient == 5.0E-8 && second.formParameterY == 0.3));
    requireNoReferences<DeflagrationStateEquationTable>(model);
    requireRoundTrip(model);

    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 1, "0"), "id");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 2, "0."), "gammaCoefficient");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 2, ""), "gammaCoefficient"); // required
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 3, "-1."), "coVolume");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 4, "0."), "gasConstant");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 5, "-1."), "specificHeatVolume");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 6, "0."), "specificHeatPressure");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 7, "0."), "chemicalEnergy");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 8, ""), "referenceDensity");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 9, "0."), "powderDensityRatio");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 10, "-1.E-8"), "burnRateCoefficient");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 11, "-0.1"), "burnRateExponent");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 12, "0."), "surfaceToVolumeRatio");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 13, "0."), "formParameterX");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 14, "-0.1"), "formParameterY");
    expectInvalid(withField("DeflagrationStateEquation", DEFLAGRATION_FIELDS, 13, "FAST"), "formParameterX");
}

TEST_CASE("an EOSIG is an explosive of the database or a list of constants over four lines", "[model][cards][explicit]")
{
    Model model = readBulk(
        "IgnitionGrowthStateEquation;20;;Explosive94A;SiUnits\nIgnitionGrowthStateEquation;23;ImperialUnits;"
        "InsensitiveExplosive17;"
        "MillimeterMilligramMicrosecond\nIgnitionGrowthStateEquation;24;;TriaminotrinitrobenzeneData;;650.;-0.04\n" +
        freeCard("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS) + "\n" +
        "IgnitionGrowthStateEquation;22;CentimeterGramMicrosecond;No;SiUnits;650.;-0.04;10.;1.1;0.85;38.;350.;0.01;4.8;"
        "0.07;4.2;1.1;0.3;0.25;0.7;"
        "2.;3.;0.05;1.E-5;20");

    // The database form: the explosive and the unit system the data are converted to, nothing else is required.
    const IgnitionGrowthStateEquationCard database = cardAt<IgnitionGrowthStateEquationTable>(model, 0);
    REQUIRE((database.id == 20 && !database.inputUnits &&
             database.explosiveDatabase == IgnitionGrowthStateEquationDatabase::Pbx9404A));
    REQUIRE(database.outputUnits == IgnitionGrowthStateEquationUnitSystem::Si);
    REQUIRE((!database.unreactedA && !database.pressureExponent && !database.chemicalEnergy));
    REQUIRE((database.burnExponentX == 2.0 / 9.0 && database.burnExponentY == 2.0 / 3.0 &&
             database.densityExponent == 0.4)); // the defaults of the format
    REQUIRE((database.pressureTolerance == 1.0E-6 && database.maxIterations == 16));
    const IgnitionGrowthStateEquationCard units = cardAt<IgnitionGrowthStateEquationTable>(model, 1);
    REQUIRE((units.inputUnits == IgnitionGrowthStateEquationUnitSystem::Imperial &&
             units.explosiveDatabase == IgnitionGrowthStateEquationDatabase::Lx17));
    REQUIRE(units.outputUnits == IgnitionGrowthStateEquationUnitSystem::Mmmgs);
    // Values given with a database are kept (the database replaces them in the solver).
    const IgnitionGrowthStateEquationCard overridden = cardAt<IgnitionGrowthStateEquationTable>(model, 2);
    REQUIRE((overridden.explosiveDatabase == IgnitionGrowthStateEquationDatabase::Tatb &&
             overridden.unreactedA == 650.0 && overridden.unreactedB == -0.04));

    // The explicit form: every constant in its field.
    const IgnitionGrowthStateEquationCard explicitCard = cardAt<IgnitionGrowthStateEquationTable>(model, 3);
    REQUIRE((explicitCard.id == 21 && explicitCard.explosiveDatabase == IgnitionGrowthStateEquationDatabase::NotUsed &&
             !explicitCard.inputUnits));
    REQUIRE((explicitCard.unreactedA == 650.0 && explicitCard.unreactedB == -0.04 && explicitCard.unreactedR1 == 10.0 &&
             explicitCard.unreactedR2 == 1.1));
    REQUIRE((explicitCard.unreactedOmega == 0.85 && explicitCard.ignitionCoefficient == 38.0 &&
             explicitCard.growthCoefficient == 350.0 && explicitCard.ignitionDensityCoefficient == 0.01));
    REQUIRE((explicitCard.productA == 4.8 && explicitCard.productB == 0.07 && explicitCard.productR1 == 4.2 &&
             explicitCard.productR2 == 1.1));
    REQUIRE((explicitCard.productOmega == 0.3 && explicitCard.pressureExponent == 2.0 &&
             explicitCard.chemicalEnergy == 0.05));
    REQUIRE((explicitCard.burnExponentX == 2.0 / 9.0 && explicitCard.burnExponentY == 2.0 / 3.0 &&
             explicitCard.densityExponent == 0.4)); // left blank
    REQUIRE((explicitCard.pressureTolerance == 1.0E-6 && explicitCard.maxIterations == 16));
    const IgnitionGrowthStateEquationCard full = cardAt<IgnitionGrowthStateEquationTable>(model, 4);
    REQUIRE((full.inputUnits == IgnitionGrowthStateEquationUnitSystem::Cgms &&
             full.explosiveDatabase == IgnitionGrowthStateEquationDatabase::NotUsed));
    REQUIRE((full.burnExponentX == 0.25 && full.burnExponentY == 0.7 && full.pressureExponent == 2.0 &&
             full.densityExponent == 3.0 && full.chemicalEnergy == 0.05));
    REQUIRE((full.pressureTolerance == 1.0E-5 && full.maxIterations == 20));
    requireNoReferences<IgnitionGrowthStateEquationTable>(model);
    requireRoundTrip(model);

    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 1, "0"), "id");
    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 2, "KELVIN"), "inputUnits");
    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 3, "DYNAMITE"), "explosiveDatabase");
    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 4, "KELVIN"), "outputUnits");
    // a conversion needs the unit of the data
    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 4, "SiUnits"), "inputUnits");
    expectInvalid("IgnitionGrowthStateEquation;20;;;SiUnits", "inputUnits");
    // Without a database every constant is required, one by one in the order of the card.
    const std::vector<std::pair<int, std::string>> required{
        {10, "unreactedA"},        {11, "unreactedB"},
        {12, "unreactedR1"},       {13, "unreactedR2"},
        {14, "unreactedOmega"},    {15, "ignitionCoefficient"},
        {16, "growthCoefficient"}, {17, "ignitionDensityCoefficient"},
        {18, "productA"},          {19, "productB"},
        {20, "productR1"},         {21, "productR2"},
        {22, "productOmega"},      {25, "pressureExponent"},
        {27, "chemicalEnergy"}};
    for (const auto& [field, name] : required)
        expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, field, ""), name);
    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 10, "ONE"), "unreactedA");
    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 23, "FAST"), "burnExponentX");
    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 28, "0."), "pressureTolerance");
    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 28, "-1.E-6"), "pressureTolerance");
    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 29, "0"), "maxIterations");
    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 29, "-3"), "maxIterations");
    expectInvalid(withField("IgnitionGrowthStateEquation", IGNITION_GROWTH_FIELDS, 29, "1.5"), "maxIterations");
}

TEST_CASE("an EOSJWL has the pressure constants on the first line and the afterburning rate on the second",
          "[model][cards][explicit]")
{
    Model model =
        readBulk("JonesWilkinsLeeStateEquation;35\nJonesWilkinsLeeStateEquation;36;5.E+11;7.E+10;4.;1.;0.3\n" +
                 freeCard("JonesWilkinsLeeStateEquation", JONES_WILKINS_LEE_FIELDS));
    const JonesWilkinsLeeStateEquationCard bare = cardAt<JonesWilkinsLeeStateEquationTable>(model, 0);
    REQUIRE((bare.id == 35 && bare.coefficientA == 0.0 && bare.coefficientB == 0.0 && bare.coefficientR1 == 0.0 &&
             bare.coefficientR2 == 0.0 && bare.omega == 0.0));
    REQUIRE((bare.initialPressure == 0.0 && bare.afterburnEnergy == 0.0 && bare.afterburnRate == 0.0 &&
             bare.coefficientM == 0.0 && bare.coefficientN == 0.0)); // no afterburning
    const JonesWilkinsLeeStateEquationCard constants = cardAt<JonesWilkinsLeeStateEquationTable>(model, 1);
    REQUIRE((constants.coefficientA == 5.0E11 && constants.coefficientB == 7.0E10 && constants.coefficientR1 == 4.0 &&
             constants.coefficientR2 == 1.0));
    REQUIRE((constants.omega == 0.3 && constants.initialPressure == 0.0));
    const JonesWilkinsLeeStateEquationCard full = cardAt<JonesWilkinsLeeStateEquationTable>(model, 2);
    REQUIRE((full.initialPressure == 1.0E5 && full.afterburnEnergy == 1.0E3));
    REQUIRE((full.afterburnRate == 1.0E-4 && full.coefficientM == 0.5 && full.coefficientN == 0.7)); // fields 9 to 11
    requireNoReferences<JonesWilkinsLeeStateEquationTable>(model);
    requireRoundTrip(model);

    expectInvalid(withField("JonesWilkinsLeeStateEquation", JONES_WILKINS_LEE_FIELDS, 1, "0"), "id");
    expectInvalid(withField("JonesWilkinsLeeStateEquation", JONES_WILKINS_LEE_FIELDS, 2, "BIG"), "coefficientA");
    expectInvalid(withField("JonesWilkinsLeeStateEquation", JONES_WILKINS_LEE_FIELDS, 8, "-1."), "afterburnEnergy");
    expectInvalid(withField("JonesWilkinsLeeStateEquation", JONES_WILKINS_LEE_FIELDS, 9, "RATE"), "afterburnRate");
    expectInvalid(withField("JonesWilkinsLeeStateEquation", JONES_WILKINS_LEE_FIELDS, 10, "HALF"), "coefficientM");
    expectInvalid(withField("JonesWilkinsLeeStateEquation", JONES_WILKINS_LEE_FIELDS, 11, "SEVEN"), "coefficientN");
}

TEST_CASE("an EOSMG has four required reals", "[model][cards][explicit]")
{
    Model model =
        readBulk("MieGruneisenStateEquation;41;3.E+3;1.6;2.;5.E+3\nMieGruneisenStateEquation;42;2000;2;1;3000");
    const MieGruneisenStateEquationCard first = cardAt<MieGruneisenStateEquationTable>(model, 0);
    REQUIRE((first.id == 41 && first.soundSpeed == 3000.0 && first.shockSlope == 1.6 &&
             first.gruneisenParameter == 2.0 && first.densityCutoff == 5000.0));
    const MieGruneisenStateEquationCard integers = cardAt<MieGruneisenStateEquationTable>(model, 1);
    REQUIRE((integers.soundSpeed == 2000.0 && integers.shockSlope == 2.0 && integers.gruneisenParameter == 1.0 &&
             integers.densityCutoff == 3000.0));
    requireNoReferences<MieGruneisenStateEquationTable>(model);
    requireRoundTrip(model);

    expectInvalid("MieGruneisenStateEquation;0;3.E+3;1.6;2.;5.E+3", "id");
    expectInvalid("MieGruneisenStateEquation;41;;1.6;2.;5.E+3", "soundSpeed");
    expectInvalid("MieGruneisenStateEquation;41;3.E+3;;2.;5.E+3", "shockSlope");
    expectInvalid("MieGruneisenStateEquation;41;3.E+3;1.6;;5.E+3", "gruneisenParameter");
    expectInvalid("MieGruneisenStateEquation;41;3.E+3;1.6;2.", "densityCutoff");
    expectInvalid("MieGruneisenStateEquation;41;3.E+3;1.6;2.;CUT", "densityCutoff");
}

TEST_CASE("an EOSNA has a gamma, a co-volume that is zero when blank and the gas constants", "[model][cards][explicit]")
{
    Model model = readBulk(
        "NobleAbelStateEquation;51;1.2\nNobleAbelStateEquation;52;1.24;0.001;314.5\nNobleAbelStateEquation;53;1.3;0."
        "002;300.;700.;1.E+3\nNobleAbelStateEquation;54;0.");
    const NobleAbelStateEquationCard bare = cardAt<NobleAbelStateEquationTable>(model, 0);
    REQUIRE((bare.id == 51 && bare.gammaCoefficient == 1.2 && bare.coVolume == 0.0 && !bare.gasConstant &&
             !bare.specificHeatVolume && !bare.specificHeatPressure));
    const NobleAbelStateEquationCard gas = cardAt<NobleAbelStateEquationTable>(model, 1);
    REQUIRE(
        (gas.coVolume == 0.001 && gas.gasConstant == 314.5 && !gas.specificHeatVolume && !gas.specificHeatPressure));
    const NobleAbelStateEquationCard full = cardAt<NobleAbelStateEquationTable>(model, 2);
    REQUIRE((full.coVolume == 0.002 && full.gasConstant == 300.0 && full.specificHeatVolume == 700.0 &&
             full.specificHeatPressure == 1000.0));
    REQUIRE(cardAt<NobleAbelStateEquationTable>(model, 3).gammaCoefficient == 0.0); // the format allows zero
    requireNoReferences<NobleAbelStateEquationTable>(model);
    requireRoundTrip(model);

    expectInvalid("NobleAbelStateEquation;0;1.2", "id");
    expectInvalid("NobleAbelStateEquation;51", "gammaCoefficient"); // required
    expectInvalid("NobleAbelStateEquation;51;-1.2", "gammaCoefficient");
    expectInvalid("NobleAbelStateEquation;51;1.2;-0.001", "coVolume");
    expectInvalid("NobleAbelStateEquation;51;1.2;0.;0.", "gasConstant");
    expectInvalid("NobleAbelStateEquation;51;1.2;0.;1.;0.", "specificHeatVolume");
    expectInvalid("NobleAbelStateEquation;51;1.2;0.;1.;1.;-1.", "specificHeatPressure");
}

TEST_CASE("an EOSPOL has zero coefficients by default, a volume limit of 1.1 and a viscosity",
          "[model][cards][explicit]")
{
    Model model = readBulk("PolynomialStateEquation;65\nPolynomialStateEquation;66;8.E+7\n" +
                           freeCard("PolynomialStateEquation", POLYNOMIAL_STATE_FIELDS));
    const PolynomialStateEquationCard bare = cardAt<PolynomialStateEquationTable>(model, 0);
    REQUIRE((bare.id == 65 && bare.coefficientA1 == 0.0 && bare.coefficientA2 == 0.0 && bare.coefficientA3 == 0.0 &&
             bare.coefficientB0 == 0.0 && bare.coefficientB1 == 0.0));
    REQUIRE(
        (bare.coefficientB2 == 0.0 && bare.coefficientB3 == 0.0 && bare.volumeLimit == 1.1 && bare.viscosity == 0.0));
    REQUIRE(cardAt<PolynomialStateEquationTable>(model, 1).coefficientA1 == 8.0E7);
    const PolynomialStateEquationCard full = cardAt<PolynomialStateEquationTable>(model, 2);
    REQUIRE((full.coefficientA1 == 8.0E7 && full.coefficientA2 == 1.0E8 && full.coefficientA3 == 2.0E8));
    REQUIRE((full.coefficientB0 == 0.3 && full.coefficientB1 == 0.4 && full.coefficientB2 == 0.5 &&
             full.coefficientB3 == 0.6));
    REQUIRE((full.volumeLimit == 1.3 && full.viscosity == 2.0E-3)); // the second line
    requireNoReferences<PolynomialStateEquationTable>(model);
    requireRoundTrip(model);

    expectInvalid(withField("PolynomialStateEquation", POLYNOMIAL_STATE_FIELDS, 1, "0"), "id");
    expectInvalid(withField("PolynomialStateEquation", POLYNOMIAL_STATE_FIELDS, 2, "STIFF"), "coefficientA1");
    expectInvalid(withField("PolynomialStateEquation", POLYNOMIAL_STATE_FIELDS, 8, "SOFT"), "coefficientB3");
    expectInvalid(withField("PolynomialStateEquation", POLYNOMIAL_STATE_FIELDS, 9, "1.0"),
                  "volumeLimit"); // it must exceed 1
    expectInvalid(withField("PolynomialStateEquation", POLYNOMIAL_STATE_FIELDS, 9, "0.5"), "volumeLimit");
    expectInvalid(withField("PolynomialStateEquation", POLYNOMIAL_STATE_FIELDS, 10, "THICK"), "viscosity");
}

TEST_CASE("an EOSUDS and a SHRUDS name a service group and a routine of at most eight characters",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "UserStateEquation;71;WATER\n"
        "UserStateEquation;72;WATER;ExternalStateEquation;0.01\n"
        "UserStateEquation;73;mygas;MYEOS;0.\n"
        "UserShearModel;271;GRP\n"
        "UserShearModel;272;mymat;ExternalShear");
    const UserStateEquationCard blankRoutine = cardAt<UserStateEquationTable>(model, 0);
    REQUIRE((blankRoutine.id == 71 && blankRoutine.groupName == "WATER" &&
             blankRoutine.userRoutineName == "ExternalStateEquation" && !blankRoutine.viscosity));
    REQUIRE(cardAt<UserStateEquationTable>(model, 1).viscosity == 0.01);
    const UserStateEquationCard other = cardAt<UserStateEquationTable>(model, 2);
    REQUIRE((other.groupName == "mygas" && other.userRoutineName == "MYEOS" &&
             other.viscosity == 0.0)); // words are kept as written
    const UserShearModelCard shear = cardAt<UserShearModelTable>(model, 0);
    REQUIRE((shear.id == 271 && shear.groupName == "GRP" && shear.userRoutineName == "ExternalShear"));
    REQUIRE(cardAt<UserShearModelTable>(model, 1).groupName == "mymat");
    // The default routine is stored and written, so the file says which routine the model uses.

    requireNoReferences<UserStateEquationTable>(model);
    requireNoReferences<UserShearModelTable>(model);
    requireRoundTrip(model);

    expectInvalid("UserStateEquation;0;WATER", "id");
    expectInvalid("UserStateEquation;71", "groupName"); // required
    oor::testing::requireModelFileRoundTrip(readModelText("UserStateEquation;71;WATERWORLD", true));
    oor::testing::requireModelFileRoundTrip(readModelText("UserStateEquation;71;WATER;EXEOSROUTINE", true));
    expectInvalid("UserStateEquation;71;WATER;ExternalStateEquation;-1.", "viscosity");
    expectInvalid("UserStateEquation;71;WATER;ExternalStateEquation;THIN", "viscosity");
    expectInvalid("UserShearModel;0;GRP", "id");
    expectInvalid("UserShearModel;271", "groupName");
    oor::testing::requireModelFileRoundTrip(readModelText("UserShearModel;271;GROUPNAME1", true));
    oor::testing::requireModelFileRoundTrip(readModelText("UserShearModel;271;GRP;ROUTINENAME", true));
}

TEST_CASE("a SHREL and a SHRPOL have reals that are zero when blank", "[model][cards][explicit]")
{
    Model model = readBulk(
        "ElasticShearModel;251\nElasticShearModel;252;8.E+7\nPolynomialShearModel;261\nPolynomialShearModel;262;8.E+"
        "7\nPolynomialShearModel;263;8.E+7;1.E+6;-2.E+5;3.E+4");
    REQUIRE((cardAt<ElasticShearModelTable>(model, 0).id == 251 &&
             cardAt<ElasticShearModelTable>(model, 0).shearModulus == 0.0));
    REQUIRE(cardAt<ElasticShearModelTable>(model, 1).shearModulus == 8.0E7);
    const PolynomialShearModelCard bare = cardAt<PolynomialShearModelTable>(model, 0);
    REQUIRE((bare.id == 261 && bare.shearModulus0 == 0.0 && bare.shearModulus1 == 0.0 && bare.shearModulus2 == 0.0 &&
             bare.shearModulus3 == 0.0));
    REQUIRE(cardAt<PolynomialShearModelTable>(model, 1).shearModulus0 == 8.0E7);
    const PolynomialShearModelCard full = cardAt<PolynomialShearModelTable>(model, 2);
    REQUIRE((full.shearModulus0 == 8.0E7 && full.shearModulus1 == 1.0E6 && full.shearModulus2 == -2.0E5 &&
             full.shearModulus3 == 3.0E4)); // a coefficient may be negative
    requireNoReferences<ElasticShearModelTable>(model);
    requireNoReferences<PolynomialShearModelTable>(model);
    requireRoundTrip(model);

    expectInvalid("ElasticShearModel;0;8.E+7", "id");
    expectInvalid("ElasticShearModel;251;STIFF", "shearModulus");
    expectInvalid("PolynomialShearModel;0", "id");
    expectInvalid("PolynomialShearModel;261;;;;FAST", "shearModulus3");
}

TEST_CASE("the equations of state and the shear models are numbered apart and a MATDEUL names them",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "DeflagrationStateEquation;11;1.2;0.001;;;;1.E+6;1550.;;3.E-8;0.8;5400.;0.6;0.1\n"
        "IgnitionGrowthStateEquation;20;;Explosive94A;SiUnits\n"
        "JonesWilkinsLeeStateEquation;31\n"
        "MieGruneisenStateEquation;41;3.E+3;1.6;2.;5.E+3\n"
        "NobleAbelStateEquation;51;1.2\n"
        "PolynomialStateEquation;61\n"
        "UserStateEquation;71;WATER\n"
        "ElasticShearModel;251\n"
        "PolynomialShearModel;261\n"
        "UserShearModel;271;GRP\n"
        "EulerMat;1;1.E+3;31;251\n"
        "EulerMat;2;1.E+3;20;261\n"
        "EulerMat;3;1.E+3;71;271");
    REQUIRE(unresolvedIn(model) == 0);

    // The ID is unique among the cards of its type; the same number in two types is the business of a check.
    REQUIRE_THROWS_AS(
        readBulk("MieGruneisenStateEquation;41;3.E+3;1.6;2.;5.E+3\nMieGruneisenStateEquation;41;2.E+3;1.5;2.;4.E+3"),
        base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ElasticShearModel;251\nElasticShearModel;251;8.E+7"), base::DeckError);

    Renumbering::apply(model, RenumberSpace::EquationOfState, Renumbering::offset(100));
    REQUIRE(cardAt<DeflagrationStateEquationTable>(model).id == 111);
    REQUIRE(cardAt<IgnitionGrowthStateEquationTable>(model).id == 120);
    REQUIRE(cardAt<JonesWilkinsLeeStateEquationTable>(model).id == 131);
    REQUIRE(cardAt<MieGruneisenStateEquationTable>(model).id == 141);
    REQUIRE(cardAt<NobleAbelStateEquationTable>(model).id == 151);
    REQUIRE(cardAt<PolynomialStateEquationTable>(model).id == 161);
    REQUIRE(cardAt<UserStateEquationTable>(model).id == 171);
    REQUIRE((cardAt<EulerMatTable>(model, 0).equationOfStateId == 131 &&
             cardAt<EulerMatTable>(model, 1).equationOfStateId == 120));
    REQUIRE(cardAt<EulerMatTable>(model, 2).equationOfStateId == 171);
    // Nothing of the shear space moved.
    REQUIRE((cardAt<ElasticShearModelTable>(model).id == 251 && cardAt<EulerMatTable>(model, 0).shearModelId == 251));

    Renumbering::apply(model, RenumberSpace::ShearModel, Renumbering::offset(1000));
    REQUIRE(cardAt<ElasticShearModelTable>(model).id == 1251);
    REQUIRE(cardAt<PolynomialShearModelTable>(model).id == 1261);
    REQUIRE(cardAt<UserShearModelTable>(model).id == 1271);
    REQUIRE(
        (cardAt<EulerMatTable>(model, 0).shearModelId == 1251 && cardAt<EulerMatTable>(model, 1).shearModelId == 1261));
    REQUIRE(cardAt<EulerMatTable>(model, 2).shearModelId == 1271);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // A MATDEUL that names a card the model lacks is reported.
    Model dangling = readBulk("NobleAbelStateEquation;51;1.2\nElasticShearModel;251\nEulerMat;1;1.E+3;52;252");
    REQUIRE(unresolvedIn(dangling) == 2);
}
