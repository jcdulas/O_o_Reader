// Checks of the cards of solution control, dynamics, damping, random analysis and tables (FREQ3 to
// FREQ5, EIGB, EIGC, EIGP, TSTEP, TSTEPNL, NLPARM, NLPCI, TIC, TF, DELAY, DPHASE, TABDMP1, TABRND1, TABRNDG, TABLED5,
// RANDPS, RANDT1, MODTRAK, MDLPRM, RGYRO, ROTORG, RCROSS, RVDOF, RVDOF1, UNBALNC, DTI): the positions of the fields on
// the continuation lines, the defaults (stored resolved or left blank), the errors by field, the references and the way
// the ID spaces and the Case Control commands resolve and renumber.

#include "oor/model/cards/dynamics/bucklingEigenMethod/BucklingEigenMethodTable.hpp"
#include "oor/model/cards/dynamics/complexEigenMethod/ComplexEigenMethodTable.hpp"
#include "oor/model/cards/dynamics/complexEigenPoles/ComplexEigenPolesTable.hpp"
#include "oor/model/cards/dynamics/dynamicLoadDelay/DynamicLoadDelayTable.hpp"
#include "oor/model/cards/dynamics/dynamicLoadPhase/DynamicLoadPhaseTable.hpp"
#include "oor/model/cards/dynamics/freqBetweenModes3/FreqBetweenModes3Table.hpp"
#include "oor/model/cards/dynamics/freqModeFractions5/FreqModeFractions5Table.hpp"
#include "oor/model/cards/dynamics/freqResponseLoad1/FreqResponseLoad1Table.hpp"
#include "oor/model/cards/dynamics/freqSpreadAroundModes4/FreqSpreadAroundModes4Table.hpp"
#include "oor/model/cards/dynamics/lanczosEigenMethod/LanczosEigenMethodTable.hpp"
#include "oor/model/cards/dynamics/randomSpectralDensity/RandomSpectralDensityTable.hpp"
#include "oor/model/cards/dynamics/randomTimeLags/RandomTimeLagsTable.hpp"
#include "oor/model/cards/dynamics/rotorUnbalanceLoad/RotorUnbalanceLoadTable.hpp"
#include "oor/model/cards/dynamics/transferFunction/TransferFunctionTable.hpp"
#include "oor/model/cards/dynamics/transientInitialCondition/TransientInitialConditionTable.hpp"
#include "oor/model/cards/dynamics/transientTableLoad1/TransientTableLoad1Table.hpp"
#include "oor/model/cards/matrices/directTable/DirectTableTable.hpp"
#include "oor/model/cards/solution_control/arcLengthParams/ArcLengthParamsTable.hpp"
#include "oor/model/cards/solution_control/crossSpectrumPair/CrossSpectrumPairTable.hpp"
#include "oor/model/cards/solution_control/modeTracking/ModeTrackingTable.hpp"
#include "oor/model/cards/solution_control/modelParams/ModelParamsTable.hpp"
#include "oor/model/cards/solution_control/nonlinearStaticParams/NonlinearStaticParamsTable.hpp"
#include "oor/model/cards/solution_control/nonlinearTimeSteps/NonlinearTimeStepsTable.hpp"
#include "oor/model/cards/solution_control/residualVectorDofs/ResidualVectorDofsTable.hpp"
#include "oor/model/cards/solution_control/residualVectorDofs1/ResidualVectorDofs1Table.hpp"
#include "oor/model/cards/solution_control/rotorLineNodes/RotorLineNodesTable.hpp"
#include "oor/model/cards/solution_control/rotordynamicParams/RotordynamicParamsTable.hpp"
#include "oor/model/cards/solution_control/transientTimeSteps/TransientTimeStepsTable.hpp"
#include "oor/model/cards/tables/tableDynamic5/TableDynamic5Table.hpp"
#include "oor/model/cards/tables/tableGustSpectrum/TableGustSpectrumTable.hpp"
#include "oor/model/cards/tables/tableModalDamping1/TableModalDamping1Table.hpp"
#include "oor/model/cards/tables/tableRandomSpectrum1/TableRandomSpectrum1Table.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using Catch::Approx;
using oor::testing::readModelText;
using oor::testing::writeModelText;

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

template <typename TableT>
std::size_t rowCount(const Model& model)
{
    const TableT* table = model.findTable<TableT>();
    return table == nullptr ? 0 : table->rows().rowCount();
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

// The number of a Case Control command of the global section.
std::int64_t globalCommand(const Model& model, const char* name)
{
    const CaseEntry* entry = CaseControl::ownCommand(model.control().caseControl.global(), name);
    REQUIRE(entry != nullptr);
    return entry->value.integer;
}

constexpr double INFINITE_END = 1.0E20;

} // namespace

// ------------------------------------------------------------------------------------------------ frequency sets

TEST_CASE("FREQ3, FREQ4 and FREQ5 have their defaults, their fields and join the frequency sets", "[model][cards]")
{
    Model model = readBulk(
        "FreqBetweenModes3;1;25.\n"
        "FreqBetweenModes3;2;15.;1500.;Logarithmic;6;1.5\n"
        "FreqSpreadAroundModes4;3\n"
        "FreqSpreadAroundModes4;4;4.;80.;0.03;5\n"
        "FreqModeFractions5;5;;;0.5\n"
        "FreqModeFractions5;6;1.;9.;1.;2.;3.;4.;5.;6.;7.\n"
        "FreqList;3;1.;2.\n"
        "FreqLinearSteps1;3;3.;1.");
    FreqBetweenModes3Card freq3 = cardAt<FreqBetweenModes3Table>(model, 0);
    REQUIRE(
        (freq3.id == 1 && freq3.frequency1 == 25.0 && freq3.frequency2 == 25.0)); // a blank F2 is F1, stored resolved
    REQUIRE((freq3.kind == FrequencySpacing::Linear && freq3.frequencyCount == 10 && freq3.clustering == 1.0));
    freq3 = cardAt<FreqBetweenModes3Table>(model, 1);
    REQUIRE((freq3.frequency2 == 1500.0 && freq3.kind == FrequencySpacing::Log && freq3.frequencyCount == 6 &&
             freq3.clustering == 1.5));
    FreqSpreadAroundModes4Card freq4 = cardAt<FreqSpreadAroundModes4Table>(model, 0);
    REQUIRE((freq4.frequency1 == 0.0 && freq4.frequency2 == INFINITE_END && freq4.frequencySpread == 0.1 &&
             freq4.frequencyCount == 3));
    freq4 = cardAt<FreqSpreadAroundModes4Table>(model, 1);
    REQUIRE((freq4.frequency1 == 4.0 && freq4.frequency2 == 80.0 && freq4.frequencySpread == 0.03 &&
             freq4.frequencyCount == 5));
    FreqModeFractions5Card freq5 = cardAt<FreqModeFractions5Table>(model, 0);
    REQUIRE(
        (freq5.frequency1 == 0.0 && freq5.frequency2 == INFINITE_END && freq5.fractions == std::vector<double>{0.5}));
    freq5 = cardAt<FreqModeFractions5Table>(model, 1);
    REQUIRE((freq5.frequency1 == 1.0 && freq5.frequency2 == 9.0));
    REQUIRE(freq5.fractions == std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0});
    // The set ID is shared with FREQ and FREQ1: the cards of the set 3 are one FREQ3-less set of three kinds.
    REQUIRE(cardsOfSet(model, CardCategory::Frequency, 3).size() == 3);
    REQUIRE(model.findTable<FreqSpreadAroundModes4Table>()->INFO.idSpace == base::IdSpace::Group);
    requireRoundTrip(model);

    // The defaults are left blank when written.
    const Model plain = readBulk("FreqBetweenModes3;1;20.;30.\nFreqSpreadAroundModes4;2\nFreqModeFractions5;3;;;0.5");

    oor::testing::requireModelFileRoundTrip(plain);

    expectInvalid("FreqBetweenModes3;1;-1.", "frequency1");
    expectInvalid("FreqBetweenModes3;1;0.;30.;Logarithmic", "frequency1");
    expectInvalid("FreqBetweenModes3;1;20.;10.", "frequency2");
    expectInvalid("FreqBetweenModes3;1;20.;30.;Logarithmic;1", "frequencyCount");
    expectInvalid("FreqBetweenModes3;1;20.;30.;Logarithmic;10;0.", "clustering");
    expectInvalid("FreqBetweenModes3;1;20.;30.;CUBIC", "kind");
    expectInvalid("FreqBetweenModes3;1", "frequency1");
    expectInvalid("FreqSpreadAroundModes4;1;0.;10.;1.5", "frequencySpread");
    expectInvalid("FreqSpreadAroundModes4;1;10.;5.", "frequency2");
    expectInvalid("FreqSpreadAroundModes4;1;0.;10.;0.1;0", "frequencyCount");
    expectInvalid("FreqModeFractions5;1", "fraction1");
    expectInvalid("FreqModeFractions5;1;0.;10.;-1.", "fractions");
    expectInvalid("FreqModeFractions5;1;5.;5.;1.", "frequency2");
}

// ------------------------------------------------------------------------------------------------ eigenvalue methods

TEST_CASE("EIGB resolves NDP and NDN from NEP, and its point is a node reference", "[model][cards]")
{
    Model model = readBulk(
        "Node;5;;0.;0.;0.\nBucklingEigenMethod;1;InverseIteration;0.;80.;15\nBucklingEigenMethod;2;SturmInverse;1.5;"
        "900.;3;2;1;Point;5;"
        "2\nBucklingEigenMethod;3;;0.;5.");
    BucklingEigenMethodCard card = cardAt<BucklingEigenMethodTable>(model, 0);
    REQUIRE((card.method == BucklingMethod::Inv && card.lowerEigenvalue == 0.0 && card.upperEigenvalue == 80.0));
    REQUIRE((card.estimatedModeCount == 15 && card.desiredPositiveModeCount == 45 &&
             card.desiredNegativeModeCount == 45)); // 3 * NEP, stored resolved
    REQUIRE((card.norm == BucklingNorm::Max && card.normNode == 0 && card.normComponent == 0));
    card = cardAt<BucklingEigenMethodTable>(model, 1);
    REQUIRE((card.method == BucklingMethod::Sinv && card.estimatedModeCount == 3 &&
             card.desiredPositiveModeCount == 2 && card.desiredNegativeModeCount == 1));
    REQUIRE((card.norm == BucklingNorm::Point && card.normNode == 5 && card.normComponent == 2));
    card = cardAt<BucklingEigenMethodTable>(model, 2);
    REQUIRE((card.method == BucklingMethod::NotGiven && card.estimatedModeCount == 0 &&
             card.desiredPositiveModeCount == 0 && card.desiredNegativeModeCount == 0));

    (void)0; // always written once resolved
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<BucklingEigenMethodTable>(model, 1).normNode == 105);
    // The ID is in the method space with EIGR and EIGRL.
    Renumbering::apply(model, RenumberSpace::Method, Renumbering::offset(10));
    REQUIRE(cardAt<BucklingEigenMethodTable>(model, 0).id == 11);
    requireRoundTrip(model);

    expectInvalid("BucklingEigenMethod;1;InverseIteration;5.;5.", "lowerEigenvalue");
    expectInvalid("BucklingEigenMethod;1;XXX;0.;5.", "method");
    expectInvalid("BucklingEigenMethod;1;InverseIteration;0.;5.;-1", "estimatedModeCount");
    expectInvalid("BucklingEigenMethod;1;InverseIteration;0.", "upperEigenvalue");
    REQUIRE_THROWS_AS(readBulk("LanczosEigenMethod;1;;;5\nBucklingEigenMethod;1;InverseIteration;0.;5."),
                      base::DeckError); // one method space
}

TEST_CASE("EIGC has the lines of its search regions in the layout of the method", "[model][cards]")
{
    const Model model = readBulk(
        "ComplexEigenMethod;1;InverseIteration;Point;5;3;2.E-4;;1.5;2.5;3.5;4.5;5.5;7;;12.;24.\n"
        "ComplexEigenMethod;2;ComplexLanczos;;;;;;0.;6.;12.;4.;7;;5\n"
        "ComplexEigenMethod;3;HessenbergMethod;;;;;4\n"
        "ComplexEigenMethod;4;ImplicitRestartedArnoldi;Maximum;;;;10\n"
        "ComplexEigenMethod;5;DeterminantMethod;;;;;;0.;0.;0.;90.;5.;3;9");
    ComplexEigenMethodCard card = cardAt<ComplexEigenMethodTable>(model, 0);
    REQUIRE((card.methodName == ComplexMethod::Inv && card.norm == ComplexNorm::Point && card.normNode == 5 &&
             card.normComponent == 3));
    REQUIRE((card.convergenceTolerance.has_value() && *card.convergenceTolerance == 2.0E-4 &&
             card.desiredModeCount0 == 0 && card.regions.size() == 2));
    REQUIRE((card.regions[0].alphaA == 1.5 && card.regions[0].omegaA == 2.5 && card.regions[0].alphaB == 3.5));
    REQUIRE((card.regions[0].omegaB == 4.5 && card.regions[0].regionWidth == 5.5 &&
             card.regions[0].estimatedModeCount == 7));
    REQUIRE(card.regions[0].desiredModeCount ==
            21); // a blank NDJ of the inverse power method is 3 * NEJ, stored resolved
    REQUIRE((card.regions[1].alphaA == 12.0 && card.regions[1].omegaA == 24.0 && card.regions[1].regionWidth == 1.0));
    REQUIRE((card.regions[1].estimatedModeCount == 0 && card.regions[1].desiredModeCount == 0));
    card = cardAt<ComplexEigenMethodTable>(model, 1);
    REQUIRE((card.methodName == ComplexMethod::Clan && card.regions.size() == 1)); // the block Lanczos layout
    REQUIRE((card.regions[0].alphaA == 0.0 && card.regions[0].omegaA == 6.0));
    REQUIRE((card.regions[0].maximumBlockSize == 12.0 && card.regions[0].initialBlockSize == 4.0 &&
             card.regions[0].solveInterval == 7));
    REQUIRE(card.regions[0].desiredModeCount == 5);
    card = cardAt<ComplexEigenMethodTable>(model, 2);
    REQUIRE((card.methodName == ComplexMethod::Hess && card.desiredModeCount0 == 4 && card.regions.empty() &&
             !card.convergenceTolerance.has_value()));
    REQUIRE(cardAt<ComplexEigenMethodTable>(model, 3).methodName == ComplexMethod::Iram);
    REQUIRE(cardAt<ComplexEigenMethodTable>(model, 4).methodName == ComplexMethod::Det);

    requireRoundTrip(model);

    expectInvalid("ComplexEigenMethod;1;FOO", "methodName");
    expectInvalid("ComplexEigenMethod;1", "methodName");
    expectInvalid("ComplexEigenMethod;1;InverseIteration;Point", "normNode");
    expectInvalid("ComplexEigenMethod;1;InverseIteration;Maximum;5", "normNode");
    expectInvalid("ComplexEigenMethod;1;InverseIteration;;;;-1.", "convergenceTolerance");
    expectInvalid("ComplexEigenMethod;1;InverseIteration;;;;;-3", "desiredModeCount0");
    expectInvalid("ComplexEigenMethod;1;InverseIteration;;;;;;0.;0.;0.;0.;-1.", "regionWidth");
    expectInvalid("ComplexEigenMethod;1;ComplexLanczos;;;;;;0.;0.;10.;3.;0", "solveInterval");
    expectInvalid("ComplexEigenMethod;1;ComplexLanczos;;;;;;0.;0.;;;;5.;4", "shiftModeCount");
}

TEST_CASE("EIGP has one or two poles and goes with the EIGC of its ID; EIGC and EIGRL have spaces of their own",
          "[model][cards]")
{
    Model model = readBulk(
        "LanczosEigenMethod;1;;;5\n"
        "ComplexEigenMethod;1;DeterminantMethod;;;;;4\n"
        "ComplexEigenPoles;1;-3.75;0.;2\n"
        "ComplexEigenPoles;1;4.5;2.25;3;1.;1.;1\n"
        "BucklingEigenMethod;2;InverseIteration;0.;5.\n"
        "ComplexEigenMethod;2;HessenbergMethod;;;;;4");
    ComplexEigenPolesCard one = cardAt<ComplexEigenPolesTable>(model, 0);
    REQUIRE(
        (one.id == 1 && one.first.realPart == -3.75 && one.first.imaginaryPart == 0.0 && one.first.multiplicity == 2));
    REQUIRE_FALSE(one.second.has_value());
    const ComplexEigenPolesCard two = cardAt<ComplexEigenPolesTable>(model, 1);
    REQUIRE((two.second.has_value() && two.second->realPart == 1.0 && two.second->multiplicity == 1));
    // The real methods and the complex ones are selected by different commands: one ID can be both.
    REQUIRE(cardsOfSet(model, CardCategory::ComplexMethod, 1).size() == 3); // the EIGC and the two EIGP
    REQUIRE(cardsOfSet(model, CardCategory::ComplexMethod, 2).size() == 1);
    REQUIRE(resolveReference(model, ReferenceTarget::ComplexMethod, 1).has_value());
    REQUIRE(unresolvedIn(model) == 0);
    // One renumbering moves the EIGC and its poles together; the real methods stay.
    Renumbering::apply(model, RenumberSpace::ComplexMethod, Renumbering::offset(100));
    REQUIRE(cardAt<ComplexEigenMethodTable>(model, 0).id == 101);
    REQUIRE(cardAt<ComplexEigenPolesTable>(model, 0).id == 101);
    REQUIRE(cardAt<ComplexEigenPolesTable>(model, 1).id == 101);
    REQUIRE(cardAt<LanczosEigenMethodTable>(model, 0).id == 1);
    requireRoundTrip(model);

    expectInvalid("ComplexEigenPoles;1;-3.75;0.", "multiplicity1");
    expectInvalid("ComplexEigenPoles;1;-3.75;0.;0", "multiplicity1");
    expectInvalid("ComplexEigenPoles;1;-3.75;0.;2;4.5", "imaginaryPart2");
    expectInvalid("ComplexEigenPoles;1;-3.75;0.;2;4.5;2.25;0", "multiplicity2");
}

// ------------------------------------------------------------------------------------------------ time steps

TEST_CASE("TSTEP has one interval a line and TSTEPNL its defaults from its method", "[model][cards]")
{
    const Model model = readBulk(
        "TransientTimeSteps;1;12;0.002;4;8;0.02;;16;0.2;2\n"
        "NonlinearTimeSteps;1;100;0.02;;Adaptive\n"
        "NonlinearTimeSteps;2;50;0.1;;Iterative\n"
        "NonlinearTimeSteps;3;50;0.1;;PseudoFullNewtonTangent;-1\n"
        "NonlinearTimeSteps;4;50;0.1\n"
        "NonlinearTimeSteps;5;50;0.1;;Automatic;;-5");
    const TransientTimeStepsCard tstep = cardAt<TransientTimeStepsTable>(model);
    REQUIRE(tstep.intervals.size() == 3);
    REQUIRE((tstep.intervals[0].stepCount == 12 && tstep.intervals[0].timeStep == 0.002 &&
             tstep.intervals[0].outputInterval == 4));
    REQUIRE((tstep.intervals[1].stepCount == 8 && tstep.intervals[1].timeStep == 0.02 &&
             tstep.intervals[1].outputInterval == 1));
    REQUIRE((tstep.intervals[2].stepCount == 16 && tstep.intervals[2].timeStep == 0.2 &&
             tstep.intervals[2].outputInterval == 2));
    requireRoundTrip(model);

    // METHOD ADAPT: KSTEP 2 and MAXITER 10, stored resolved. The tolerances that depend on the
    // solution are not given.
    NonlinearTimeStepsCard card = cardAt<NonlinearTimeStepsTable>(model, 0);
    REQUIRE((card.id == 1 && card.stepCount == 100 && card.timeStep == 0.02 && card.outputInterval == 1));
    REQUIRE(
        (card.method == NonlinearTransientMethod::Adapt && card.stiffnessUpdateStep == 2 && card.maxIterations == 10));
    REQUIRE((card.epsilonU == 0.01 && !card.epsilonP.has_value() && !card.epsilonW.has_value()));
    REQUIRE((card.maxDivergences == 2 && card.maxQuasiNewtonVectors == 10 && card.maxLineSearches == 2 &&
             card.stressSubincrementFraction == 0.2 && card.maxBisections == 5));
    REQUIRE((card.adjustInterval == 5 && !card.minStepsPerCycle.has_value() && card.stepBounds == 0.6 &&
             card.maxRotationRatio == 32.0 && card.displacementTolerance == 0.1));
    REQUIRE((card.rotationTolerance == 20.0 && !card.minIterations.has_value() && card.convergenceFlags.empty()));
    card = cardAt<NonlinearTimeStepsTable>(model, 1);
    REQUIRE((card.stiffnessUpdateStep == 10 && card.maxIterations == 25));
    card = cardAt<NonlinearTimeStepsTable>(model, 2);
    REQUIRE(
        (card.method == NonlinearTransientMethod::Pfnt && card.stiffnessUpdateStep == -1 && card.maxIterations == 25));
    REQUIRE((card.epsilonU == -0.01 && card.maxQuasiNewtonVectors == 0 &&
             card.maxLineSearches == 0)); // the defaults of PFNT
    card = cardAt<NonlinearTimeStepsTable>(model, 3);
    REQUIRE(card.method ==
            NonlinearTransientMethod::NotGiven); // the default depends on the solution: blank stays blank
    REQUIRE((!card.stiffnessUpdateStep.has_value() && !card.maxIterations.has_value() && card.maxBisections == 5));
    card = cardAt<NonlinearTimeStepsTable>(model, 4);
    REQUIRE((card.maxIterations == -5 &&
             !card.maxBisections.has_value())); // a negative MAXITER: the default of MAXBIS is the solution's

    const Model full = readBulk(
        "NonlinearTimeSteps;7;240;0.015;2;Adaptive;3;12;PW;0.01;0.002;2.E-6;4;8;3;0.3;6;4;15;0.5;20.;0.2;25.;2");
    card = cardAt<NonlinearTimeStepsTable>(full);
    REQUIRE((card.stiffnessUpdateStep == 3 && card.maxIterations == 12 && card.convergenceFlags == "PW" &&
             card.epsilonP == 2.0E-3 && card.epsilonW == 2.0E-6));
    REQUIRE((card.maxDivergences == 4 && card.maxQuasiNewtonVectors == 8 && card.maxLineSearches == 3 &&
             card.stressSubincrementFraction == 0.3 && card.maxBisections == 6));
    REQUIRE((card.adjustInterval == 4 && card.minStepsPerCycle == 15 && card.stepBounds == 0.5 &&
             card.maxRotationRatio == 20.0 && card.displacementTolerance == 0.2));
    REQUIRE((card.rotationTolerance == 25.0 && card.minIterations == 2));
    requireRoundTrip(full);

    expectInvalid("TransientTimeSteps;1", "stepCounts");
    expectInvalid("TransientTimeSteps;1;0;0.1", "stepCounts");
    expectInvalid("TransientTimeSteps;1;10;0.", "timeSteps");
    expectInvalid("TransientTimeSteps;1;10;0.1;0", "outputIntervals");
    expectInvalid("NonlinearTimeSteps;1;2;0.1", "stepCount");
    expectInvalid("NonlinearTimeSteps;1;10;0.", "timeStep");
    expectInvalid("NonlinearTimeSteps;1;10;0.1;0", "outputInterval");
    expectInvalid("NonlinearTimeSteps;1;10;0.1;;FOO", "method");
    expectInvalid("NonlinearTimeSteps;1;10;0.1;;FullNewtonTangent;2", "stiffnessUpdateStep");
    expectInvalid("NonlinearTimeSteps;1;10;0.1;;;;0", "maxIterations");
    expectInvalid("NonlinearTimeSteps;1;10;0.1;;;;;XYZ", "convergenceFlags");
    expectInvalid("NonlinearTimeSteps;1;10;0.1;;;;;;;;;;;;1.5", "stressSubincrementFraction");
    expectInvalid("NonlinearTimeSteps;1;10;0.1;;;;;;;;;;;;;9", "maxBisections");
    expectInvalid("NonlinearTimeSteps;1;10;0.1;;;;;;;;;;;;;;;;1.5", "stepBounds");
}

TEST_CASE("NLPARM has its fields on three lines, blank tolerances, defaults from KMETHOD and MAXITER, and NLPCI",
          "[model][cards]")
{
    const Model model = readBulk(
        "NonlinearStaticParams;1\n"
        "NonlinearStaticParams;2;10;;PseudoFullNewtonTangent;;40\n"
        "NonlinearStaticParams;3;1;;;;-10\n"
        "NonlinearStaticParams;4;5;0.1;Iterative;3;40;UPW;Yes;0.001;0.002;0.003;4;5;6;0.3;0.4;7;25.;30.;2\n"
        "NonlinearStaticParams;5;1;;;;;;5\n"
        "ArcLengthParams;1\n"
        "ArcLengthParams;4;Riks;0.5;3.;2.;10;15");
    NonlinearStaticParamsCard card = cardAt<NonlinearStaticParamsTable>(model, 0);
    REQUIRE((card.id == 1 && !card.incrementCount.has_value() && card.timeStep == 0.0 &&
             card.stiffnessMethod == StiffnessMethod::NotGiven));
    REQUIRE((!card.stiffnessUpdateStep.has_value() && card.maxIterations == 25 && card.convergenceFlags.empty() &&
             card.intermediateOutput == "No"));
    // blank tolerances stay blank: PARAM,NLTOL applies, which a given 1.0E-2 would override
    REQUIRE((!card.epsilonU.has_value() && !card.epsilonP.has_value() && !card.epsilonW.has_value() &&
             card.maxDivergences == 3));
    REQUIRE((card.maxQuasiNewtonVectors == 25 && card.maxLineSearches == 4 && card.stressSubincrementFraction == 0.2 &&
             card.lineSearchTolerance == 0.5 && card.maxBisections == 5));
    REQUIRE((card.maxRotationRatio == 20.0 && card.rotationTolerance == 20.0 && !card.minIterations.has_value()));
    card = cardAt<NonlinearStaticParamsTable>(model, 1); // PFNT: MAXQN and MAXLS 0
    REQUIRE((card.incrementCount == 10 && card.stiffnessMethod == StiffnessMethod::Pfnt && card.maxIterations == 40));
    REQUIRE((!card.epsilonU.has_value() && !card.epsilonW.has_value() && card.maxQuasiNewtonVectors == 0 &&
             card.maxLineSearches == 0));
    card =
        cardAt<NonlinearStaticParamsTable>(model, 2); // a negative MAXITER: no bisection by default, MAXQN is |MAXITER|
    REQUIRE((card.maxIterations == -10 && card.maxBisections == 0 && card.maxQuasiNewtonVectors == 10));
    card = cardAt<NonlinearStaticParamsTable>(model, 3);
    REQUIRE((card.incrementCount == 5 && card.timeStep == 0.1 && card.stiffnessMethod == StiffnessMethod::Iter &&
             card.stiffnessUpdateStep == 3));
    REQUIRE((card.maxIterations == 40 && card.convergenceFlags == "UPW" && card.intermediateOutput == "Yes" &&
             card.epsilonU == 1.0E-3));
    REQUIRE((card.epsilonP == 2.0E-3 && card.epsilonW == 3.0E-3 && card.maxDivergences == 4 &&
             card.maxQuasiNewtonVectors == 5 && card.maxLineSearches == 6));
    REQUIRE((card.stressSubincrementFraction == 0.3 && card.lineSearchTolerance == 0.4 && card.maxBisections == 7 &&
             card.maxRotationRatio == 25.0));
    REQUIRE((card.rotationTolerance == 30.0 && card.minIterations == 2));            // fields 17, 21, 23 and 24
    REQUIRE(cardAt<NonlinearStaticParamsTable>(model, 4).intermediateOutput == "5"); // an integer, SOL 400
    const ArcLengthParamsCard defaults = cardAt<ArcLengthParamsTable>(model, 0);
    REQUIRE((defaults.type == ArcLengthType::Cris && defaults.minArcLengthRatio == 0.25 &&
             defaults.maxArcLengthRatio == 4.0));
    REQUIRE((defaults.scaleFactor == 0.0 && defaults.designIteration == 12 && defaults.maxIncrements == 20));
    const ArcLengthParamsCard riks = cardAt<ArcLengthParamsTable>(model, 1);
    REQUIRE((riks.type == ArcLengthType::Riks && riks.minArcLengthRatio == 0.5 && riks.maxArcLengthRatio == 3.0 &&
             riks.scaleFactor == 2.0));
    REQUIRE((riks.designIteration == 10 && riks.maxIncrements == 15));
    // The NLPARM and the NLPCI of an ID are one selection: they renumber together.
    Model renumbered = model.copy();
    Renumbering::apply(renumbered, RenumberSpace::Nonlinear, Renumbering::offset(100));
    REQUIRE(cardAt<NonlinearStaticParamsTable>(renumbered, 3).id == 104);
    REQUIRE(cardAt<ArcLengthParamsTable>(renumbered, 1).id == 104);
    REQUIRE(cardsOfSet(renumbered, CardCategory::NonlinearStatic, 104).size() == 2);
    requireRoundTrip(model);

    expectInvalid("NonlinearStaticParams;1;-1", "incrementCount");
    expectInvalid("NonlinearStaticParams;1;;-1.", "timeStep");
    expectInvalid("NonlinearStaticParams;1;;;XXX", "stiffnessMethod");
    expectInvalid("NonlinearStaticParams;1;;;;-2", "stiffnessUpdateStep");
    expectInvalid("NonlinearStaticParams;1;;;;;0", "maxIterations");
    expectInvalid("NonlinearStaticParams;1;;;;;;XYZ", "convergenceFlags");
    expectInvalid("NonlinearStaticParams;1;;;;;;;MAYBE", "intermediateOutput");
    expectInvalid("NonlinearStaticParams;1;;;;;;;0", "intermediateOutput");
    expectInvalid("NonlinearStaticParams;1;;;;;;;;;-1.", "epsilonP");
    expectInvalid("NonlinearStaticParams;1;;;;;;;;;;;;;;;1.", "lineSearchTolerance");
    expectInvalid("NonlinearStaticParams;1;;;;;;;;;;;;;;;;;50.", "maxRotationRatio");
    expectInvalid("ArcLengthParams;1;XX", "constraintKind");
    expectInvalid("ArcLengthParams;1;;1.5", "minArcLengthRatio");
    expectInvalid("ArcLengthParams;1;;;0.5", "maxArcLengthRatio");
    expectInvalid("ArcLengthParams;1;;;;;0", "designIteration");
    expectInvalid("ArcLengthParams;1;;;;;;0", "maxIncrements");
}

// ------------------------------------------------------------------------------------------------ TIC, TF, DELAY,
// DPHASE

TEST_CASE("TIC and TF refer to points, a modal coordinate is no point", "[model][cards]")
{
    Model model = readBulk(
        "Node;10;;0.;0.;0.\n"
        "ScalarPoint;12\n"
        "Node;2;;0.;0.;0.\n"
        "Node;3;;0.;0.;0.\n"
        "Node;7;;0.;0.;0.\n"
        "TransientInitialCondition;1;10;3;0.1;0.5\n"
        "TransientInitialCondition;1;12;0;0.2;0.3\n"
        "TransientInitialCondition;2;5;-1;1.5;0.\n"
        "TransferFunction;1;2;1;3.5;4.5;5.5;3;2;2.5;1.5;0.5;7;;1.\n"
        "TransferFunction;1;3;0;1.");
    REQUIRE(cardAt<TransientInitialConditionTable>(model, 0).components == 3);
    const TransientInitialConditionCard modal = cardAt<TransientInitialConditionTable>(model, 2);
    REQUIRE((modal.node == 5 && modal.components == TransientInitialConditionCard::MODAL &&
             modal.initialDisplacement == 1.5 && modal.initialVelocity == 0.0));
    const TransferFunctionCard tf = cardAt<TransferFunctionTable>(model, 0);
    REQUIRE((tf.nodeD == 2 && tf.componentsD == 1 && tf.coefficientB0 == 3.5 && tf.coefficientB1 == 4.5 &&
             tf.coefficientB2 == 5.5 && tf.terms.size() == 2));
    REQUIRE((tf.terms[0].node == 3 && tf.terms[0].components == 2 && tf.terms[0].coefficientA0 == 2.5 &&
             tf.terms[0].coefficientA1 == 1.5));
    REQUIRE((tf.terms[0].coefficientA2 == 0.5 && tf.terms[1].node == 7 && tf.terms[1].components == 0 &&
             tf.terms[1].coefficientA0 == 1.0));
    REQUIRE(cardAt<TransferFunctionTable>(model, 1).terms.empty());
    REQUIRE(cardsOfSet(model, CardCategory::TransferFunction, 1).size() == 2);
    REQUIRE(cardsOfSet(model, CardCategory::InitialCondition, 1).size() == 2);
    REQUIRE(unresolvedIn(model) == 0); // the modal coordinate 5 is no GRID, and is not looked for
    RecordingVisitor visitor;
    model.table<TransientInitialConditionTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields.empty());
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<TransientInitialConditionTable>(model, 0).node == 110);
    REQUIRE(cardAt<TransientInitialConditionTable>(model, 1).node == 112); // the scalar point follows too
    REQUIRE(cardAt<TransientInitialConditionTable>(model, 2).node == 5);
    REQUIRE(cardAt<TransferFunctionTable>(model, 0).nodeD == 102);
    REQUIRE((cardAt<TransferFunctionTable>(model, 0).terms[0].node == 103 &&
             cardAt<TransferFunctionTable>(model, 0).terms[1].node == 107));
    Renumbering::apply(model, RenumberSpace::InitialCondition, Renumbering::offset(30));
    Renumbering::apply(model, RenumberSpace::TransferFunction, Renumbering::offset(40));
    REQUIRE(cardAt<TransientInitialConditionTable>(model, 0).id == 31);
    REQUIRE(cardAt<TransferFunctionTable>(model, 1).id == 41);
    requireRoundTrip(model);

    expectInvalid("TransientInitialCondition;1;0;1", "node");
    expectInvalid("TransientInitialCondition;1;5;7", "components");
    expectInvalid("TransientInitialCondition;1;5;-2", "components");
    expectInvalid("TransientInitialCondition;1", "node");
    expectInvalid("TransferFunction;1;0;3", "nodeD");
    expectInvalid("TransferFunction;1;2;9", "componentsD");
    expectInvalid("TransferFunction;1;2;1;3.5;4.5;5.5;0;4;5.", "termNodes");
    expectInvalid("TransferFunction;1;2;1;3.5;4.5;5.5;3;8;5.", "termComponents");
}

TEST_CASE("DELAY and DPHASE are sets that the dynamic loads name", "[model][cards]")
{
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "Node;2;;0.;0.;0.\n"
        "DynamicLoadScale;9;1;1;1.\n"
        "TableDynamic1;3;;;0.;1.;1.;1.;EndTable\n"
        "DynamicLoadDelay;5;1;1;0.5\n"
        "DynamicLoadDelay;5;2;;-1.5;1;3;0.25\n"
        "DynamicLoadPhase;6;1;1;30.\n"
        "FreqResponseLoad1;10;9;5;;6;;3\n"
        "TransientTableLoad1;11;9;5;;;3");
    const DynamicLoadDelayCard second = cardAt<DynamicLoadDelayTable>(model, 1);
    REQUIRE((second.points[0] == 2 && second.components[0] == 0 && second.delays[0] == -1.5));
    REQUIRE(
        (second.hasSecondGroup() && second.points[1] == 1 && second.components[1] == 3 && second.delays[1] == 0.25));
    REQUIRE_FALSE(cardAt<DynamicLoadDelayTable>(model, 0).hasSecondGroup());
    REQUIRE(cardAt<DynamicLoadPhaseTable>(model).phaseAngles[0] == 30.0);
    REQUIRE(cardsOfSet(model, CardCategory::Delay, 5).size() == 2);
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor visitor;
    model.table<FreqResponseLoad1Table>().visitReferences(0, visitor);
    REQUIRE(visitor.targets[1] == ReferenceTarget::DelaySet);
    REQUIRE(visitor.targets[2] == ReferenceTarget::PhaseSet);
    Renumbering::apply(model, RenumberSpace::Delay, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::Phase, Renumbering::offset(200));
    REQUIRE(cardAt<DynamicLoadDelayTable>(model).id == 105);
    REQUIRE(cardAt<DynamicLoadPhaseTable>(model).id == 206);
    REQUIRE(cardAt<FreqResponseLoad1Table>(model).delayId == 105); // the loads follow
    REQUIRE(cardAt<FreqResponseLoad1Table>(model).phaseId == 206);
    REQUIRE(cardAt<TransientTableLoad1Table>(model).delayId == 105);
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE((cardAt<DynamicLoadDelayTable>(model, 1).points[0] == 12 &&
             cardAt<DynamicLoadDelayTable>(model, 1).points[1] == 11));
    model.remove(*cardsOfSet(model, CardCategory::Delay, 105).begin());
    model.remove(*cardsOfSet(model, CardCategory::Delay, 105).begin());
    REQUIRE(unresolvedIn(model) == 2); // the RLOAD1 and the TLOAD1 name a DELAY set that is gone
    requireRoundTrip(
        readBulk("DynamicLoadDelay;5;1;1;0.5\nDynamicLoadDelay;6;2;;-1.5;1;3;0.25\nDynamicLoadPhase;7;1;1;30.;2;;4."));

    expectInvalid("DynamicLoadDelay;5;0;1;0.5", "point1");
    expectInvalid("DynamicLoadDelay;5;1;7;0.5", "components1");
    expectInvalid("DynamicLoadDelay;5;1;1", "delay1");
    expectInvalid("DynamicLoadDelay;5;1;1;0.5;2;7;0.5", "components2");
    expectInvalid("DynamicLoadDelay;5;1;1;0.5;2;1", "delay2");
    expectInvalid("DynamicLoadPhase;5;1;1", "phaseAngle1");
    expectInvalid("DynamicLoadPhase;5;1;1;30.;;;1.", "point2");
}

// ------------------------------------------------------------------------------------------------ tables

TEST_CASE("TABDMP1, TABRND1 and TABRNDG have ID spaces of their own and TABDMP1 and TABRND1 evaluate", "[model][cards]")
{
    // A TABLED1, a TABDMP1 and a TABRND1 of the same ID (and a TABLEM1: decks do), but one random table space.
    const Model model = readBulk(
        "TableDynamic1;1;;;0.;0.;1.;1.;EndTable\n"
        "TableModalDamping1;1;;0.;0.01;100.;0.02;EndTable\n"
        "TableRandomSpectrum1;1;Logarithmic;Logarithmic;20.;0.01;50.;0.015;EndTable\n"
        "TableGustSpectrum;2;1;0.2;6.\n"
        "TableModalDamping1;2;CriticalDamping;0.;0.02;EndTable");
    REQUIRE(model.findTable<TableModalDamping1Table>()->INFO.idSpace == base::IdSpace::HybridDampingRotorOptionsTable);
    REQUIRE(model.findTable<TableRandomSpectrum1Table>()->INFO.idSpace == base::IdSpace::RandomTable);
    REQUIRE(model.locate(base::IdSpace::HybridDampingRotorOptionsTable, 1).has_value());
    REQUIRE(model.locate(base::IdSpace::RandomTable, 2).has_value());
    REQUIRE_THROWS_AS(readBulk("TableRandomSpectrum1;1;;;1.;1.;2.;2.;EndTable\nTableGustSpectrum;1;1;0.2;6."),
                      base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TableModalDamping1;1;;1.;1.;EndTable\nTableModalDamping1;1;;1.;1.;EndTable"),
                      base::DeckError);

    const TableModalDamping1Card damping = cardAt<TableModalDamping1Table>(model, 0);
    REQUIRE((damping.kind == DampingType::G && damping.x == std::vector<double>{0.0, 100.0}));
    REQUIRE(cardAt<TableModalDamping1Table>(model, 1).kind == DampingType::Crit);
    const TableModalDamping1Table& dampingTable = *model.findTable<TableModalDamping1Table>();
    REQUIRE(dampingTable.evaluateRow(0, 50.0) == Approx(0.015));
    REQUIRE(dampingTable.evaluateRow(0, 0.0) == Approx(0.01));
    REQUIRE(dampingTable.evaluateRow(0, 200.0) == Approx(0.03)); // the last segment, extended
    REQUIRE(TableModalDamping1Table::evaluate(damping, 50.0) == Approx(0.015));
    std::vector<double> in = {0.0, 50.0, 100.0};
    std::vector<double> out(3);
    dampingTable.evaluateRow(0, in, out);
    REQUIRE((out[0] == Approx(0.01) && out[1] == Approx(0.015) && out[2] == Approx(0.02)));
    const TableRandomSpectrum1Table& psd = *model.findTable<TableRandomSpectrum1Table>();
    const TableRandomSpectrum1Card psdCard = psd.card(0);
    REQUIRE((psdCard.xAxisKind == TableAxis::Log && psdCard.yAxisKind == TableAxis::Log));
    REQUIRE(psd.evaluateRow(0, 20.0) == Approx(0.01));
    REQUIRE(psd.evaluateRow(0, std::sqrt(20.0 * 50.0)) == Approx(std::sqrt(0.01 * 0.015))); // straight in log-log
    REQUIRE(TableRandomSpectrum1Table::evaluate(psdCard, 50.0) == Approx(0.015));
    REQUIRE(std::isnan(psd.evaluateRow(0, -1.0))); // no logarithm
    const TableGustSpectrumCard gust = cardAt<TableGustSpectrumTable>(model);
    REQUIRE((gust.id == 2 && gust.kind == 1 && gust.turbulenceScaleRatio == 0.2 && gust.gustVelocity == 6.0));
    requireRoundTrip(model);

    expectInvalid("TableModalDamping1;1;FOO;0.;0.01;EndTable", "kind");
    expectInvalid("TableModalDamping1;1;;0.;0.01;100.;0.02", "tableEnd");
    expectInvalid("TableModalDamping1;1;;-1.;0.01;EndTable", "frequency");
    expectInvalid("TableModalDamping1;1;;0.;0.01;5.;0.02;3.;0.01;EndTable", "x");
    expectInvalid("TableRandomSpectrum1;1;;;1.;1.;EndTable", "x");
    expectInvalid("TableRandomSpectrum1;1;FOO;;1.;1.;2.;2.;EndTable", "xAxisKind");
    expectInvalid("TableRandomSpectrum1;1;Logarithmic;;0.;1.;2.;2.;EndTable", "x");
    expectInvalid("TableGustSpectrum;1;3;0.2;6.", "kind");
    expectInvalid("TableGustSpectrum;1;1;0.2", "gustVelocity");
}

TEST_CASE("TABLED5 is a dynamic table of tables, evaluated by looking them up", "[model][cards]")
{
    Model model = readBulk(
        "TableDynamic1;101;;;0.;0.;1.;1.;EndTable\n"
        "TableDynamic1;102;;;0.;0.;1.;2.;EndTable\n"
        "TableDynamic5;5;0.;101;10.;102;EndTable");
    REQUIRE(model.findTable<TableDynamic5Table>()->INFO.idSpace == base::IdSpace::DynamicTable);
    REQUIRE(model.locate(base::IdSpace::DynamicTable, 5).has_value());
    const TableDynamic5Table& table = *model.findTable<TableDynamic5Table>();
    const TableDynamic5Card card = table.card(0);
    REQUIRE((card.id == 5 && card.x == std::vector<double>{0.0, 10.0} &&
             card.tables == std::vector<base::EntityId>{101, 102}));
    // f(x, y): the tables 101 and 102 give 0.5 and 1.0 at y = 0.5; between x = 0 and 10 the value is linear in x.
    REQUIRE(table.evaluateRow(model, 0, 0.0, 0.5) == Approx(0.5));
    REQUIRE(table.evaluateRow(model, 0, 10.0, 0.5) == Approx(1.0));
    REQUIRE(table.evaluateRow(model, 0, 5.0, 0.5) == Approx(0.75));
    REQUIRE(table.evaluateRow(model, 0, 20.0, 0.5) == Approx(1.5)); // the last segment, extended
    REQUIRE(TableDynamic5Table::evaluate(model, card, 5.0, 0.25) == Approx(0.375));
    REQUIRE(std::isnan(table.evaluateRow(model, 0, std::nan(""), 0.5)));
    REQUIRE(unresolvedIn(model) == 0);
    // The tables of the pairs are dynamic table references: they renumber with the dynamic tables.
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(1000));
    const TableDynamic5Card moved = cardAt<TableDynamic5Table>(model);
    REQUIRE((moved.id == 1005 && moved.tables == std::vector<base::EntityId>{1101, 1102}));
    REQUIRE(model.findTable<TableDynamic5Table>()->evaluateRow(model, 0, 5.0, 0.5) == Approx(0.75));
    requireRoundTrip(model);
    // A table that is missing, or that is not a TABLEDi, gives NaN where it counts.
    const Model missing = readBulk("TableDynamic1;101;;;0.;0.;1.;1.;EndTable\nTableDynamic5;5;0.;101;10.;102;EndTable");
    REQUIRE(std::isnan(missing.findTable<TableDynamic5Table>()->evaluateRow(missing, 0, 5.0, 0.5)));
    REQUIRE(unresolvedIn(const_cast<Model&>(missing)) == 1);

    expectInvalid("TableDynamic5;5;10.;101;0.;102;EndTable", "x");
    expectInvalid("TableDynamic5;5;0.;101;10.;102", "tableEnd");
    expectInvalid("TableDynamic5;5;0.;0;EndTable", "tables");
    expectInvalid("TableDynamic5;0;0.;101;EndTable", "id");
    expectInvalid("TableDynamic5;5;EndTable", "x");
}

// ------------------------------------------------------------------------------------------------ random analysis

TEST_CASE("RANDPS and RANDT1 share a set that RANDOM selects, and RANDPS names a random table", "[model][cards]")
{
    Model model = readBulk(
        "RandomSpectralDensity;1;1;1;4.;;4\n"
        "RandomSpectralDensity;1;1;2;2.;2.5\n"
        "RandomSpectralDensity;1;1;3\n"
        "RandomTimeLags;1;4;0.;0.4\n"
        "TableRandomSpectrum1;4;;;1.;1.;2.;2.;EndTable");
    const RandomSpectralDensityCard auto_ = cardAt<RandomSpectralDensityTable>(model, 0);
    REQUIRE((auto_.id == 1 && auto_.subcaseJ == 1 && auto_.subcaseK == 1 && auto_.realPart == 4.0 &&
             auto_.imaginaryPart == 0.0 && auto_.tableId == 4));
    const RandomSpectralDensityCard cross = cardAt<RandomSpectralDensityTable>(model, 1);
    REQUIRE((cross.subcaseK == 2 && cross.realPart == 2.0 && cross.imaginaryPart == 2.5 && cross.tableId == 0));
    REQUIRE((cardAt<RandomSpectralDensityTable>(model, 2).realPart == 0.0 &&
             cardAt<RandomSpectralDensityTable>(model, 2).imaginaryPart == 0.0));
    const RandomTimeLagsCard lags = cardAt<RandomTimeLagsTable>(model);
    REQUIRE((lags.count == 4 && lags.startTime == 0.0 && lags.maximumTime == 0.4));
    REQUIRE(cardsOfSet(model, CardCategory::Random, 1).size() == 4);
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::RandomTable, Renumbering::offset(10));
    REQUIRE(cardAt<RandomSpectralDensityTable>(model, 0).tableId == 14);
    REQUIRE(cardAt<TableRandomSpectrum1Table>(model).id == 14);
    Renumbering::apply(model, RenumberSpace::Random, Renumbering::offset(50));
    REQUIRE(cardAt<RandomSpectralDensityTable>(model, 0).id == 51);
    REQUIRE(cardAt<RandomTimeLagsTable>(model).id == 51);
    requireRoundTrip(model);

    expectInvalid("RandomSpectralDensity;1;1;1;0.", "realPart");
    expectInvalid("RandomSpectralDensity;1;1;1;1.;1.", "realPart");
    expectInvalid("RandomSpectralDensity;1;2;1;1.", "subcaseK");
    expectInvalid("RandomSpectralDensity;1;0;1;1.", "subcaseJ");
    expectInvalid("RandomSpectralDensity;1;1;2;1.;1.;-1", "tableId");
    expectInvalid("RandomTimeLags;1;0;0.;0.4", "count");
    expectInvalid("RandomTimeLags;1;4;0.4;0.4", "maximumTime");
    expectInvalid("RandomTimeLags;1;4;-1.;0.4", "startTime");
    expectInvalid("RandomTimeLags;1;4;0.", "maximumTime");
}

// ------------------------------------------------------------------------------------------------ the other cards

TEST_CASE("MODTRAK, MDLPRM and RGYRO have their fields, defaults and rules", "[model][cards]")
{
    const Model model = readBulk(
        "ModeTracking;1;3;24;0.85\n"
        "ModeTracking;2\n"
        "ModelParams;HDF5;1;OffsetDefinition;LoadRecoveryOffsets;PRCNT;0.5\n"
        "ModelParams;ab;2\n"
        "RotordynamicParams;1;SynchronousWhirl;1;RevolutionsPerMinute;1200.;4800.;;10;1.5;2.5;3.5\n"
        "RotordynamicParams;1;;2;Frequency;;;40.");
    const ModeTrackingCard modtrak = cardAt<ModeTrackingTable>(model, 0);
    REQUIRE((modtrak.id == 1 && modtrak.lowRange == 3 && modtrak.highRange == 24 && modtrak.trackingFilter == 0.85));
    const ModeTrackingCard blank = cardAt<ModeTrackingTable>(model, 1);
    REQUIRE((blank.lowRange == 0 && blank.highRange == 0 && blank.trackingFilter == 0.9));
    const ModelParamsCard parameters = cardAt<ModelParamsTable>(model, 0);
    REQUIRE(parameters.parameters.size() == 3);
    REQUIRE((parameters.parameters[0].name == "HDF5" && parameters.parameters[0].kind == ModelParameterKind::Integer &&
             parameters.parameters[0].integerValue == 1));
    // OFFDEF has its neutral name; its value is a deck word and stays as written.
    REQUIRE((parameters.parameters[1].name == "OffsetDefinition" &&
             parameters.parameters[1].kind == ModelParameterKind::Text &&
             parameters.parameters[1].textValue == "LoadRecoveryOffsets"));
    REQUIRE((parameters.parameters[2].kind == ModelParameterKind::Real && parameters.parameters[2].realValue == 0.5));
    // An arbitrary parameter name keeps its exact spelling.
    REQUIRE(cardAt<ModelParamsTable>(model, 1).parameters[0].name == "ab");
    const RotordynamicParamsCard synchronous = cardAt<RotordynamicParamsTable>(model, 0);
    REQUIRE((synchronous.id == 1 && synchronous.synchronous == RotorSynchronisation::Sync &&
             synchronous.referenceRotorId == 1));
    REQUIRE((synchronous.speedUnit == RotorSpeedUnit::Rpm && synchronous.speedLow == 1200.0 &&
             synchronous.speedHigh == 4800.0));
    REQUIRE((synchronous.speed == 0.0 && synchronous.rotorSuperelementId == 10 &&
             synchronous.viscousDampingFrequency == 1.5));
    REQUIRE((synchronous.structuralDampingFrequency == 2.5 && synchronous.hybridDampingFrequency == 3.5));
    const RotordynamicParamsCard statics = cardAt<RotordynamicParamsTable>(model, 1);
    REQUIRE((statics.synchronous == RotorSynchronisation::NotGiven && statics.speedUnit == RotorSpeedUnit::Freq));
    REQUIRE((statics.speedLow == 0.0 && statics.speedHigh == 99999.0 && statics.speed == 40.0 &&
             statics.rotorSuperelementId == 0));
    REQUIRE(cardsOfSet(model, CardCategory::Other, 1).size() >= 2); // several RGYRO cards share a RID
    requireRoundTrip(model);

    expectInvalid("ModeTracking;1;5;3", "lowRange");
    expectInvalid("ModeTracking;1;-1", "lowRange");
    expectInvalid("ModeTracking;1;;-4", "highRange");
    expectInvalid("ModeTracking;0", "id");
    expectInvalid("ModelParams;HDF5;1;HDF5;0", "parameterNames");
    expectInvalid("ModelParams;1X;1", "parameterNames");
    expectInvalid("ModelParams;HDF5", "parameterValues");
    expectInvalid("ModelParams;;1", "parameterNames");
    expectInvalid("ModelParams", "parameterNames");
    expectInvalid("RotordynamicParams;1;SynchronousWhirl;1", "speedUnit");
    expectInvalid("RotordynamicParams;1;FOO;1;RevolutionsPerMinute", "synchronous");
    expectInvalid("RotordynamicParams;1;;0;RevolutionsPerMinute", "referenceRotorId");
    expectInvalid("RotordynamicParams;1;;1;XX", "speedUnit");
    expectInvalid("RotordynamicParams;1;;1;RevolutionsPerMinute;;;;-1", "rotorSuperelementId");
}

TEST_CASE("ROTORG keeps its runs and never splits one between two lines", "[model][cards]")
{
    const Model model = readBulk(
        "RotorLineNodes;1;4101;4202;4103;4304\n"
        "RotorLineNodes;2;1001;Through;1100;By;2\n"
        "RotorLineNodes;3;1001;Through;1011\n"
        "RotorLineNodes;4;1;2;3;4;5;6;7;8;Through;20;30");
    REQUIRE(cardAt<RotorLineNodesTable>(model, 0).runs.size() == 4);
    const RotorLineNodesCard strided = cardAt<RotorLineNodesTable>(model, 1);
    REQUIRE((strided.runs.size() == 1 && strided.runs[0].first == 1001 && strided.runs[0].last == 1100));
    REQUIRE(strided.runs[0].increment == 2);
    REQUIRE(cardAt<RotorLineNodesTable>(model, 2).runs[0].increment == 1);
    const RotorLineNodesCard lines = cardAt<RotorLineNodesTable>(model, 3);
    REQUIRE(
        (lines.runs.size() == 9 && lines.runs[7].first == 8 && lines.runs[7].last == 20 && lines.runs[8].first == 30));
    requireRoundTrip(model);
    // A run that does not fit in what is left of a line goes to the next one: THRU is never the first word of a line.
    const Model packed = readBulk("RotorLineNodes;9;1;2;3;4;5;6;7;8;9;10;Through;20");
    const std::string text = writeModelText(packed);
    INFO(text);
    REQUIRE(cardAt<RotorLineNodesTable>(packed).runs.size() == 10);
    requireRoundTrip(packed);

    expectInvalid("RotorLineNodes;0;1", "id");
    expectInvalid("RotorLineNodes;1", "nodeIds");
    expectInvalid("RotorLineNodes;1;10;Through;5", "nodeIds");
    expectInvalid("RotorLineNodes;1;10;Through;20;By;0", "increment");
    expectInvalid("RotorLineNodes;1;0;2", "nodeIds");
}

TEST_CASE("RCROSS takes elements or points by the kind of response, RVDOF and RVDOF1 are DOF lists", "[model][cards]")
{
    Model model = readBulk(
        "Node;100;;0.;0.;0.\n"
        "Node;101;;0.;0.;0.\n"
        "Node;102;;0.;0.;0.\n"
        "Rod2;200;1;100;101\n"
        "CrossSpectrumPair;1;Displacement;100;3;Stress;200;10;2\n"
        "CrossSpectrumPair;2;Displacement;100;3;;102;4\n"
        "CrossSpectrumPair;3;Pressure;6000;1;Pressure;6000;1\n"
        "ResidualVectorDofs;100;1;102;2\n"
        "ResidualVectorDofs1;3;100;101\n"
        "ResidualVectorDofs1;12;100;Through;102");
    const CrossSpectrumPairCard first = cardAt<CrossSpectrumPairTable>(model, 0);
    REQUIRE((first.id == 1 && first.responseKinds[0] == "Displacement" && first.targetIds[0] == 100 &&
             first.components[0] == 3));
    REQUIRE((first.responseKinds[1] == "Stress" && first.targetIds[1] == 200 && first.components[1] == 10 &&
             first.curveId == 2));
    const CrossSpectrumPairCard second = cardAt<CrossSpectrumPairTable>(model, 1);
    REQUIRE((second.responseKinds[0] == "Displacement" &&
             second.responseKinds[1] == "Displacement")); // a blank RTYPE is the other one
    REQUIRE(unresolvedIn(model) == 1); // the CROD 200 has no property: only RCROSS 3 names a point that need not exist
    RecordingVisitor visitor;
    model.table<CrossSpectrumPairTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"targetId1", "targetId2"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ScalarOrGridPoint, ReferenceTarget::Element});
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(5));
    const CrossSpectrumPairCard moved = cardAt<CrossSpectrumPairTable>(model, 0);
    REQUIRE((moved.targetIds[0] == 105 && moved.targetIds[1] == 1200));
    REQUIRE(cardAt<ResidualVectorDofsTable>(model).dofs[1].node == 107);
    const ResidualVectorDofs1Card runs = cardAt<ResidualVectorDofs1Table>(model, 1);
    REQUIRE(
        (runs.components == 12 && runs.nodes.size() == 1 && runs.nodes[0].first == 105 && runs.nodes[0].last == 107));
    requireRoundTrip(model);

    expectInvalid("CrossSpectrumPair;1;Displacement;100;3", "targetId2");
    expectInvalid("CrossSpectrumPair;1", "responseKind1");
    expectInvalid("CrossSpectrumPair;1;Displacement;0;3;Displacement;1;1", "targetId1");
    expectInvalid("CrossSpectrumPair;1;Displacement;1;0;Displacement;1;1", "component1");
    expectInvalid("CrossSpectrumPair;1;Displacement;1;1;Displacement;1;1;-1", "curveId");
    requireRoundTrip(readBulk("CrossSpectrumPair;1;TOOLONGWORD;1;1;Displacement;1;1"));
    expectInvalid("ResidualVectorDofs;100;7", "components1");
    expectInvalid("ResidualVectorDofs1;3;100;Through;50", "nodes");
}

TEST_CASE("UNBALNC has its lines of EPOINTs, tables for its constants and joins the dynamic load sets",
          "[model][cards]")
{
    Model model = readBulk(
        "Node;4001;;0.;0.;0.\n"
        "TableDynamic1;5;;;0.;1.;1.;1.;EndTable\n"
        "RotorUnbalanceLoad;4880;;0.075;4001;0.;1.;0.;;0.035;45.;;0.25;;;Mass;4101;4102;4103;4104;4105;4106;4201;4202;"
        "4203;4204;4205;4206;4301;4303;4304\n"
        "RotorUnbalanceLoad;881;;0.1;4001;0.;1.;0.\n"
        "RotorUnbalanceLoad;882;5;;4001;1.;0.;0.;5;;;5;;1.;2.");
    const RotorUnbalanceLoadCard sampleCard = cardAt<RotorUnbalanceLoadTable>(model, 0);
    REQUIRE((sampleCard.massTableId == 0 && sampleCard.mass == 0.075 && sampleCard.nodeId == 4001 &&
             sampleCard.direction[1] == 1.0));
    REQUIRE((sampleCard.radialOffsetTableId == 0 && sampleCard.radialOffset == 0.035 &&
             sampleCard.angularPosition == 45.0 && sampleCard.axialOffset == 0.25));
    REQUIRE((sampleCard.startTimeOrFrequency == 0.0 && sampleCard.stopTimeOrFrequency == 999999.0 &&
             sampleCard.correctionFlag == UnbalanceCorrection::Mass));
    REQUIRE((sampleCard.extraPoints[0] == 4101 && sampleCard.extraPoints[5] == 4106 &&
             sampleCard.extraPoints[6] == 4201 && sampleCard.extraPoints[11] == 4206));
    REQUIRE((sampleCard.extraPoints[12] == 4301 && sampleCard.extraPoints[13] == 4303 &&
             sampleCard.extraPoints[14] == 4304));
    const RotorUnbalanceLoadCard minimal = cardAt<RotorUnbalanceLoadTable>(model, 1);
    REQUIRE((minimal.radialOffset == 1.0 && minimal.angularPosition == 0.0 && minimal.axialOffset == 0.0 &&
             minimal.correctionFlag == UnbalanceCorrection::None));
    REQUIRE(minimal.extraPoints == std::array<base::EntityId, RotorUnbalanceLoadCard::EXTRA_POINT_COUNT>{});
    const RotorUnbalanceLoadCard tables = cardAt<RotorUnbalanceLoadTable>(model, 2);
    REQUIRE((tables.massTableId == 5 && tables.radialOffsetTableId == 5 && tables.axialOffsetTableId == 5 &&
             tables.startTimeOrFrequency == 1.0 && tables.stopTimeOrFrequency == 2.0));
    // The set is one of the dynamic loads (the DLOAD command selects it); the grid, the EPOINTs and the tables are
    // references.
    REQUIRE(cardsOfSet(model, CardCategory::DynamicLoad, 4880).size() == 1);
    RecordingVisitor visitor;
    model.table<RotorUnbalanceLoadTable>().visitReferences(2, visitor);
    REQUIRE((visitor.fields.size() == 4 && visitor.fields[0] == "nodeId" && visitor.fields[1] == "mass"));
    REQUIRE(visitor.targets[1] == ReferenceTarget::DynamicTable);
    RecordingVisitor points;
    model.table<RotorUnbalanceLoadTable>().visitReferences(0, points);
    REQUIRE(points.fields.size() == 1 + 15);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE(cardAt<RotorUnbalanceLoadTable>(model, 2).massTableId == 105);
    REQUIRE(cardAt<RotorUnbalanceLoadTable>(model, 2).axialOffsetTableId == 105);
    Renumbering::apply(model, RenumberSpace::DynamicLoadSet, Renumbering::offset(1000));
    REQUIRE(cardAt<RotorUnbalanceLoadTable>(model, 0).id == 5880);
    requireRoundTrip(model);

    expectInvalid("RotorUnbalanceLoad;1;;;1001;0.;1.;0.", "mass");
    expectInvalid("RotorUnbalanceLoad;1;;0.1;0;0.;1.;0.", "nodeId");
    expectInvalid("RotorUnbalanceLoad;1;;0.1;1001;0.;1.", "directionZ");
    expectInvalid("RotorUnbalanceLoad;1;;0.1;1001;0.;1.;0.;;;;;;;;FOO", "correctionFlag");
    expectInvalid("RotorUnbalanceLoad;1;;0.1;1001;0.;1.;0.;;;;;;-1.", "time/startTimeOrFrequency");
    expectInvalid("RotorUnbalanceLoad;1;;0.1;1001;0.;1.;0.;;;;;;;0.", "time/stopTimeOrFrequency");
    expectInvalid("RotorUnbalanceLoad;1;;0.1;1001;0.;1.;0.;;;;;;;;;-1", "forcePointT1");
}

TEST_CASE("DTI keeps the values of a record as they were written", "[model][cards]")
{
    const Model model = readBulk(
        "DirectTable;BLKA;0;12;48;20000;0;5;0\n"
        "DirectTable;BLKA;1;1;0.;2;0.;3;0.\n"
        "DirectTable;BLKA;2\n"
        "DirectTable;PROPS;1;YIELD;312500.;ULTIMATE;4.5E+5\n"
        "DirectTable;xxx;1;2.;;abc;ENDREC");
    const DirectTableCard header = cardAt<DirectTableTable>(model, 0);
    REQUIRE((header.entryName == "BLKA" && header.recordNumber == 0 && header.values.size() == 6));
    REQUIRE((header.values[2].kind == DirectTableKind::Integer && header.values[2].integerValue == 20000));
    REQUIRE(cardAt<DirectTableTable>(model, 1).values[1].kind == DirectTableKind::Real);
    REQUIRE(cardAt<DirectTableTable>(model, 2).values.empty());
    const DirectTableCard words = cardAt<DirectTableTable>(model, 3);
    REQUIRE((words.values[0].kind == DirectTableKind::Text && words.values[0].textValue == "YIELD"));
    REQUIRE((words.values[1].kind == DirectTableKind::Real && words.values[1].realValue == 312500.0));
    const DirectTableCard blanks = cardAt<DirectTableTable>(model, 4);
    REQUIRE((blanks.entryName == "xxx" && blanks.values.size() == 4)); // a blank in the middle counts
    REQUIRE((blanks.values[1].kind == DirectTableKind::Blank && blanks.values[2].textValue == "abc"));
    REQUIRE(blanks.values[3].textValue == "ENDREC");
    requireRoundTrip(model);

    requireRoundTrip(readBulk("DirectTable;1X;0"));
    expectInvalid("DirectTable;NAME", "recordNumber");
    expectInvalid("DirectTable;NAME;-1", "recordNumber");
    expectInvalid("DirectTable;NAME;1;3000000000", "values");
}

// ------------------------------------------------------------------------------------------------ Case Control

TEST_CASE("the Case Control commands of the solution control resolve to their cards and renumber with them",
          "[model][cards][control]")
{
    Model model = readModelText(
        "[executive]\n"
        "SolutionSequence;107\n"
        "[case]\n"
        "TimeStepSelection;5\n"
        "NonlinearStaticParamsSelection;6\n"
        "FreqSetSelection;7\n"
        "ComplexEigenMethodSelection;8\n"
        "ModalDampingSelection;9\n"
        "RandomAnalysisSelection;10\n"
        "InitialConditionSelection;11\n"
        "SubcaseStart;1\n"
        "EigenMethodSelection;12\n"
        "[model]\n"
        "Node;1;;0.;0.;0.\n"
        "TransientTimeSteps;5;10;0.1\n"
        "NonlinearTimeSteps;5;10;0.1\n"
        "NonlinearStaticParams;6\n"
        "ArcLengthParams;6\n"
        "FreqSpreadAroundModes4;7\n"
        "FreqList;7;1.;2.\n"
        "ComplexEigenMethod;8;HessenbergMethod;;;;;4\n"
        "ComplexEigenPoles;8;-3.75;0.;2\n"
        "TableModalDamping1;9;;0.;0.01;EndTable\n"
        "RandomSpectralDensity;10;1;1;1.\n"
        "TransientInitialCondition;11;1;1;0.1\n"
        "LanczosEigenMethod;12;;;3\n");
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(cardsOfSet(model, CardCategory::TimeStep, 5).size() == 2);
    REQUIRE(cardsOfSet(model, CardCategory::Frequency, 7).size() == 2);

    // SDAMPING also selects a TABLEDi (the viscoelastic material of a direct frequency response).
    Model viscous = readModelText(
        "[executive]\n"
        "SolutionSequence;108\n"
        "[case]\n"
        "ModalDampingSelection;9\n"
        "InitialConditionSelection;11\n"
        "[model]\n"
        "TableDynamic1;9;;;0.;1.;1.;1.;EndTable\n"
        "NodeTemperatureLoad;11;1;20.\n"
        "Node;1\n");
    REQUIRE(unresolvedIn(viscous) == 0);
    REQUIRE(resolveReference(viscous, ReferenceTarget::HybridDampingRotorOptionsTable, 9).has_value());

    // A RANDOM that names a Case Control SET is not a reference to Bulk Data.
    Model sets = readModelText(
        "[executive]\nSolutionSequence;111\n[case]\nSetDefinition;20;1;2\nRandomAnalysisSelection;20\nSubcaseStart;"
        "1\nRandomAnalysisSelection;99\n[model]\nNode;1\n");
    REQUIRE(unresolvedIn(sets) == 1); // only the RANDOM = 99 of the subcase

    Renumbering::apply(model, RenumberSpace::TimeStep, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::Nonlinear, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::FrequencySet, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::ComplexMethod, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::HybridDampingRotorOptionsTable, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::Random, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::InitialCondition, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::Method, Renumbering::offset(100));
    REQUIRE(globalCommand(model, "TimeStepSelection") == 105);
    REQUIRE(globalCommand(model, "NonlinearStaticParamsSelection") == 106);
    REQUIRE(globalCommand(model, "FreqSetSelection") == 107);
    REQUIRE(globalCommand(model, "ComplexEigenMethodSelection") == 108);
    REQUIRE(globalCommand(model, "ModalDampingSelection") == 109);
    REQUIRE(globalCommand(model, "RandomAnalysisSelection") == 110);
    REQUIRE(globalCommand(model, "InitialConditionSelection") == 111);

    oor::testing::requireModelFileRoundTrip(model);
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(cardAt<NonlinearTimeStepsTable>(model).id == 105);
    REQUIRE(cardAt<ArcLengthParamsTable>(model).id == 106);

    // An ID that a TABLEDi and a TABDMP1 both have is ambiguous for SDAMPING: only the table the renumbering moves is
    // not enough to tell, so the command is left alone; the same for IC with a temperature set.
    Model both = readModelText(
        "[executive]\n"
        "SolutionSequence;108\n"
        "[case]\n"
        "ModalDampingSelection;9\n"
        "InitialConditionSelection;11\n"
        "[model]\n"
        "TableDynamic1;9;;;0.;1.;1.;1.;EndTable\n"
        "TableModalDamping1;9;;0.;0.01;EndTable\n"
        "NodeTemperatureLoad;11;1;20.\n"
        "TransientInitialCondition;11;1;1;0.1\n"
        "Node;1\n");
    Renumbering::apply(both, RenumberSpace::HybridDampingRotorOptionsTable, Renumbering::offset(100));
    Renumbering::apply(both, RenumberSpace::InitialCondition, Renumbering::offset(100));
    REQUIRE(globalCommand(both, "ModalDampingSelection") == 9);
    REQUIRE(globalCommand(both, "InitialConditionSelection") == 11);
    REQUIRE(cardAt<TableModalDamping1Table>(both).id == 109);
    // Without the clash, the command follows the table that is moved.
    Model clear = readModelText(
        "[executive]\n"
        "SolutionSequence;108\n"
        "[case]\n"
        "ModalDampingSelection;9\n"
        "InitialConditionSelection;11\n"
        "[model]\n"
        "TableDynamic1;9;;;0.;1.;1.;1.;EndTable\n"
        "NodeTemperatureLoad;11;1;20.\n"
        "Node;1\n");
    Renumbering::apply(clear, RenumberSpace::DynamicTable, Renumbering::offset(100));
    Renumbering::apply(clear, RenumberSpace::LoadCombinationSet, Renumbering::offset(100));
    REQUIRE(globalCommand(clear, "ModalDampingSelection") == 109);
    REQUIRE(globalCommand(clear, "InitialConditionSelection") == 111);
}

TEST_CASE("intermediate output uses exact words or a positive model integer without a field-width cap",
          "[model][cards][nonlinear]")
{
    Model model = readBulk("NonlinearStaticParams;1");
    NonlinearStaticParamsCard card = cardAt<NonlinearStaticParamsTable>(model);
    for (const std::string value :
         {"Yes", "No", "All", "9999999999", "2147483648", "9223372036854775807", "+10", "0001"})
    {
        INFO(value);
        card.intermediateOutput = value;
        REQUIRE_FALSE(NonlinearStaticParamsFormat::check(card).has_value());
        model.table<NonlinearStaticParamsTable>().store(0, card);
        requireRoundTrip(model);
    }
    for (const std::string value : {"0", "-1", "9223372036854775808", "yes", "YES", "all", "NO"})
    {
        INFO(value);
        card.intermediateOutput = value;
        const auto problem = NonlinearStaticParamsFormat::check(card);
        REQUIRE(problem.has_value());
        REQUIRE(std::string_view(problem->field) == "intermediateOutput");
    }
}
