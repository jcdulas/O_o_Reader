// Checks of the non-linear materials: the non-linear laws of SOL 400 and SOL 700 (MATS3, MATS8, MATG, MATTG, MATSORT,
// MATTORT, MATORT, MATPE1, MATRIG, MATSMA, MATUSR, MATUDS, MATVP, MATF, MATCB, MATFAB, MATDEUL, MATDIGI, MATBV,
// NLMOPTS) and their companions (MATTEP, MATTHE, MATTVE, MATTUSR, MATTF, MAT1A, MAT8A, MATF1, MAT1F, MAT2F, MAT8F,
// MAT9F, MAT10F, MATFTG) and the SOL 700 form of the MATVE: field positions, variants, references, ID spaces,
// renumbering and refused values.

#include "oor/model/cards/materials/anisoMat2DFreq/AnisoMat2DFreqTable.hpp"
#include "oor/model/cards/materials/anisoMat3DFreq/AnisoMat3DFreqTable.hpp"
#include "oor/model/cards/materials/bulkViscosityMat/BulkViscosityMatTable.hpp"
#include "oor/model/cards/materials/elastoPlasticMatTemperature/ElastoPlasticMatTemperatureTable.hpp"
#include "oor/model/cards/materials/eulerMat/EulerMatTable.hpp"
#include "oor/model/cards/materials/fabricMat/FabricMatTable.hpp"
#include "oor/model/cards/materials/failureMat/FailureMatTable.hpp"
#include "oor/model/cards/materials/failureMatTemperature/FailureMatTemperatureTable.hpp"
#include "oor/model/cards/materials/fatigueMat/FatigueMatTable.hpp"
#include "oor/model/cards/materials/fluidMatFreq/FluidMatFreqTable.hpp"
#include "oor/model/cards/materials/hyperelasticMatTemperature/HyperelasticMatTemperatureTable.hpp"
#include "oor/model/cards/materials/isoMatExplicitExt/IsoMatExplicitExtTable.hpp"
#include "oor/model/cards/materials/isoMatFreq/IsoMatFreqTable.hpp"
#include "oor/model/cards/materials/micromechanicsCompositeMat/MicromechanicsCompositeMatTable.hpp"
#include "oor/model/cards/materials/nonlinearMatOptions/NonlinearMatOptionsTable.hpp"
#include "oor/model/cards/materials/orthoMat2DFailure/OrthoMat2DFailureTable.hpp"
#include "oor/model/cards/materials/orthoMat2DFreq/OrthoMat2DFreqTable.hpp"
#include "oor/model/cards/materials/poroElasticMatFreq/PoroElasticMatFreqTable.hpp"
#include "oor/model/cards/materials/progressiveFailureMat/ProgressiveFailureMatTable.hpp"
#include "oor/model/cards/materials/shapeMemoryMat/ShapeMemoryMatTable.hpp"
#include "oor/model/cards/materials/userMat/UserMatTable.hpp"
#include "oor/model/cards/materials/userMatService/UserMatServiceTable.hpp"
#include "oor/model/cards/materials/userMatTable/UserMatTableTable.hpp"
#include "oor/model/cards/materials/viscoElasticMat/ViscoElasticMatTable.hpp"
#include "oor/model/cards/materials/viscoElasticMatTemperature/ViscoElasticMatTemperatureTable.hpp"
#include "oor/model/cards/materials/viscoPlasticMat/ViscoPlasticMatTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using Catch::Approx;
using oor::testing::readModelText;

namespace
{

Model readBulk(const std::string& cards)
{
    return readModelText(cards, true);
}

template <typename TableT>
typename TableT::CardType cardAt(const Model& model, std::uint32_t row = 0)
{
    return model.findTable<TableT>()->card(row);
}

std::size_t unresolvedIn(Model& model)
{
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    return ReferenceChecker::check(model, sink);
}

// The card record is an error in strict mode and in tolerant mode, where the first error names the field (when given).
void expectInvalid(const std::string& card, const std::string& field = "")
{
    INFO(card);
    REQUIRE_THROWS_AS(readModelText(card, true), base::DeckError);
    const Model tolerant = readModelText(card, false);
    REQUIRE(tolerant.diagnostics().hasErrors());
    if (!field.empty())
    {
        const auto error = tolerant.diagnostics().firstError();
        REQUIRE(error.has_value());
        INFO(error->message);
        REQUIRE(error->location.fieldName == field);
    }
}

// Records the references a card lists: the field names and what each points to.
struct RecordingVisitor final : ReferenceVisitor
{
    std::vector<std::string> fields;
    std::vector<ReferenceTarget> targets;
    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        fields.emplace_back(field);
        targets.push_back(target);
        return id;
    }
};

// Writes the model as a model file and reads that again: the same cards.
void requireRoundTrip(const Model& model)
{
    oor::testing::requireModelFileRoundTrip(model);
}

} // namespace

// ------------------------------------------------------------------------------------------------ the companions

TEST_CASE("a MATTEP has its tables in the fields of the format reference and the Chaboche group on the third line",
          "[model][cards]")
{
    Model model = readBulk(
        "ElastoPlasticMatTemperature;100;20;;5;ChabocheHardening;11;12;13;14;15;16;17;18;19;20\n"
        "ElastoPlasticMatTemperature;101;;4\n"
        "ElastoPlasticMatTemperature;102;20;;;ChabocheHardening;1;2;3;4;5;6;7");
    const ElastoPlasticMatTemperatureCard full = cardAt<ElastoPlasticMatTemperatureTable>(model, 0);
    REQUIRE(full.yieldStressTableId == 20);
    REQUIRE(full.stressStrainCurvesTableId == 0);
    REQUIRE(full.hardeningSlopeTableId == 5);
    REQUIRE(full.chabocheHardening.has_value());
    REQUIRE((*full.chabocheHardening)[0] == 11);
    REQUIRE((*full.chabocheHardening)[6] == 17);
    REQUIRE((*full.chabocheHardening)[7] == 18); // QM: the second line of the group, after a blank field
    REQUIRE((*full.chabocheHardening)[9] == 20);
    REQUIRE(cardAt<ElastoPlasticMatTemperatureTable>(model, 1).stressStrainCurvesTableId == 4);
    REQUIRE_FALSE(cardAt<ElastoPlasticMatTemperatureTable>(model, 1).chabocheHardening.has_value());
    REQUIRE((*cardAt<ElastoPlasticMatTemperatureTable>(model, 2).chabocheHardening)[6] == 7);
    requireRoundTrip(model);
    // T(FID) replaces T(Y0) and T(H); the blank cells of the layout stay blank; the group is CHABOCHE
    // only.
    expectInvalid("ElastoPlasticMatTemperature;1;20;4", "stressStrainCurvesTableId");
    expectInvalid("ElastoPlasticMatTemperature;1;-3", "yieldStressTableId");
}

TEST_CASE("a MATTVE has the WLF, POWER, NARA and USER shift functions", "[model][cards]")
{
    Model model = readBulk(
        "ViscoElasticMatTemperature;100;WilliamsLandelFerry;100.;;;;;;0.8;1.2\n"
        "ViscoElasticMatTemperature;101;PowerLaw;100.;;;;;11;0.;0.;1.;0.97;0.95;0.93;0.91;0.89;0.87;0.85;0.83;0.82;0."
        "81\n"
        "ViscoElasticMatTemperature;102;Narayanaswamy;;5.6E-8;0.65;180.;360.;11;0.;0.;1.;0.97;0.95;0.93;0.91;0.89;0.87;"
        "0.85;0."
        "83;0.82;0.81;;;;;;315.;314.;313.;312.;311.;310.;309.;308.;307.;306.;305.\n"
        "ViscoElasticMatTemperature;103;User;360.;;;;;8\n"
        "ViscoElasticMatTemperature;104;Narayanaswamy;;5.6E-8;0.65;180.;360.;3;0.;0.;1.;0.9;0.8;;;;;;315.;314.;313.;;;;"
        ";;2");
    const ViscoElasticMatTemperatureCard wlf = cardAt<ViscoElasticMatTemperatureTable>(model, 0);
    REQUIRE(wlf.shift == ViscoElasticMatTemperatureShift::Wlf);
    REQUIRE(wlf.wlfConstant1 == Approx(0.8));
    REQUIRE(wlf.wlfConstant2 == Approx(1.2));
    REQUIRE(wlf.shiftReferenceTemperature == Approx(100.0));
    const ViscoElasticMatTemperatureCard power = cardAt<ViscoElasticMatTemperatureTable>(model, 1);
    REQUIRE(power.powerCoefficients.size() == 11);
    REQUIRE(power.powerCoefficients[10] == Approx(0.81));
    const ViscoElasticMatTemperatureCard nara = cardAt<ViscoElasticMatTemperatureTable>(model, 2);
    REQUIRE(nara.naraWeights.size() == 11);
    REQUIRE(nara.naraRelaxationTimes.size() == 11);
    REQUIRE(nara.naraRelaxationTimes[10] == Approx(305.0));
    REQUIRE(nara.activationEnergy.value() == Approx(5.6e-8));
    REQUIRE_FALSE(nara.fictiveTemperatureOption.has_value());
    REQUIRE(cardAt<ViscoElasticMatTemperatureTable>(model, 3).shift == ViscoElasticMatTemperatureShift::User);
    REQUIRE(cardAt<ViscoElasticMatTemperatureTable>(model, 4).fictiveTemperatureOption.value() ==
            2); // after the T line, on a new line
    requireRoundTrip(model);
    expectInvalid("ViscoElasticMatTemperature;1;WilliamsLandelFerry;100.", "wlfConstant1"); // WLF needs A1 and A2
    expectInvalid("ViscoElasticMatTemperature;1;SHIFT", "shiftFunction");
    expectInvalid("ViscoElasticMatTemperature;1;Narayanaswamy;;;0.65;180.;360.;1;0.;0.;1.;;;;;;;;315.",
                  "activationEnergy"); // NARA needs its four reals
    expectInvalid("ViscoElasticMatTemperature;1;PowerLaw;;;;;;12;0.;0.;1.;2.;3.;4.;5.;6.;7.;8.;9.;10.;11.;12.",
                  "termCount");
    expectInvalid("ViscoElasticMatTemperature;1;Narayanaswamy;;1.;0.65;180.;360.;1;0.;0.;1.;;;;;;;;315.;;;;;;;;4",
                  "fictiveTemperatureOption");
    expectInvalid("ViscoElasticMatTemperature;1;WilliamsLandelFerry;100.;1.;;;;;0.8;1.2",
                  "activationEnergy"); // ENER is for NARA only
}

TEST_CASE("a MATUDS has its INT, REAL and CHAR blocks in any order, seven values to a line", "[model][cards]")
{
    Model model = readBulk(
        "UserMatService;17;ElastoPlasticMat;MATERIAL;UELAST1\n"
        "UserMatService;18;UserMat;MATERIAL;UHYPER2;RealBlock;3;0.00217;23850.;0.05;IntegerBlock;2;6;4\n"
        "UserMatService;19;UserMat;GRP;UMAT;IntegerBlock;9;1;2;3;4;5;6;7;8;9;RealBlock;2;1.5;2.5;TextBlock;2;AAA;BBB");
    const UserMatServiceCard first = cardAt<UserMatServiceTable>(model, 0);
    REQUIRE(first.materialCardType == "ElastoPlasticMat");
    REQUIRE(first.userRoutineName == "UELAST1");
    REQUIRE(first.integers.empty());
    const UserMatServiceCard second = cardAt<UserMatServiceTable>(model, 1);
    REQUIRE(second.integers == std::vector<int>{6, 4});
    REQUIRE(second.reals.size() == 3);
    REQUIRE(second.reals[1] == Approx(23850.0));
    const UserMatServiceCard third = cardAt<UserMatServiceTable>(model, 2);
    REQUIRE(third.integers == std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9});
    REQUIRE(third.words == std::vector<std::string>{"AAA", "BBB"});
    requireRoundTrip(model);
    expectInvalid("UserMatService;1;UserMat;G;U;IntegerBlock;1;1;IntegerBlock;1;2"); // a block twice
    expectInvalid("UserMatService;1;UserMat;G;U;TextBlock;2;AAA;RealBlock");         // a CDATA cannot be a keyword
    expectInvalid("UserMatService;1;UserMat;G;U;IntegerBlock;3;1;;3");               // a blank cell between data
    oor::testing::requireModelFileRoundTrip(
        readModelText("UserMatService;1;UserMat;G;UNAMETOOLONG", true)); // long routine names are preserved
    // The ID is the material that the card serves.
    RecordingVisitor visitor;
    model.table<UserMatServiceTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
}

// ------------------------------------------------------------------------------------------------ SOL 700 and failure

TEST_CASE("the SOL 700 materials MATBV, MAT1A, MAT8A, MATDEUL and MATFAB have the layouts of the format reference",
          "[model][cards]")
{
    Model model = readBulk(
        "BulkViscosityMat;7;;1.2\n"
        "BulkViscosityMat;8;ExternalExplicitSolver;0.5;2.;0.1;0.05;0.01\n"
        "IsoMatExplicitExt;101;201\n"
        "OrthoMat2DFailure;7;CombinedFailure;;120.;;;;240.;180.;130.;140.;Steps;250.;;ChangChangFailure;ModifiedTsaiWu;"
        "ModifiedTsaiWu;StressTheory;ChangChangFailure;;;;;"
        "0101\n"
        "OrthoMat2DFailure;8;;;50.\n"
        "EulerMat;4510;2600.;120;115;311;312;405\n"
        "EulerMat;4511;1800.;120;115;311;312;405;0.1;2.;ExternalExplicitSolver\n"
        "FabricMat;4520;780;4.75E+6;0.31;;;;;1.84E+8;;;1.;0.;0.;1.84E+8;;;0.;1.;0.\n"
        "FabricMat;4521;780;4.75E+6;0.31;2.E+6;0.04;0.6;15.;1.84E+8;1.E+5;25.;1.;0.;0.;1.84E+8;2.E+5;115.;0.;1.;0.;0.3;"
        "1.E+7;0.04;0.7;14.;16.\n"
        "ViscoElasticMat;9;IsotropicLinear;6.E+7;2.E+7;0.2");
    const BulkViscosityMatCard viscosity = cardAt<BulkViscosityMatTable>(model, 0);
    REQUIRE(viscosity.bulkViscosityKind == BulkViscosityMatType::Standard);
    REQUIRE(viscosity.quadraticBulkViscosity == Approx(1.2));
    REQUIRE(viscosity.linearBulkViscosity == Approx(1.0)); // defaults
    REQUIRE(viscosity.initialMetricRelaxation == Approx(1.0e-3));
    REQUIRE(cardAt<BulkViscosityMatTable>(model, 1).bulkViscosityKind == BulkViscosityMatType::Enhanced);
    REQUIRE(cardAt<IsoMatExplicitExtTable>(model).equationOfStateId == 201);
    const OrthoMat2DFailureCard failure = cardAt<OrthoMat2DFailureTable>(model, 0);
    REQUIRE(failure.theory == OrthoMat2DFailureTheory::Combination);
    REQUIRE(failure.modes[0] == OrthoMat2DFailureTheory::Chang);
    REQUIRE(failure.modes[2] == OrthoMat2DFailureTheory::ModifiedTsai);
    REQUIRE(failure.propertyDegradation[4] == "0101"); // PRDSH keeps its leading zeros
    REQUIRE(failure.propertyDegradation[0] == "1111"); // the default
    REQUIRE(failure.degradationParameter == Approx(250.0));
    REQUIRE(cardAt<OrthoMat2DFailureTable>(model, 1).theory == OrthoMat2DFailureTheory::None);
    const EulerMatCard euler = cardAt<EulerMatTable>(model, 1);
    REQUIRE(euler.linearBulkViscosity == Approx(0.1));
    REQUIRE(euler.quadraticBulkViscosity == Approx(2.0));
    REQUIRE(euler.bulkViscosityKind == EulerMatBulkType::Enhanced);
    REQUIRE(cardAt<EulerMatTable>(model, 0).spallModelId == 405);
    const FabricMatCard fabric = cardAt<FabricMatTable>(model, 1);
    REQUIRE(fabric.coatingThicknessPercent == Approx(15.0));
    REQUIRE(fabric.warpAngle.value() == Approx(25.0));
    REQUIRE(fabric.weftAngle.value() == Approx(115.0));
    REQUIRE(fabric.lockAngle2 == Approx(16.0));
    REQUIRE(cardAt<FabricMatTable>(model, 0).warpDirection == std::array<double, 3>{1.0, 0.0, 0.0});
    // The SOL 700 form of the MATVE: the four constants of the linear isotropic model.
    const ViscoElasticMatCard isolin = cardAt<ViscoElasticMatTable>(model, 0);
    REQUIRE(isolin.model == ViscoElasticMatModel::IsoLin);
    REQUIRE(isolin.terms.size() == 1);
    REQUIRE(isolin.terms[0][0] == Approx(6.0e7));
    REQUIRE(isolin.terms[0][3] == Approx(0.0));
    requireRoundTrip(model);
    expectInvalid("BulkViscosityMat;1;;1.;1.;1.;1.;1.", "initialMetricRelaxation");
    expectInvalid("BulkViscosityMat;1;TNT", "bulkViscosityKind");
    expectInvalid("IsoMatExplicitExt;1", "equationOfStateId");
    expectInvalid("OrthoMat2DFailure;1;CombinedFailure;;50.;;;;1.;1.;1.;1.",
                  "fiberTensionTheory");                                // COMBINAT needs the five modes
    expectInvalid("OrthoMat2DFailure;1;Hill;;50.", "tensionStrength1"); // a theory needs its stresses
    expectInvalid("OrthoMat2DFailure;1;;;50.;;;;;;;;;;;;;;;;0102", "fiberTensionDegradation"); // 0 or 1 digits only
    expectInvalid("OrthoMat2DFailure;1", "shearStrength");                                     // S is required
    expectInvalid("EulerMat;1;1.", "equationOfStateId");
    expectInvalid("FabricMat;1;850;;;;;;50.;1.;;;;;;1.", "coatingThicknessPercent"); // a coating needs two constants
    // MATVE's own laws still read.
    REQUIRE_NOTHROW(readBulk("ViscoElasticMat;1;IsotropicForm1;1.E-5;;1.E+3;10."));
}

TEST_CASE("a MATF has up to three criteria of three lines and the progressive failure line, a MATTF their tables",
          "[model][cards]")
{
    Model model = readBulk(
        "FailureMat;100;0;Criterion;1;2200.;3600.;2200.;3600.;1800.;2700.;4100.;4100.;4100.;;;;;;;;;;;;;;Criterion;7;"
        "2200.;2700.;"
        "1300.;1800.;;;900.;900.;;;;;;;0.12;0.07;0.09;0.04;0.065;0.025;0.025;0.025;Criterion;4;2200.;3600.;2200.;3600.;"
        "1800.;"
        "2700.;4100.;4100.;4100.\n"
        "FailureMat;101;2;Criterion;7;2200.;2700.;1300.;1800.;;;900.;900.;;;;;;;;;;;;;;;ProgressiveFailure;0.02;0.5;0."
        "3;0.4;0.6\n"
        "FailureMat;102;;Criterion;5;2200.;2700.;1300.;1800.;1800.;2700.;4100.;4100.;4100.;1.5;-0.3;-0.2;-0.1\n"
        "FailureMatTemperature;100;;12;1;51;52;53;54;55;56;61;62;63;;;;;;;;;;;;;;12;2;;;;;;;;;;;;;;;71;72;73;74;75;76;"
        "77;78;12;4;81;82;83;84;85;86;91;92;93;94\n"
        "FailureMatTemperature;101;7;12;1;1");
    const FailureMatCard three = cardAt<FailureMatTable>(model, 0);
    REQUIRE(three.criteria.size() == 3);
    REQUIRE(three.criteria[0].criterion == 1);
    REQUIRE(three.criteria[0].values[6].value() == Approx(4100.0)); // V7, the first value of the second line
    REQUIRE_FALSE(three.criteria[0].values[9].has_value());
    REQUIRE(three.criteria[1].criterion == 7);
    REQUIRE(three.criteria[1].strainValues[0].has_value() == false);
    REQUIRE(three.criteria[1].strainValues[1].value() == Approx(0.12)); // W2, the first value of the third line
    REQUIRE(three.criteria[2].criterion == 4);
    const FailureMatCard progressive = cardAt<FailureMatTable>(model, 1);
    REQUIRE(progressive.degradationKind == 2);
    REQUIRE(progressive.progressiveFailure.has_value());
    REQUIRE((*progressive.progressiveFailure)[4].value() == Approx(0.6));
    const FailureMatCard tsaiWu = cardAt<FailureMatTable>(model, 2);
    REQUIRE(tsaiWu.criteria[0].failureIndexScale.value() == Approx(1.5));
    REQUIRE(tsaiWu.criteria[0].values[9].value() == Approx(-0.3)); // the interactive strength constants may be negative
    const FailureMatTemperatureCard variation = cardAt<FailureMatTemperatureTable>(model, 0);
    REQUIRE(variation.blocks.size() == 3);
    REQUIRE(variation.blocks[0].tables[0] == 51);  // T(Xt)
    REQUIRE(variation.blocks[0].tables[8] == 63);  // T(Szx), the third value of the second line
    REQUIRE(variation.blocks[1].tables[14] == 71); // T(Exc), the first value of the third line
    REQUIRE(variation.blocks[2].tables[9] == 94);  // T(Find)
    REQUIRE(cardAt<FailureMatTemperatureTable>(model, 1).bondShearAllowableTableId == 7);
    requireRoundTrip(model);
    // The refusals.
    expectInvalid("FailureMat;1;1;Criterion;1;1.", "degradationKind");
    expectInvalid("FailureMat;1;2;Criterion;6;1.", "criterion");
    expectInvalid("FailureMat;1;;Criterion;1;1.;;;;;;;;;;;;;;;;;;;;;;ProgressiveFailure;0.1",
                  "progressiveFailure"); // PF needs a progressive failure type
    expectInvalid(
        "FailureMat;1;2;Criterion;1;1.;;;;;;;;;;;;;;;;;;;;;;ProgressiveFailure;0.1;;;;;ProgressiveFailure;0.1"); // two
                                                                                                                 // PF
                                                                                                                 // lines
    expectInvalid(
        "FailureMat;1;;Criterion;1;1.;;;;;;;;;;;;;;;;;;;;;;Criterion;1;1.;;;;;;;;;;;;;;;;;;;;;;Criterion;1;1.;;;;;;;;;;"
        ";;;;;;;;;;;;Criterion;1;"
        "1.",
        "criteria"); // four criteria
    expectInvalid("FailureMat;1", "criteria");
    expectInvalid("FailureMatTemperature;1;;0;1", "kind");
    expectInvalid("FailureMatTemperature;1;;12;-1;1", "criterion");
}

TEST_CASE("a MATF of SOL 700 has one criterion from 101 to 105, its words in the cells the format gives them",
          "[model][cards]")
{
    Model model = readBulk(
        "FailureMat;1;;Criterion;103;0.05\n"
        "FailureMat;2;;Criterion;102;0.1;0.2;;;;1.;;;20.;1650.;520.;Discard\n"
        "FailureMat;3;;Criterion;105;0.1;0.2;0.3;0.4;0.5;0.6;;;1.;0.;0.;0.;SoilModel;VectorizedIterations;;;-1.");
    const FailureMatCard plastic = cardAt<FailureMatTable>(model, 0);
    REQUIRE((plastic.criteria.size() == 1 && plastic.criteria[0].criterion == 103));
    REQUIRE(plastic.criteria[0].values[0].value() == Approx(0.05)); // MPS
    const FailureMatCard johnsonCook = cardAt<FailureMatTable>(model, 1);
    REQUIRE(johnsonCook.criteria[0].values[5].value() == Approx(1.0));           // 0DOT
    REQUIRE(johnsonCook.criteria[0].values[8].value() == Approx(20.0));          // TROOM
    REQUIRE(johnsonCook.criteria[0].failureIndexScale.value() == Approx(1650.)); // TMELT, in the cell of FIND
    REQUIRE(johnsonCook.criteria[0].words == std::array<std::string, 2>{"Discard", ""});
    const FailureMatCard cap = cardAt<FailureMatTable>(model, 2);
    REQUIRE(cap.criteria[0].words == std::array<std::string, 2>{"SoilModel", "VectorizedIterations"});
    REQUIRE(cap.criteria[0].strainValues[3].value() == Approx(-1.0)); // TOFF, in the cell of W4
    requireRoundTrip(model);
    expectInvalid("FailureMat;1;2;Criterion;103;0.05", "criteria");              // no ITYPE in the SOL 700 form
    expectInvalid("FailureMat;1;;Criterion;103", "criterionValue");              // MPS is required
    expectInvalid("FailureMat;1;;Criterion;102;0.1;;;;;;;;;;;BLUE", "criteria"); // MTH is CONT or DISC
    expectInvalid("FailureMat;1;;Criterion;103;0.05;;;;;;;;;;;;;;;;;;;;;;Criterion;1;1.",
                  "criteria"); // the only criterion of its card
    expectInvalid("FailureMat;1;;Criterion;106;0.05", "criterion");
}

TEST_CASE(
    "a MATFTG has its blocks in any order, the MATID list with THRU, and no reference for the ID with a MATID line",
    "[model][cards]")
{
    Model model = readBulk(
        "FatigueMat;9;;;Static;410;655;77\n"
        "FatigueMat;12;;;Static;410;655;;;;0.1;;;StressLifeMethod;2870;-0.1246;1.E+8;0.;1.E+8\n"
        "FatigueMat;13;;;Static;410;655;;;;0.1;;;MatIds;2.1E+5;0.3;;4;9;Through;15;18\n"
        "FatigueMat;10;1.5;;Static;410;655;77;MeanStress;-1.;0.1;0.5;50.;StressLifeSegment1;2870;-0.12;1.E+8;0.;1.E+8;"
        "0.1;0.5;-0.1;-0.2;-0."
        "3;-0.4;0.5;1.5;0.2;1.;2.;3.;4.;5.;6.;7.;8.;9.;10.;11.;12.;StressLifeMethod;3.E+3;-0.12;2.E+8;;;;;;;;;;;;;;;;;;"
        ";;;;;;Table;4;"
        "1.;11;2.;12;3.;13;4.;14;MetallicHandbookData;50.;-20.;0.5;10.;StrainLifeMethod;1.E+3;-0.1;-0.5;0.5;0.1;1500.;"
        "2.E+8;0.01;0.02;0.03;1.E+8;1."
        "E-5;MatIds;2.1E+5;0.3;;8;9;Through;15;18;20;21;22;23\n"
        "FatigueMat;11;1.;1;Static;410;655;;Amplitude;;;;;StressLifeMethod;2870;-0.1246;1.E+8;0.;1.E+8");
    const FatigueMatCard plain = cardAt<FatigueMatTable>(model, 0);
    REQUIRE(plain.fixed.yieldStrength.value() == Approx(410.0));
    REQUIRE(plain.fixed.materialCode.value() == 77);
    REQUIRE_FALSE(plain.curves[0].has_value());
    const FatigueMatCard curve = cardAt<FatigueMatTable>(model, 1);
    REQUIRE(curve.fixed.standardError.value() == Approx(0.1)); // the eighth field of the STATIC line is SE
    REQUIRE(curve.curves[0].has_value());
    REQUIRE(curve.curves[0]->curve[1].value() == Approx(-0.1246));
    const FatigueMatCard listed = cardAt<FatigueMatTable>(model, 2);
    REQUIRE(listed.materials.has_value());
    REQUIRE(listed.materials->youngsModulus.value() == Approx(2.1e5));
    REQUIRE(listed.materials->matIds.size() == 2);
    REQUIRE(listed.materials->matIds[0].first == 9);
    REQUIRE(listed.materials->matIds[0].last == 15); // 9 THRU 15
    REQUIRE(listed.materials->matIds[1].first == 18);
    const FatigueMatCard every = cardAt<FatigueMatTable>(model, 3);
    REQUIRE(every.stressUnitFactor == Approx(1.5));
    REQUIRE(every.fixed.curveKind == "MeanStress");
    REQUIRE(every.fixed.areaReduction.value() == Approx(50.0)); // on the second line of STATIC
    REQUIRE(every.curves[1].has_value());                       // SNS1
    REQUIRE(every.curves[1]->meanStress[0].value() == Approx(-0.1));
    REQUIRE(every.curves[1]->spotWeld[11].value() == Approx(12.0)); // SF-FZ to TE-MZ: the last spot weld value
    REQUIRE(every.table.has_value());
    REQUIRE(every.table->size() == 4);
    REQUIRE((*every.table)[3].second == 14);
    REQUIRE(every.handbookCoefficients.has_value());
    REQUIRE(every.strainLife.has_value());
    REQUIRE(every.strainLife->errors[3].value() == Approx(1.0e8));
    REQUIRE(every.strainLife->errors[4].value() == Approx(1.0e-5)); // TOLER is the eighth field of the second line
    REQUIRE(every.materials->matIds.size() == 6);
    REQUIRE(cardAt<FatigueMatTable>(model, 4).debugOutput == 1);
    requireRoundTrip(model);
    // The ID is a reference to the MAT1 only when there is no MATID line, and the single MIDi are references then.
    RecordingVisitor plainVisitor;
    model.table<FatigueMatTable>().visitReferences(0, plainVisitor);
    REQUIRE(plainVisitor.fields == std::vector<std::string>{"id"});
    RecordingVisitor listedVisitor;
    model.table<FatigueMatTable>().visitReferences(2, listedVisitor);
    REQUIRE(listedVisitor.fields == std::vector<std::string>{"matIds"}); // 18; the range 9 THRU 15 is left as written
    expectInvalid("FatigueMat;1", "staticProperties");
    expectInvalid("FatigueMat;1;;;Static;410;655;;;;;;;Static;410;655", "staticProperties");
    expectInvalid("FatigueMat;1;;;Static;410;655;;TYPE"); // TYPE is a known word only
    expectInvalid("FatigueMat;1;;;Static;410;655;;;;;;;MatIds;2.1E+5;0.3;;3;15;Through;9", "matIds");
    expectInvalid("FatigueMat;1;;;Static;410;655;;;;;;;MatIds;2.1E+5;0.3;;2;Through;9");
    expectInvalid("FatigueMat;1;;;Static;410;655;;;;;;;MatIds;2.1E+5;0.3"); // the MATID line needs a MIDi
    expectInvalid("FatigueMat;1;;;Static;410;655;;;;;;;Table;1;1.");        // a VALUE without its TID
    expectInvalid(
        "FatigueMat;1;;;Static;410;655;;;;;;;StressLifeMethod;1.;;;;;;;;;;;;;;;;;;;;;;;;;;StressLifeMethod;2.");
    expectInvalid("FatigueMat;1;;;Static;410;655;;;;11.", "standardError"); // SE is from 0.0 to 10.0
}

TEST_CASE("NLMOPTS options come in any order, each at most once, and have their values checked", "[model][cards]")
{
    Model model = readBulk(
        "NonlinearMatOptions;Creep;0;;;;Hemicube;1;500;0;0.01\nNonlinearMatOptions;MapTolerance;0.2;Creep;;;;;"
        "AssumedStrainOption;;Deactivation;;"
        "1\nNonlinearMatOptions;LargeStrain;-1");
    const NonlinearMatOptionsCard first = cardAt<NonlinearMatOptionsTable>(model, 0);
    REQUIRE(first.creep.has_value());
    REQUIRE(first.creep->implicitTangent == 0);
    REQUIRE(first.hemicube.has_value());
    REQUIRE(first.hemicube->pixelCount == 500);
    REQUIRE(first.hemicube->implicitFraction == Approx(0.01)); // the sixth value, after a blank field and CUTOFF
    REQUIRE_FALSE(first.hemicube->reflectionTolerance.has_value());
    const NonlinearMatOptionsCard second = cardAt<NonlinearMatOptionsTable>(model, 1);
    REQUIRE(second.mapTolerance.value() == Approx(0.2));
    REQUIRE(second.creep.has_value()); // the keyword alone is the default option
    REQUIRE(second.assumedStrain.value() == "");
    REQUIRE_FALSE(second.deactivation->rule.has_value());
    REQUIRE(second.deactivation->output == 1);
    REQUIRE(cardAt<NonlinearMatOptionsTable>(model, 2).largeStrain.value() == -1);
    requireRoundTrip(model);
    expectInvalid("NonlinearMatOptions;Creep;0;;;;Creep;1");
    expectInvalid("NonlinearMatOptions;Creep;0;0;0;3", "implicitCreepTangent");
    expectInvalid("NonlinearMatOptions;LargeStrain;3", "largeStrain");
    expectInvalid("NonlinearMatOptions;AssumedStrainOption;MAYBE", "assumedStrain");
    expectInvalid("NonlinearMatOptions;PropertyMap;4", "propertyMapping");
    expectInvalid("NonlinearMatOptions;PropertyMap;0;PLANE", "propertyBehaviour");
    expectInvalid("NonlinearMatOptions;TemperatureDistribution;CUBIC", "temperatureDistribution");
    expectInvalid("NonlinearMatOptions;MapTolerance;0.", "mapTolerance");
}

// ------------------------------------------------------------------------------------------------ the frequency tables

TEST_CASE("the frequency dependent cards have their tables where the material has its properties", "[model][cards]")
{
    Model model = readBulk(
        "IsoMatFreq;33;14;23;18;19\n"
        "AnisoMat2DFreq;34;;;;;;;;44;46;52;44;46;52\n"
        "AnisoMat2DFreq;35;1;2;3;4;5;6;7;11;12;13;14;15;16\n"
        "OrthoMat2DFreq;76;;;;;;;93\n"
        "OrthoMat2DFreq;77;1;2;3;4;5;6;8\n"
        "AnisoMat3DFreq;101;5;;;;;;5;;;;;5;;;;;;;5;;5\n"
        "AnisoMat3DFreq;102;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;19;20;21;31;41;42;43;44;45;46;47;48;49;50;51;"
        "52;53;54;55;56;57;58;59;60;61\n"
        "FluidMatFreq;103;555\n"
        "FluidMatFreq;104;1;2;3;4;5");
    REQUIRE(cardAt<IsoMatFreqTable>(model).structuralDampingTableId == 19); // GE is in the eighth field, as on a MAT1
    REQUIRE(cardAt<AnisoMat2DFreqTable>(model, 0).structuralDampingTermTableIds[0] == 44); // GE11 is field 18
    REQUIRE(cardAt<AnisoMat2DFreqTable>(model, 1).structuralDampingTableId == 7);          // GE is field 13
    REQUIRE(cardAt<AnisoMat2DFreqTable>(model, 1).stiffnessTableIds == std::array<base::EntityId, 6>{1, 2, 3, 4, 5, 6});
    REQUIRE(cardAt<OrthoMat2DFreqTable>(model, 0).structuralDampingTableId == 93); // GE is field 17
    REQUIRE(cardAt<OrthoMat2DFreqTable>(model, 1).shearModulus2ZTableId == 6);
    const AnisoMat3DFreqCard single = cardAt<AnisoMat3DFreqTable>(model, 0);
    REQUIRE(single.stiffnessTableIds[0] == 5);  // G11
    REQUIRE(single.stiffnessTableIds[6] == 5);  // G22 is field 8
    REQUIRE(single.stiffnessTableIds[11] == 5); // G33 is field 13
    REQUIRE(single.stiffnessTableIds[18] == 5); // G55 is field 20
    REQUIRE(single.stiffnessTableIds[20] == 5); // G66 is field 22
    const AnisoMat3DFreqCard everyTable = cardAt<AnisoMat3DFreqTable>(model, 1);
    REQUIRE(everyTable.stiffnessTableIds[20] == 21);
    REQUIRE(everyTable.structuralDampingTableId == 31); // GE is field 31
    REQUIRE(everyTable.structuralDampingTermTableIds[0] == 41);
    REQUIRE(everyTable.structuralDampingTermTableIds[20] == 61); // GE66 is field 53
    REQUIRE(cardAt<FluidMatFreqTable>(model, 1).normalizedAdmittanceTableId == 5);
    requireRoundTrip(model);
    // The tables are TABLEDi cards, the ID the material.
    RecordingVisitor visitor;
    model.table<IsoMatFreqTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "youngsModulusTableId", "shearModulusTableId",
                                                       "poissonRatioTableId", "structuralDampingTableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::DynamicTable,
                                                            ReferenceTarget::DynamicTable,
                                                            ReferenceTarget::DynamicTable,
                                                            ReferenceTarget::DynamicTable});
    expectInvalid("IsoMatFreq;1;-5", "youngsModulusTableId");
    expectInvalid("IsoMatFreq;0;5", "id");
}

// ------------------------------------------------------------------------------------------------ references and
// numbering

TEST_CASE("the new companions refer to their material and their tables, and follow their renumbering", "[model][cards]")
{
    Model model = readBulk(
        "IsoMat;10;2.1E+11;;0.3\n"
        "IsoMat;20;2.1E+11;;0.3\n"
        "OrthoMat2D;30;1.;1.;0.3\n"
        "AnisoMat2D;40;1.\n"
        "UserMat;50;0;1;1.\n"
        "ElastoPlasticMatTemperature;10;201;;202\n"
        "ElastoPlasticMatTemperature;30;;310\n"
        "HyperelasticMatTemperature;20;MooneyRivlin;201\n"
        "ViscoElasticMatTemperature;10;WilliamsLandelFerry;100.;;;;;;0.8;1.2\n"
        "UserMatTable;50;201;;;;201\n"
        "BulkViscosityMat;10\n"
        "GammaLawStateEquation;99\n"
        "IsoMatExplicitExt;10;99\n"
        "OrthoMat2DFailure;30;;;50.\n"
        "PoroElasticMatFreq;10;201\n"
        "IsoMatFreq;10;501\n"
        "AnisoMat2DFreq;40;501\n"
        "OrthoMat2DFreq;30;501\n"
        "FailureMat;10;;Criterion;1;1.\n"
        "FailureMatTemperature;10;;12;1;201\n"
        "UserMatService;50;UserMat;GRP;UMAT\n"
        "ViscoPlasticMat;10;1.\n"
        "FatigueMat;10;;;Static;410;655\n"
        "TableMaterial1;201;;;0.;1.;1.;2.;EndTable\n"
        "TableMaterial1;202;;;0.;1.;1.;2.;EndTable\n"
        "TableStress1;310;;0.;0.;1.;1.;EndTable\n"
        "TableDynamic1;501;;;0.;1.;1.;2.;EndTable");
    REQUIRE(unresolvedIn(model) == 0);
    // Each companion has the ID space of its card type, so the same MID in all of them is not a clash.
    REQUIRE(model.locate(base::IdSpace::Material, 10).has_value());
    REQUIRE(model.locateInType(typeIndexOf<ElastoPlasticMatTemperatureTable>(), 10).has_value());
    REQUIRE(model.locateInType(typeIndexOf<BulkViscosityMatTable>(), 10).has_value());
    REQUIRE(model.locateInType(typeIndexOf<ViscoPlasticMatTable>(), 10).has_value());
    RecordingVisitor visitor;
    model.table<ElastoPlasticMatTemperatureTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "yieldStressTableId", "hardeningSlopeTableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::MaterialTable,
                                                            ReferenceTarget::MaterialTable});
    RecordingVisitor stressVisitor;
    model.table<ElastoPlasticMatTemperatureTable>().visitReferences(1, stressVisitor);
    REQUIRE(stressVisitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::StressTable});
    // Renumbering the materials follows into every companion.
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE(cardAt<ElastoPlasticMatTemperatureTable>(model).id == 1010);
    REQUIRE(cardAt<ElastoPlasticMatTemperatureTable>(model, 1).id == 1030);
    REQUIRE(cardAt<HyperelasticMatTemperatureTable>(model).id == 1020);
    REQUIRE(cardAt<ViscoElasticMatTemperatureTable>(model).id == 1010);
    REQUIRE(cardAt<UserMatTableTable>(model).id == 1050);
    REQUIRE(cardAt<UserMatTable>(model).id == 1050);
    REQUIRE(cardAt<UserMatServiceTable>(model).id == 1050);
    REQUIRE(cardAt<BulkViscosityMatTable>(model).id == 1010);
    REQUIRE(cardAt<IsoMatExplicitExtTable>(model).id == 1010);
    REQUIRE(cardAt<IsoMatExplicitExtTable>(model).equationOfStateId ==
            99); // an equation of state is not a material: a
                 // renumbering of the materials leaves it alone
    Renumbering::apply(model, RenumberSpace::EquationOfState, Renumbering::offset(1));
    REQUIRE(cardAt<IsoMatExplicitExtTable>(model).equationOfStateId == 100);
    REQUIRE(cardAt<OrthoMat2DFailureTable>(model).id == 1030);
    REQUIRE(cardAt<PoroElasticMatFreqTable>(model).id == 1010);
    REQUIRE(cardAt<IsoMatFreqTable>(model).id == 1010);
    REQUIRE(cardAt<AnisoMat2DFreqTable>(model).id == 1040);
    REQUIRE(cardAt<OrthoMat2DFreqTable>(model).id == 1030);
    REQUIRE(cardAt<FailureMatTable>(model).id == 1010);
    REQUIRE(cardAt<FailureMatTemperatureTable>(model).id == 1010);
    REQUIRE(cardAt<ViscoPlasticMatTable>(model).id == 1010);
    REQUIRE(cardAt<FatigueMatTable>(model).id == 1010);
    REQUIRE(unresolvedIn(model) == 0);
    // The tables follow the space they are in.
    Renumbering::apply(model, RenumberSpace::MaterialTable, Renumbering::offset(5000));
    REQUIRE(cardAt<ElastoPlasticMatTemperatureTable>(model).yieldStressTableId == 5201);
    REQUIRE(cardAt<ElastoPlasticMatTemperatureTable>(model).hardeningSlopeTableId == 5202);
    REQUIRE(cardAt<ElastoPlasticMatTemperatureTable>(model, 1).stressStrainCurvesTableId == 310);
    REQUIRE(cardAt<HyperelasticMatTemperatureTable>(model).bulkModulusTableId == 5201);
    REQUIRE(cardAt<HyperelasticMatTemperatureTable>(model).coefficientCTableIds[0] == 0);
    REQUIRE(cardAt<UserMatTableTable>(model).structuralDampingTableId == 5201);
    // T(RHO) may be a TABL3Di too: this one is a TABLEM1.
    REQUIRE(cardAt<UserMatTableTable>(model).densityTableId == 5201);
    REQUIRE(cardAt<PoroElasticMatFreqTable>(model).youngsModulusTableId == 5201);
    REQUIRE(cardAt<FailureMatTemperatureTable>(model).blocks[0].tables[0] == 5201);
    Renumbering::apply(model, RenumberSpace::StressTable, Renumbering::offset(7000));
    REQUIRE(cardAt<ElastoPlasticMatTemperatureTable>(model, 1).stressStrainCurvesTableId == 7310);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(9000));
    REQUIRE(cardAt<IsoMatFreqTable>(model).youngsModulusTableId == 9501);
    REQUIRE(cardAt<AnisoMat2DFreqTable>(model).stiffnessTableIds[0] == 9501);
    REQUIRE(cardAt<OrthoMat2DFreqTable>(model).youngsModulus1TableId == 9501);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    // What is not there is reported: the companions of a missing material and a missing table.
    Model lacking = readBulk("ElastoPlasticMatTemperature;10;201;;202\nBulkViscosityMat;10\nIsoMatFreq;10;501");
    REQUIRE(unresolvedIn(lacking) == 6);
}

TEST_CASE("the MATFTG with a MATID line and the materials of SOL 700 are primaries of the shared material space",
          "[model][cards]")
{
    // The primaries share one ID space; a MATFTG with a MATID line has an ID of its own that is not a material.
    REQUIRE_NOTHROW(
        readBulk("IsoMat;9;1.;;0.3\nFatigueMat;9;;;Static;410;655\nFatigueMat;13;;;Static;410;655;;;;;;;MatIds;2.1E+5;"
                 "0.3;;1;9"));
    REQUIRE_THROWS_AS(readBulk("EulerMat;6;2600.;120\nFabricMat;6;780;;;;;;;1.;;;;;;1."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("MicromechanicsCompositeMat;6;10\nRigidMat;6"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ProgressiveFailureMat;6;CuntzeBold;1.\nShapeMemoryMat;6;1;77.;0.05"), base::DeckError);
    Model model = readBulk(
        "IsoMat;9;1.;;0.3\nFatigueMat;13;;;Static;410;655;;;;;;;MatIds;2.1E+5;0.3;;2;9;18\nFatigueMat;9;;;Static;410;"
        "655");
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE(cardAt<FatigueMatTable>(model, 0).id == 13);
    REQUIRE(cardAt<FatigueMatTable>(model, 0).materials->matIds[0].first == 109);
    REQUIRE(cardAt<FatigueMatTable>(model, 1).id == 109);
}
