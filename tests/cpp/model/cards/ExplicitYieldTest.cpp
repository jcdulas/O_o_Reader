// Checks of the yield models (YLDHY, YLDJC, YLDMC, YLDMSS, YLDPOL, YLDRPL, YLDSG, YLDTM, YLDUDS, YLDVM, YLDZA), the
// failure models (FAILJC, FAILMPS, FAILUDS), the spall model PMINC and the cohesive zone material MCOHE of the
// explicit solver: the fields in the places of the format reference, the defaults, the values a card refuses, the
// references, and the ID spaces that a MATDEUL names. The generic round trip tests read the samples of these cards.

#include "oor/model/cards/elements/cohesiveMat/CohesiveMatTable.hpp"
#include "oor/model/cards/explicit/constantSpallation/ConstantSpallationTable.hpp"
#include "oor/model/cards/explicit/hydrodynamicYield/HydrodynamicYieldTable.hpp"
#include "oor/model/cards/explicit/johnsonCookFailure/JohnsonCookFailureTable.hpp"
#include "oor/model/cards/explicit/johnsonCookYield/JohnsonCookYieldTable.hpp"
#include "oor/model/cards/explicit/maxPlasticStrainFailure/MaxPlasticStrainFailureTable.hpp"
#include "oor/model/cards/explicit/mohrCoulombYield/MohrCoulombYieldTable.hpp"
#include "oor/model/cards/explicit/polynomialYield/PolynomialYieldTable.hpp"
#include "oor/model/cards/explicit/ratePowerLawYield/RatePowerLawYieldTable.hpp"
#include "oor/model/cards/explicit/snowMultiSurfaceYield/SnowMultiSurfaceYieldTable.hpp"
#include "oor/model/cards/explicit/steinbergGuinanYield/SteinbergGuinanYieldTable.hpp"
#include "oor/model/cards/explicit/tanimuraMimuraYield/TanimuraMimuraYieldTable.hpp"
#include "oor/model/cards/explicit/userFailureModel/UserFailureModelTable.hpp"
#include "oor/model/cards/explicit/userYield/UserYieldTable.hpp"
#include "oor/model/cards/explicit/vonMisesYield/VonMisesYieldTable.hpp"
#include "oor/model/cards/explicit/zerilliArmstrongYield/ZerilliArmstrongYieldTable.hpp"
#include "oor/model/cards/materials/eulerMat/EulerMatTable.hpp"
#include "support/ExplicitTestSupport.hpp"
#include "support/SlotRecord.hpp"

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The fields of a card by their format reference number (field 1 is the first one after the card name) and their text.
using Fields = std::map<int, std::string>;

// The record of the card, its fields given by the slots its card reader reads: the slots in between stay blank. A
// field with an empty text is blank.
std::string cardText(const std::string& name, const Fields& fields)
{
    std::vector<std::string> slots;
    for (const auto& [number, text] : fields)
    {
        if (text.empty())
            continue;
        if (slots.size() < static_cast<std::size_t>(number))
            slots.resize(static_cast<std::size_t>(number));
        slots[static_cast<std::size_t>(number - 1)] = text;
    }
    return slotRecord(name, slots);
}

// The same fields with one of them replaced.
Fields with(Fields fields, int number, const std::string& text)
{
    fields[number] = text;
    return fields;
}

} // namespace

TEST_CASE("a YLDHY has only its number and a YLDMC has three required reals", "[model][cards][explicit]")
{
    const Model model =
        readBulk("HydrodynamicYield;200\nMohrCoulombYield;1;1.E+6;2.E+6;1.E+4\nMohrCoulombYield;2;5.E+5;0.;-1.E+3");
    REQUIRE(cardAt<HydrodynamicYieldTable>(model).id == 200);
    const MohrCoulombYieldCard cap = cardAt<MohrCoulombYieldTable>(model, 0);
    REQUIRE((cap.id == 1 && cap.maxYieldStress == 10.0E5 && cap.cohesion == 20.0E5 && cap.internalFriction == 1.0E4));
    const MohrCoulombYieldCard flat = cardAt<MohrCoulombYieldTable>(model, 1);
    REQUIRE((flat.cohesion == 0.0 &&
             flat.internalFriction == -1.0E3)); // the format reference gives no sign for the three reals
    requireRoundTrip(model);

    expectInvalid("HydrodynamicYield;0", "id");
    expectInvalid(cardText("MohrCoulombYield", {{1, "1"}, {2, "10.E5"}, {3, "20.E5"}}), "internalFriction");
    expectInvalid(cardText("MohrCoulombYield", {{1, "1"}, {3, "20.E5"}, {4, "1.E4"}}), "maxYieldStress");
    expectInvalid("MohrCoulombYield;1;1.E+6;GOLD;1.E+4", "cohesion");
}

TEST_CASE("a YLDJC has its constants in the fields of the format reference and the defaults of the format",
          "[model][cards][explicit]")
{
    const Model model = readBulk(
        "JohnsonCookYield;100;3.1E+8;6.5E+7;0.12;0.22;0.85;3.;420.;1650.;288.\nJohnsonCookYield;101;1.E+"
        "8\nJohnsonCookYield;102;0.");
    const JohnsonCookYieldCard full = cardAt<JohnsonCookYieldTable>(model, 0);
    REQUIRE((full.id == 100 && full.coefficientA == 310.0E6 && full.coefficientB == 65.0E6 &&
             full.coefficientN == 0.12 && full.coefficientC == 0.22));
    REQUIRE((full.coefficientM == 0.85 && full.referenceStrainRate == 3.0 && full.specificHeat == 420.0 &&
             full.meltTemperature == 1650.0 && full.roomTemperature == 288.0));
    const JohnsonCookYieldCard bare = cardAt<JohnsonCookYieldTable>(model, 1);
    REQUIRE((bare.coefficientB == 0.0 && bare.coefficientN == 1.0 && bare.coefficientC == 0.0 &&
             bare.coefficientM == 1.0 && bare.referenceStrainRate == 1.0));
    REQUIRE((bare.specificHeat == JOHNSON_COOK_YIELD_NO_LIMIT && bare.meltTemperature == JOHNSON_COOK_YIELD_NO_LIMIT &&
             bare.roomTemperature == JOHNSON_COOK_YIELD_ROOM_TEMPERATURE));
    REQUIRE(cardAt<JohnsonCookYieldTable>(model, 2).coefficientA == 0.0); // a static yield stress of zero is allowed
    requireRoundTrip(model);

    const Fields base{{1, "1"}, {2, "100."}};
    expectInvalid("JohnsonCookYield;0;100.", "id");
    expectInvalid(cardText("JohnsonCookYield", {{1, "1"}}), "coefficientA"); // required
    expectInvalid(cardText("JohnsonCookYield", with(base, 2, "-1.")), "coefficientA");
    expectInvalid(cardText("JohnsonCookYield", with(base, 3, "ROUGH")), "coefficientB");
    expectInvalid(cardText("JohnsonCookYield", with(base, 7, "0.")), "referenceStrainRate");
    expectInvalid(cardText("JohnsonCookYield", with(base, 8, "0.")), "specificHeat");
}

TEST_CASE("a YLDMSS keeps the tensile strength resolved and accepts the Drucker-Prager case",
          "[model][cards][explicit]")
{
    const Model model = readBulk(
        "SnowMultiSurfaceYield;7;0.162;88.;2.E-5;0.07;0.3;0.97;88.;-0.32;0.\n"
        "SnowMultiSurfaceYield;8;0.162;78.;2.E-5;0.07;0.3;;;-0.32\n"
        "SnowMultiSurfaceYield;9;0.162;78.;0.");
    const SnowMultiSurfaceYieldCard example = cardAt<SnowMultiSurfaceYieldTable>(model, 0);
    REQUIRE((example.coefficientKc == 0.162 && example.coefficientT == 88.0 && example.coefficientCc == 2.0E-5 &&
             example.ac == 0.07));
    REQUIRE((example.bc == 0.3 && example.coefficientFc == 0.97 && example.initialAlpha == -0.32 &&
             example.coefficientDs == 0.0));
    REQUIRE(example.coefficientFtu == 88.0); // above T / 3, which the solver reduces: the card keeps what the deck says
    const SnowMultiSurfaceYieldCard blank = cardAt<SnowMultiSurfaceYieldTable>(model, 1);
    REQUIRE((blank.coefficientFc == SNOW_MULTI_SURFACE_DEFAULT_COEFFICIENT_FC && blank.coefficientDs == 0.0));
    REQUIRE(blank.coefficientFtu == 26.0); // a blank FTU is T / 3, stored resolved
    const SnowMultiSurfaceYieldCard drucker = cardAt<SnowMultiSurfaceYieldTable>(model, 2);
    REQUIRE((drucker.coefficientCc == 0.0 && drucker.ac == 0.0 && drucker.bc == 0.0 && drucker.initialAlpha == 0.0 &&
             drucker.coefficientFtu == 26.0));
    requireRoundTrip(model);

    const Fields base{{1, "1"}, {2, "0.162"}, {3, "78."}, {4, "2.E-5"}, {5, "0.07"}, {6, "0.3"}, {9, "-0.32"}};
    REQUIRE_NOTHROW(readBulk(cardText("SnowMultiSurfaceYield", base)));
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 1, "0")), "id");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 2, "")), "coefficientKc");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 2, "0.")), "coefficientKc");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 3, "0.")), "coefficientT");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 4, "")), "coefficientCc");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 4, "-1.")), "coefficientCc");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 5, "")),
                  "coefficientAc"); // required while CC is not zero
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 6, "0.")), "coefficientBc");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 7, "1.")), "coefficientFc");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 7, "0.")), "coefficientFc");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 8, "0.")), "coefficientFtu");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 9, "")), "initialAlpha");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 9, "0.32")), "initialAlpha");
    expectInvalid(cardText("SnowMultiSurfaceYield", with(base, 10, "-1.")), "coefficientDs");
}

TEST_CASE("a YLDPOL and a YLDRPL have their coefficients in the fields of the format reference",
          "[model][cards][explicit]")
{
    const Model model = readBulk(
        "PolynomialYield;7;1.8E+8\n"
        "PolynomialYield;8;1.8E+8;1.E+9;-2.E+9;3.E+9;-4.E+9;5.E+9;4.E+8\n"
        "RatePowerLawYield;7;1.8E+8\n"
        "RatePowerLawYield;8;1.8E+8;1.E+9;0.5;0.25;1.E+8");
    const PolynomialYieldCard bare = cardAt<PolynomialYieldTable>(model, 0);
    REQUIRE((bare.id == 7 && bare.coefficientA == 180.0E6 && bare.coefficientB == 0.0 && bare.coefficientC == 0.0 &&
             bare.coefficientD == 0.0 && bare.coefficientE == 0.0));
    REQUIRE((bare.coefficientF == 0.0 && bare.maxYieldStress == POLYNOMIAL_YIELD_NO_LIMIT));
    const PolynomialYieldCard full = cardAt<PolynomialYieldTable>(model, 1);
    REQUIRE((full.coefficientB == 1.0E9 && full.coefficientC == -2.0E9 && full.coefficientD == 3.0E9 &&
             full.coefficientE == -4.0E9 && full.coefficientF == 5.0E9));
    REQUIRE(full.maxYieldStress == 400.0E6);
    const RatePowerLawYieldCard rateBare = cardAt<RatePowerLawYieldTable>(model, 0);
    REQUIRE((rateBare.coefficientB == 0.0 && rateBare.coefficientN == 1.0 && rateBare.coefficientM == 1.0 &&
             rateBare.coefficientC == RATE_POWER_LAW_YIELD_DEFAULT_MINIMUM));
    const RatePowerLawYieldCard rateFull = cardAt<RatePowerLawYieldTable>(model, 1);
    REQUIRE((rateFull.coefficientA == 180.0E6 && rateFull.coefficientB == 1.0E9 && rateFull.coefficientN == 0.5 &&
             rateFull.coefficientM == 0.25));
    REQUIRE(rateFull.coefficientC == 100.0E6);
    requireRoundTrip(model);

    expectInvalid("PolynomialYield;0;1.8E+8", "id");
    expectInvalid("PolynomialYield;7", "coefficientA");
    expectInvalid("PolynomialYield;7;0.", "coefficientA");
    expectInvalid(cardText("PolynomialYield", {{1, "7"}, {2, "180.E6"}, {5, "ROUGH"}}), "coefficientD");
    expectInvalid("RatePowerLawYield;0;1.8E+8", "id");
    expectInvalid("RatePowerLawYield;7", "coefficientA");
    expectInvalid("RatePowerLawYield;7;-1.", "coefficientA");
    expectInvalid(cardText("RatePowerLawYield", {{1, "7"}, {2, "180.E6"}, {4, "SLOW"}}), "coefficientN");
}

TEST_CASE("a YLDSG has its second line in fields 9 and 10 and a YLDTM has fourteen fields", "[model][cards][explicit]")
{
    const Model model = readBulk(
        "SteinbergGuinanYield;7;7.E+6;1.2E+8;95.;0.5;;;;1620.;288.\n"
        "SteinbergGuinanYield;8;7.E+6;1.2E+8;95.;0.5;0.001;0.02;480.\n"
        "TanimuraMimuraYield;7;5.23E+7;1.72E+7;;;1.;0.001;;;;3.6E+9;1900.;0.5;1.\n"
        "TanimuraMimuraYield;8;5.23E+7");
    const SteinbergGuinanYieldCard example = cardAt<SteinbergGuinanYieldTable>(model, 0);
    REQUIRE((example.coefficientA1 == 7.0E6 && example.coefficientA2 == 120.0E6 && example.coefficientA3 == 95.0 &&
             example.coefficientA4 == 0.5));
    REQUIRE((example.coefficientH == 0.0 && example.coefficientB == 0.0 &&
             example.specificHeat == STEINBERG_GUINAN_YIELD_NO_LIMIT));
    REQUIRE((example.meltTemperature == 1620.0 && example.roomTemperature == 288.0));
    const SteinbergGuinanYieldCard heated = cardAt<SteinbergGuinanYieldTable>(model, 1);
    REQUIRE((heated.coefficientH == 1.0E-3 && heated.coefficientB == 2.0E-2 && heated.specificHeat == 480.0));
    REQUIRE((heated.meltTemperature == STEINBERG_GUINAN_YIELD_NO_LIMIT &&
             heated.roomTemperature == STEINBERG_GUINAN_YIELD_ROOM_TEMPERATURE));

    const TanimuraMimuraYieldCard tanimura = cardAt<TanimuraMimuraYieldTable>(model, 0);
    REQUIRE((tanimura.coefficientA == 52.3E6 && tanimura.coefficientB == 17.2E6 && tanimura.coefficientC == 0.0 &&
             tanimura.coefficientD == 0.0));
    REQUIRE((tanimura.coefficientM == 1.0 && tanimura.quasiStaticStrainRate == 0.001 &&
             tanimura.specificHeat == TANIMURA_MIMURA_YIELD_NO_LIMIT));
    REQUIRE((tanimura.meltTemperature == TANIMURA_MIMURA_YIELD_NO_LIMIT &&
             tanimura.roomTemperature == TANIMURA_MIMURA_YIELD_ROOM_TEMPERATURE));
    REQUIRE((tanimura.criticalStress == 3600.0E6 && tanimura.coefficientE == 1900.0 && tanimura.coefficientK == 0.5 &&
             tanimura.referenceStrainRate == 1.0));
    const TanimuraMimuraYieldCard bare = cardAt<TanimuraMimuraYieldTable>(model, 1);
    REQUIRE((bare.coefficientM == 0.0 && bare.quasiStaticStrainRate == 1.0 && bare.criticalStress == 1.0 &&
             bare.coefficientE == 0.0 && bare.coefficientK == 1.0 && bare.referenceStrainRate == 1.0));
    requireRoundTrip(model);

    const Fields steinberg{{1, "7"}, {2, "7.E6"}, {3, "120.E6"}, {4, "95."}, {5, "0.5"}};
    REQUIRE_NOTHROW(readBulk(cardText("SteinbergGuinanYield", steinberg)));
    expectInvalid(cardText("SteinbergGuinanYield", with(steinberg, 1, "0")), "id");
    expectInvalid(cardText("SteinbergGuinanYield", with(steinberg, 2, "")), "coefficientA1");
    expectInvalid(cardText("SteinbergGuinanYield", with(steinberg, 3, "0.")), "coefficientA2");
    expectInvalid(cardText("SteinbergGuinanYield", with(steinberg, 4, "-1.")), "coefficientA3");
    expectInvalid(cardText("SteinbergGuinanYield", with(steinberg, 5, "")), "coefficientA4");
    expectInvalid(cardText("SteinbergGuinanYield", with(steinberg, 8, "0.")), "specificHeat");

    const Fields tm{{1, "7"}, {2, "52.3E6"}};
    REQUIRE_NOTHROW(readBulk(cardText("TanimuraMimuraYield", tm)));
    expectInvalid(cardText("TanimuraMimuraYield", with(tm, 1, "0")), "id");
    expectInvalid(cardText("TanimuraMimuraYield", with(tm, 2, "")), "coefficientA");
    expectInvalid(cardText("TanimuraMimuraYield", with(tm, 7, "0.")), "quasiStaticStrainRate");
    expectInvalid(cardText("TanimuraMimuraYield", with(tm, 8, "0.")), "specificHeat");
    expectInvalid(cardText("TanimuraMimuraYield", with(tm, 11, "0.")), "criticalStress");
    expectInvalid(cardText("TanimuraMimuraYield", with(tm, 14, "0.")), "referenceStrainRate");
}

TEST_CASE("the user defined yield and failure models have a group, a routine and its default",
          "[model][cards][explicit]")
{
    const Model model = readBulk(
        "UserYield;12;myyld;ExternalYield\nUserYield;13;myyld\nUserFailureModel;200;usrfail;EXFAIL2\nUserFailureModel;"
        "201;"
        "usrfail");
    const UserYieldCard named = cardAt<UserYieldTable>(model, 0);
    REQUIRE((named.id == 12 && named.groupName == "myyld" && named.userRoutineName == "ExternalYield"));
    REQUIRE((cardAt<UserYieldTable>(model, 1).userRoutineName == USER_YIELD_DEFAULT_ROUTINE_NAME));
    const UserFailureModelCard failure = cardAt<UserFailureModelTable>(model, 0);
    REQUIRE((failure.id == 200 && failure.groupName == "usrfail" && failure.userRoutineName == "EXFAIL2"));
    REQUIRE((cardAt<UserFailureModelTable>(model, 1).userRoutineName == USER_FAILURE_MODEL_DEFAULT_ROUTINE_NAME));
    requireRoundTrip(model);

    expectInvalid("UserYield;0;myyld", "id");
    expectInvalid("UserYield;12", "groupName"); // the group has no default
    oor::testing::requireModelFileRoundTrip(readModelText("UserYield;12;toolonggroup", true));
    oor::testing::requireModelFileRoundTrip(readModelText("UserYield;12;my'yld", true));
    oor::testing::requireModelFileRoundTrip(readModelText("UserYield;12;myyld;TOOLONGNAME", true));
    expectInvalid("UserFailureModel;0;usrfail", "id");
    expectInvalid("UserFailureModel;200", "groupName");
    oor::testing::requireModelFileRoundTrip(readModelText("UserFailureModel;200;usrfail;TOOLONGNAME", true));
}

TEST_CASE("a YLDVM is a bilinear curve or a table, with the rate effect of a table or of Cowper-Symonds",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "VonMisesYield;32;2.75E+8;1.8E+9\n"
        "VonMisesYield;33;;;30;Plasticity;31\n"
        "VonMisesYield;34;2.75E+8;1.8E+9;;;;38.2;4.\n"
        "TableDynamic1;30;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;31;;;0.;1.;1.;1.;EndTable");
    const VonMisesYieldCard bilinear = cardAt<VonMisesYieldTable>(model, 0);
    REQUIRE((bilinear.id == 32 && bilinear.yieldStress == 275.0E6 && bilinear.hardeningModulus == 1800.0E6));
    REQUIRE((bilinear.tableId == 0 && bilinear.type == VonMisesYieldCurveType::TrueStrain &&
             bilinear.rateScaleTableId == 0));
    REQUIRE((bilinear.rateCoefficientD == 0.0 && bilinear.rateExponentP == 0.0));
    const VonMisesYieldCard curve = cardAt<VonMisesYieldTable>(model, 1);
    REQUIRE(
        (!curve.yieldStress && curve.hardeningModulus == 0.0 && curve.tableId == 30 && curve.rateScaleTableId == 31));
    REQUIRE(curve.type == VonMisesYieldCurveType::PlasticStrain);
    const VonMisesYieldCard rate = cardAt<VonMisesYieldTable>(model, 2);
    REQUIRE((rate.rateCoefficientD == 38.2 && rate.rateExponentP == 4.0 && rate.rateScaleTableId == 0));
    REQUIRE(unresolvedIn(model) == 0);

    // The two tables are TABLED1 cards; a card without a table visits none.
    RecordingVisitor visitor;
    model.table<VonMisesYieldTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"tableId", "rateScaleTableId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::DynamicTable, ReferenceTarget::DynamicTable});
    visitor = RecordingVisitor{};
    model.table<VonMisesYieldTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());

    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(500));
    REQUIRE((cardAt<VonMisesYieldTable>(model, 1).tableId == 530 &&
             cardAt<VonMisesYieldTable>(model, 1).rateScaleTableId == 531));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    Model dangling = readBulk("VonMisesYield;1;;;30;;31");
    REQUIRE(unresolvedIn(dangling) == 2);

    expectInvalid("VonMisesYield;0;2.75E+8", "id");
    expectInvalid("VonMisesYield;1", "yieldStress");             // neither a yield stress nor a table
    expectInvalid("VonMisesYield;1;2.75E+8;;30", "yieldStress"); // a yield stress and a table
    expectInvalid("VonMisesYield;1;;;30;STRESS", "curveType");
    expectInvalid("VonMisesYield;1;;;-30", "tableId");
    expectInvalid("VonMisesYield;1;2.75E+8;;;;-5", "rateScaleTableId");
    expectInvalid("VonMisesYield;1;2.75E+8;;;;;-1.", "rateCoefficientD");
    expectInvalid("VonMisesYield;1;2.75E+8;;;;;;-1.", "rateExponentP");
    expectInvalid("VonMisesYield;1;;;30;True;31;1.",
                  "rateCoefficientD"); // a table of rate factors replaces Cowper-Symonds
    expectInvalid("VonMisesYield;1;;;30;True;31;;2.", "rateExponentP");
}

TEST_CASE("a YLDZA is a face centred metal unless it has a D, and has its specific heat in field 8",
          "[model][cards][explicit]")
{
    const Model model = readBulk(
        "ZerilliArmstrongYield;7;2.E+8;5.E+7;0.1;;;;;0.\nZerilliArmstrongYield;8;2.E+8\nZerilliArmstrongYield;9;2.E+8;"
        "5.E+7;0.5;2.;0.5;0.1;450.;1.E+5");
    const ZerilliArmstrongYieldCard bcc = cardAt<ZerilliArmstrongYieldTable>(model, 0);
    REQUIRE((bcc.id == 7 && bcc.coefficientA == 200.0E6 && bcc.coefficientB == 50.0E6 && bcc.coefficientN == 0.1));
    REQUIRE((bcc.coefficientD.has_value() && *bcc.coefficientD == 0.0)); // a D of zero is a body centred metal
    const ZerilliArmstrongYieldCard fcc = cardAt<ZerilliArmstrongYieldTable>(model, 1);
    REQUIRE((fcc.coefficientB == 0.0 && fcc.coefficientN == 0.0 && fcc.coefficientC == 1.0 && fcc.coefficientM == 1.0 &&
             fcc.referenceStrainRate == 1.0));
    REQUIRE((fcc.specificHeat == ZERILLI_ARMSTRONG_YIELD_NO_LIMIT && !fcc.coefficientD));
    const ZerilliArmstrongYieldCard full = cardAt<ZerilliArmstrongYieldTable>(model, 2);
    REQUIRE((full.coefficientC == 2.0 && full.coefficientM == 0.5 && full.referenceStrainRate == 0.1 &&
             full.specificHeat == 450.0 && full.coefficientD == 1.0E5));
    requireRoundTrip(model);

    const Fields base{{1, "7"}, {2, "200.E6"}};
    REQUIRE_NOTHROW(readBulk(cardText("ZerilliArmstrongYield", with(base, 3, "0."))));
    expectInvalid(cardText("ZerilliArmstrongYield", with(base, 1, "0")), "id");
    expectInvalid(cardText("ZerilliArmstrongYield", with(base, 2, "0.")), "coefficientA");
    expectInvalid(cardText("ZerilliArmstrongYield", with(base, 3, "-1.")), "coefficientB");
    expectInvalid(cardText("ZerilliArmstrongYield", with(base, 7, "0.")), "referenceStrainRate");
    expectInvalid(cardText("ZerilliArmstrongYield", with(base, 8, "0.")), "specificHeat");
    expectInvalid(cardText("ZerilliArmstrongYield", with(base, 9, "HARD")), "coefficientD");
}

TEST_CASE("a FAILJC has its method in field 11 and a FAILMPS only its maximum strain", "[model][cards][explicit]")
{
    const Model model = readBulk(
        "JohnsonCookFailure;1;0.07;2.86;-1.75;0.004;0.21;1.;291.;1520.;470.;Continue\n"
        "JohnsonCookFailure;2\n"
        "JohnsonCookFailure;3;;;;;;;;;;Discard\n"
        "JohnsonCookFailure;4;;;;;;;;;;NoFailure\n"
        "MaxPlasticStrainFailure;1;0.15\n"
        "MaxPlasticStrainFailure;2;-0.001");
    const JohnsonCookFailureCard example = cardAt<JohnsonCookFailureTable>(model, 0);
    REQUIRE((example.id == 1 && example.coefficientD1 == 0.07 && example.coefficientD2 == 2.86 &&
             example.coefficientD3 == -1.75));
    REQUIRE((example.coefficientD4 == 0.004 && example.coefficientD5 == 0.21 && example.referenceStrainRate == 1.0 &&
             example.roomTemperature == 291.0));
    REQUIRE((example.meltTemperature == 1520.0 && example.specificHeat == 470.0 &&
             example.method == JohnsonCookFailureMethod::Continuous));
    const JohnsonCookFailureCard bare = cardAt<JohnsonCookFailureTable>(model, 1);
    REQUIRE((bare.coefficientD1 == 0.0 && bare.coefficientD5 == 0.0 && bare.referenceStrainRate == 1.0 &&
             bare.roomTemperature == 0.0));
    REQUIRE((bare.meltTemperature == JOHNSON_COOK_FAILURE_NO_LIMIT &&
             bare.specificHeat == JOHNSON_COOK_FAILURE_NO_LIMIT &&
             bare.method == JohnsonCookFailureMethod::Continuous));
    REQUIRE(cardAt<JohnsonCookFailureTable>(model, 2).method == JohnsonCookFailureMethod::Discrete);
    REQUIRE(cardAt<JohnsonCookFailureTable>(model, 3).method ==
            JohnsonCookFailureMethod::NoFailure); // the word in any case
    REQUIRE((cardAt<MaxPlasticStrainFailureTable>(model, 0).id == 1 &&
             cardAt<MaxPlasticStrainFailureTable>(model, 0).maxPlasticStrain == 0.15));
    REQUIRE(cardAt<MaxPlasticStrainFailureTable>(model, 1).maxPlasticStrain ==
            -1.0E-3); // the format reference gives no sign
    requireRoundTrip(model);

    const Fields base{{1, "1"}};
    expectInvalid("JohnsonCookFailure;0", "id");
    expectInvalid(cardText("JohnsonCookFailure", with(base, 2, "ROUGH")), "coefficientD1");
    expectInvalid(cardText("JohnsonCookFailure", with(base, 11, "ABRUPT")), "method");
    expectInvalid("MaxPlasticStrainFailure;0;0.15", "id");
    expectInvalid("MaxPlasticStrainFailure;1", "maxPlasticStrain");
    expectInvalid("MaxPlasticStrainFailure;1;STRAIN", "maxPlasticStrain");
}

TEST_CASE("a PMINC has a minimum pressure that is not positive and two volume tolerances", "[model][cards][explicit]")
{
    const Model model =
        readBulk("ConstantSpallation;220;-420\nConstantSpallation;221\nConstantSpallation;222;-420.;1.E-5;2.E-5");
    const ConstantSpallationCard example = cardAt<ConstantSpallationTable>(model, 0);
    REQUIRE((example.id == 220 && example.minPressure == -420.0 &&
             example.volumeCutoffTolerance == CONSTANT_SPALLATION_DEFAULT_CUTOFF_TOLERANCE));
    REQUIRE(example.maxVoidFraction == 0.0);
    const ConstantSpallationCard bare = cardAt<ConstantSpallationTable>(model, 1);
    REQUIRE((bare.minPressure == 0.0 && bare.volumeCutoffTolerance == CONSTANT_SPALLATION_DEFAULT_CUTOFF_TOLERANCE &&
             bare.maxVoidFraction == 0.0));
    const ConstantSpallationCard full = cardAt<ConstantSpallationTable>(model, 2);
    REQUIRE((full.volumeCutoffTolerance == 1.0E-5 && full.maxVoidFraction == 2.0E-5));
    requireRoundTrip(model);

    expectInvalid("ConstantSpallation;0", "id");
    expectInvalid("ConstantSpallation;220;10.", "minPressure");
    expectInvalid("ConstantSpallation;220;-420.;0.", "volumeCutoffTolerance");
    expectInvalid("ConstantSpallation;220;-420.;1.E-4;-1.E-5", "maxVoidFraction");
}

TEST_CASE("an MCOHE is a material with its fields over three lines and a stress table", "[model][cards][explicit]")
{
    Model model = readBulk(
        "CohesiveMat;705;2;364;142.5;0.;0.;1.;0.03;0.;;1.\n"
        "CohesiveMat;706\n"
        "CohesiveMat;707;3;364;142.5;0.01;0.03;2.;0.5;0.1;0.001;0.5;3.;100.;200.\n"
        "CohesiveMat;708;-2;;1.5;;;;;;;;;100.;200.\n"
        "TableStress1;364;;0.;0.;0.01;1.2E+4;0.02;16500.;EndTable");
    const CohesiveMatCard example = cardAt<CohesiveMatTable>(model, 0);
    REQUIRE((example.id == 705 && example.cohesiveModel == COHESIVE_MAT_MODEL_EXPONENTIAL && example.tableId == 364));
    REQUIRE((example.cohesiveEnergy == 142.5 && example.criticalOpening == 0.0 && example.maxOpening == 0.0));
    REQUIRE((example.shearNormalStressRatio == 1.0 && example.exponent == 0.03 && example.viscousDissipation == 0.0 &&
             example.referenceDisplacementRate == 0.0));
    REQUIRE((example.compressionStiffening == 1.0 && example.shearNormalEnergyRatio == 1.0 &&
             example.inPlaneStiffness == 0.0 && example.normalStiffness == 0.0));
    const CohesiveMatCard bare = cardAt<CohesiveMatTable>(model, 1);
    REQUIRE((bare.cohesiveModel == COHESIVE_MAT_MODEL_BILINEAR && bare.tableId == 0 && !bare.cohesiveEnergy &&
             !bare.criticalOpening && !bare.maxOpening));
    REQUIRE((bare.shearNormalStressRatio == 1.0 && bare.exponent == 1.0 && bare.viscousDissipation == 0.0 &&
             bare.referenceDisplacementRate == 0.0 && bare.compressionStiffening == 1.0));
    REQUIRE((bare.shearNormalEnergyRatio == 1.0 && bare.inPlaneStiffness == 0.0 && bare.normalStiffness == 0.0));
    const CohesiveMatCard full = cardAt<CohesiveMatTable>(model, 2);
    REQUIRE((full.cohesiveModel == COHESIVE_MAT_MODEL_LINEAR_EXPONENTIAL && full.criticalOpening == 0.01 &&
             full.maxOpening == 0.03));
    REQUIRE((full.shearNormalStressRatio == 2.0 && full.exponent == 0.5 && full.viscousDissipation == 0.1 &&
             full.referenceDisplacementRate == 1.0E-3 && full.compressionStiffening == 0.5));
    REQUIRE((full.shearNormalEnergyRatio == 3.0 && full.inPlaneStiffness == 100.0 && full.normalStiffness == 200.0));
    const CohesiveMatCard linear = cardAt<CohesiveMatTable>(model, 3);
    REQUIRE((linear.cohesiveModel == COHESIVE_MAT_MODEL_LINEAR && linear.cohesiveEnergy == 1.5 &&
             linear.inPlaneStiffness == 100.0 && linear.normalStiffness == 200.0));
    REQUIRE(unresolvedIn(model) == 0);

    // The TID is a stress table, and the ID is in the space of the materials.
    RecordingVisitor visitor;
    model.table<CohesiveMatTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"tableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::StressTable});
    visitor = RecordingVisitor{};
    model.table<CohesiveMatTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields.empty());
    REQUIRE_THROWS_AS(readBulk("CohesiveMat;5\nIsoMat;5;2.1E+11;;0.3"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::StressTable, Renumbering::offset(1000));
    REQUIRE((cardAt<CohesiveMatTable>(model, 0).tableId == 1364 && cardAt<CohesiveMatTable>(model, 2).tableId == 1364));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(10));
    REQUIRE((cardAt<CohesiveMatTable>(model, 0).id == 715 && cardAt<CohesiveMatTable>(model, 1).id == 716));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    Model dangling = readBulk("CohesiveMat;1;1;9");
    REQUIRE(unresolvedIn(dangling) == 1);

    const Fields base{{1, "1"}, {9, "1."}};
    REQUIRE_NOTHROW(readBulk(cardText("CohesiveMat", base)));
    expectInvalid("CohesiveMat;0", "id");
    expectInvalid(cardText("CohesiveMat", with(base, 2, "4")), "cohesiveModel");
    expectInvalid(cardText("CohesiveMat", with(base, 2, "0")), "cohesiveModel");
    expectInvalid(cardText("CohesiveMat", with(base, 2, "-3")), "cohesiveModel");
    expectInvalid(cardText("CohesiveMat", with(base, 4, "-1")), "tableId");
    expectInvalid(cardText("CohesiveMat", with(base, 9, "-1.")), "cohesiveEnergy");
    expectInvalid(cardText("CohesiveMat", with(base, 10, "-1.")), "criticalOpening");
    expectInvalid(cardText("CohesiveMat", with(base, 11, "-1.")), "maxOpening");
    expectInvalid(cardText("CohesiveMat", with(base, 12, "0.")), "shearNormalStressRatio");
    expectInvalid(cardText("CohesiveMat", with(base, 13, "0.")), "exponent");
    expectInvalid(cardText("CohesiveMat", with(base, 14, "-1.")), "viscousDissipation");
    expectInvalid(cardText("CohesiveMat", with(base, 15, "-1.")), "referenceDisplacementRate");
    expectInvalid(cardText("CohesiveMat", with(base, 16, "-1.")), "compressionStiffening");
    expectInvalid(cardText("CohesiveMat", with(base, 17, "0.")), "shearNormalEnergyRatio");
    expectInvalid(cardText("CohesiveMat", with(base, 19, "-1.")), "inPlaneStiffness");
    expectInvalid(cardText("CohesiveMat", with(base, 20, "-1.")), "normalStiffness");
}

TEST_CASE("the yield, failure and spall models are spaces of their own that a MATDEUL names",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "EulerMat;25;1.E+3;1;;32;5;7\n"
        "GammaLawStateEquation;1;1.4\n"
        "VonMisesYield;32;2.5E+8\n"
        "HydrodynamicYield;10\n"
        "UserYield;40;myyld\n"
        "MaxPlasticStrainFailure;5;0.15\n"
        "UserFailureModel;6;usrfail\n"
        "ConstantSpallation;7;-370.");
    REQUIRE(unresolvedIn(model) == 0);

    Renumbering::apply(model, RenumberSpace::YieldModel, Renumbering::offset(100));
    REQUIRE((cardAt<VonMisesYieldTable>(model).id == 132 && cardAt<HydrodynamicYieldTable>(model).id == 110));
    REQUIRE(cardAt<UserYieldTable>(model).id == 140); // every YLDxxx card is in the space
    REQUIRE(cardAt<EulerMatTable>(model).yieldModelId == 132);
    Renumbering::apply(model, RenumberSpace::FailureModel, Renumbering::offset(200));
    REQUIRE((cardAt<MaxPlasticStrainFailureTable>(model).id == 205 && cardAt<UserFailureModelTable>(model).id == 206));
    REQUIRE(cardAt<EulerMatTable>(model).failureModelId == 205);
    Renumbering::apply(model, RenumberSpace::SpallModel, Renumbering::offset(300));
    REQUIRE((cardAt<ConstantSpallationTable>(model).id == 307 && cardAt<EulerMatTable>(model).spallModelId == 307));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // A model that no card defines is a reference to nothing.
    Model dangling = readBulk("EulerMat;25;1.E+3;1;;32;5;7");
    REQUIRE(unresolvedIn(dangling) == 4); // the equation of state and the three models

    // The ID of a card is unique among the cards of its type; the other types of the category are a check of the model.
    REQUIRE_THROWS_AS(readBulk("HydrodynamicYield;1\nHydrodynamicYield;1"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("MaxPlasticStrainFailure;1;0.15\nMaxPlasticStrainFailure;1;0.2"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ConstantSpallation;1\nConstantSpallation;1"), base::DeckError);
    REQUIRE_NOTHROW(readBulk(
        "HydrodynamicYield;1\nMohrCoulombYield;2;1.;2.;3.\nMaxPlasticStrainFailure;1;0.15\nConstantSpallation;1"));
}
