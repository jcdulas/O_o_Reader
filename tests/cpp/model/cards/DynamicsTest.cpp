// Checks of the dynamics cards: the eigenvalue methods, the frequency sets, the dynamic loads and their references.
// Defaults that depend on other fields, variants, the layout of the cards with variable lists and the way the dynamic
// load sets are resolved.

#include "oor/model/cards/dynamics/dynamicLoadCombination/DynamicLoadCombinationTable.hpp"
#include "oor/model/cards/dynamics/dynamicLoadScale/DynamicLoadScaleTable.hpp"
#include "oor/model/cards/dynamics/freqLinearSteps1/FreqLinearSteps1Table.hpp"
#include "oor/model/cards/dynamics/freqList/FreqListTable.hpp"
#include "oor/model/cards/dynamics/freqLogSteps2/FreqLogSteps2Table.hpp"
#include "oor/model/cards/dynamics/freqResponseLoad1/FreqResponseLoad1Table.hpp"
#include "oor/model/cards/dynamics/freqResponseLoad2/FreqResponseLoad2Table.hpp"
#include "oor/model/cards/dynamics/lanczosEigenMethod/LanczosEigenMethodTable.hpp"
#include "oor/model/cards/dynamics/realEigenMethod/RealEigenMethodTable.hpp"
#include "oor/model/cards/dynamics/transientFormulaLoad2/TransientFormulaLoad2Table.hpp"
#include "oor/model/cards/dynamics/transientTableLoad1/TransientTableLoad1Table.hpp"
#include "oor/model/cards/tables/tableDynamic1/TableDynamic1Table.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

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

std::size_t unresolvedIn(Model& model)
{
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    return ReferenceChecker::check(model, sink);
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

} // namespace

TEST_CASE("EIGRL has its defaults and takes the options of the second line", "[model][cards]")
{
    const Model model = readBulk(
        "LanczosEigenMethod;1;;;5\n"
        "LanczosEigenMethod;2;0.4;6.5;12;2;7;0.5;Mass\n"
        "LanczosEigenMethod;3;;;;;;;;Alpha=0.8;FrequencySegmentCount=2;SegmentFrequency1=1.5;SegmentFrequency2=2.5;"
        "SegmentFrequency3=4.\n"
        "LanczosEigenMethod;4;;;;;;;;LowerFrequency = "
        "0.5;UpperFrequency=100.;DesiredModeCount=6;MessageLevel=1;MaximumSet=9;ShiftScale=2.5;Normalization=Maximum");
    const LanczosEigenMethodCard plain = cardAt<LanczosEigenMethodTable>(model, 0);
    REQUIRE(plain.desiredModeCount == 5);
    REQUIRE_FALSE(plain.lowerFrequency.has_value());
    REQUIRE_FALSE(plain.upperFrequency.has_value());
    REQUIRE(plain.messageLevel == 0);
    REQUIRE(plain.maximumSet == 0);
    REQUIRE(plain.norm == EigenvectorNorm::Default);
    REQUIRE(plain.alpha == 1.0);
    REQUIRE(plain.frequencySegmentCount == 1);
    REQUIRE(plain.segmentFrequencies.empty());
    const LanczosEigenMethodCard range = cardAt<LanczosEigenMethodTable>(model, 1);
    REQUIRE(range.lowerFrequency == 0.4);
    REQUIRE(range.upperFrequency == 6.5);
    REQUIRE(range.messageLevel == 2);
    REQUIRE(range.maximumSet == 7);
    REQUIRE(range.shiftScale == 0.5);
    REQUIRE(range.norm == EigenvectorNorm::Mass);
    const LanczosEigenMethodCard parallel = cardAt<LanczosEigenMethodTable>(model, 2);
    REQUIRE(parallel.alpha == 0.8);
    REQUIRE(parallel.frequencySegmentCount == 2);
    REQUIRE(parallel.segmentFrequencies == std::vector<double>{1.5, 2.5, 4.0});
    // The options that stand for fields of the first line give the same card as those fields.
    const LanczosEigenMethodCard options = cardAt<LanczosEigenMethodTable>(model, 3);
    REQUIRE(options.lowerFrequency == 0.5);
    REQUIRE(options.upperFrequency == 100.0);
    REQUIRE(options.desiredModeCount == 6);
    REQUIRE(options.messageLevel == 1);
    REQUIRE(options.maximumSet == 9);
    REQUIRE(options.shiftScale == 2.5);
    REQUIRE(options.norm == EigenvectorNorm::Max);
}

TEST_CASE("EIGRL options are refused when they repeat, are unknown or have a gap", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("LanczosEigenMethod;1;;;;;;;;Alpha=0.8;Alpha=0.9"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("LanczosEigenMethod;1;;;;;;;;FrequencySegmentCount=0"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("LanczosEigenMethod;1;;;;;;;;SegmentFrequency2=1.5"), base::DeckError); // F1 is missing
    REQUIRE_THROWS_AS(readBulk("LanczosEigenMethod;1;;;;;;;;F16=1.5"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("LanczosEigenMethod;1;;;;;;;;Normalization=POINT"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("LanczosEigenMethod;1;;;;;;;;ND"), base::DeckError); // no value
    REQUIRE_THROWS_AS(readBulk("LanczosEigenMethod;1;;;;;;;;LowerFrequency=1.;LowerFrequency=2."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("LanczosEigenMethod;1;2.;1."), base::DeckError); // V1 above V2
    REQUIRE_THROWS_AS(readBulk("LanczosEigenMethod;1;;;;7"), base::DeckError);  // MSGLVL 7
}

TEST_CASE("EIGRL writes the options that are not defaults, one after the other, from field 9", "[model][cards]")
{
    const Model model =
        readBulk("LanczosEigenMethod;1;;;5\nLanczosEigenMethod;2;;;;;;;;FrequencySegmentCount=3;SegmentFrequency1=1.5");
    const std::string text = writeModelText(model);
    REQUIRE(text.find("LanczosEigenMethod;1;;;5\n") != std::string::npos); // no ALPH and no NUMS
    REQUIRE(text.find("LanczosEigenMethod;2;;;;;;;;FrequencySegmentCount=3;SegmentFrequency1=1.5\n") !=
            std::string::npos);
    // The defaults are written when the writer is asked to keep them.
    oor::testing::requireModelFileRoundTrip(model, false);
    const Model again = readModelText(writeModelText(model), true);
    REQUIRE(cardAt<LanczosEigenMethodTable>(again, 0).alpha == 1.0);
    REQUIRE(cardAt<LanczosEigenMethodTable>(again, 1).segmentFrequencies == std::vector<double>{1.5});
}

TEST_CASE("an EIGRL option keeps all its digits in the small format by going to the large one", "[model][cards]")
{
    const Model model = readBulk("LanczosEigenMethod;1;;;;;;;;Alpha=0.123456789");

    oor::testing::requireModelFileRoundTrip(model);
    REQUIRE(cardAt<LanczosEigenMethodTable>(readModelText(writeModelText(model), true)).alpha == 0.123456789);
}

TEST_CASE("EIGR keeps a blank ND blank and keeps G and C for the point normalisation", "[model][cards]")
{
    const Model model = readBulk(
        "RealEigenMethod;1;Lanczos;1.;2.\n"
        "RealEigenMethod;2\n"
        "RealEigenMethod;3;SturmInverse;1.\n"
        "RealEigenMethod;4;InverseIteration;1.;;4\n"
        "RealEigenMethod;5;Givens;;;;3;Point;5;3\n"
        "RealEigenMethod;6;AutomaticHouseholder;;;;;Maximum\n"
        "RealEigenMethod;7;Householder");
    REQUIRE(cardAt<RealEigenMethodTable>(model, 0).method == RealEigenMethodMethod::Lanczos);
    REQUIRE_FALSE(cardAt<RealEigenMethodTable>(model, 0).desiredModeCount.has_value());
    REQUIRE(cardAt<RealEigenMethodTable>(model, 1).method == RealEigenMethodMethod::Lanczos); // a blank METHOD
    REQUIRE(cardAt<RealEigenMethodTable>(model, 1).normalization == RealEigenMethodNorm::Mass);
    REQUIRE_FALSE(cardAt<RealEigenMethodTable>(model, 1).frequency1.has_value());
    REQUIRE_FALSE(cardAt<RealEigenMethodTable>(model, 2)
                      .desiredModeCount.has_value()); // not 600: a blank asks for every root in the range
    const RealEigenMethodCard inverse = cardAt<RealEigenMethodTable>(model, 3);
    REQUIRE(inverse.estimatedModeCount == 4);
    REQUIRE_FALSE(inverse.desiredModeCount.has_value());
    const RealEigenMethodCard point = cardAt<RealEigenMethodTable>(model, 4);
    REQUIRE(point.desiredModeCount == 3);
    REQUIRE(point.normalization == RealEigenMethodNorm::Point);
    REQUIRE(point.normNode == 5);
    REQUIRE(point.normComponent == 3);
    REQUIRE(cardAt<RealEigenMethodTable>(model, 5).method == RealEigenMethodMethod::AutoHouseholder);
    REQUIRE(cardAt<RealEigenMethodTable>(model, 5).normalization == RealEigenMethodNorm::Max);
    REQUIRE_FALSE(cardAt<RealEigenMethodTable>(model, 6).desiredModeCount.has_value());
    // A blank ND is written blank, a given one as given.

    oor::testing::requireModelFileRoundTrip(model);
    (void)0; // LAN is the default METHOD
    // G and C are not used without POINT, but a card that has them keeps them.
    const Model kept = readBulk("RealEigenMethod;1;Lanczos;;;;;Maximum;5;2");
    REQUIRE(cardAt<RealEigenMethodTable>(kept).normNode == 5);
    REQUIRE(cardAt<RealEigenMethodTable>(kept).normComponent == 2);
    const Model again = readModelText(writeModelText(kept), true);
    REQUIRE(cardAt<RealEigenMethodTable>(again).normNode == 5);
    REQUIRE_THROWS_AS(readBulk("RealEigenMethod;1;Lanczos;;;;;Point"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("RealEigenMethod;1;Lanczos;;;;;Point;5;7"), base::DeckError);
}

TEST_CASE("EIGR and EIGRL share the method ID space and EIGR lists the POINT grid as a reference", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("LanczosEigenMethod;1;;;5\nRealEigenMethod;1;Lanczos"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("RealEigenMethod;1;Lanczos\nLanczosEigenMethod;1;;;5"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("RealEigenMethod;1;Lanczos\nLanczosEigenMethod;2;;;5"));
    Model model = readBulk("RealEigenMethod;5;Givens;;;;3;Point;5;3\nRealEigenMethod;6");
    RecordingVisitor visitor;
    model.table<RealEigenMethodTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"normNode"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node});
    RecordingVisitor none;
    model.table<RealEigenMethodTable>().visitReferences(1, none);
    REQUIRE(none.fields.empty());
}

TEST_CASE("the frequency sets take their defaults and share one set ID space", "[model][cards]")
{
    const Model model = readBulk(
        "FreqList;1;4.15;4.62;12.8;19.7;23.4;33.1;36.5;;34.9;27.6\n"
        "FreqLinearSteps1;1;;0.5\n"
        "FreqLinearSteps1;1;3.6;0.4;11\n"
        "FreqLogSteps2;1;2.;9.");
    const FreqListCard list = cardAt<FreqListTable>(model);
    REQUIRE(list.frequencies == std::vector<double>{4.15, 4.62, 12.8, 19.7, 23.4, 33.1, 36.5, 34.9, 27.6});
    const FreqLinearSteps1Card first = cardAt<FreqLinearSteps1Table>(model, 0);
    REQUIRE(first.frequency1 == 0.0);
    REQUIRE(first.incrementCount == 1);
    REQUIRE(cardAt<FreqLinearSteps1Table>(model, 1).incrementCount == 11);
    REQUIRE(cardAt<FreqLogSteps2Table>(model).intervalCount == 1);
    // A set is made of cards of the three kinds.
    REQUIRE(cardsOfSet(model, CardCategory::Frequency, 1).size() == 4);
    REQUIRE_THROWS_AS(readBulk("FreqLinearSteps1;1;1.;0.5;0"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("FreqLogSteps2;1;0.;2."), base::DeckError);
}

TEST_CASE("DAREA has one or two groups, each with a component from 0 to 6", "[model][cards]")
{
    Model model = readBulk("DynamicLoadScale;3;4;3;5.6;12;2;7.3\nDynamicLoadScale;3;7;;1.5");
    const DynamicLoadScaleCard two = cardAt<DynamicLoadScaleTable>(model, 0);
    REQUIRE(two.points == std::array<base::EntityId, 2>{4, 12});
    REQUIRE(two.components == std::array<std::int32_t, 2>{3, 2});
    REQUIRE(two.scaleFactors == std::array<double, 2>{5.6, 7.3});
    const DynamicLoadScaleCard scalar = cardAt<DynamicLoadScaleTable>(model, 1);
    REQUIRE_FALSE(scalar.hasSecondGroup());
    REQUIRE(scalar.components[0] == 0);
    REQUIRE_THROWS_AS(readBulk("DynamicLoadScale;3;6;7;1."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("DynamicLoadScale;3;6;1;1.;4;1;1.;9;1;1."), base::DeckError); // a third group
    RecordingVisitor visitor;
    model.table<DynamicLoadScaleTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"point1", "point2"});
    RecordingVisitor single;
    model.table<DynamicLoadScaleTable>().visitReferences(1, single);
    REQUIRE(single.fields == std::vector<std::string>{"point1"});
}

TEST_CASE("the TYPE of a dynamic load is a code or an exact neutral word", "[model][cards]")
{
    const Model model = readBulk(
        "TransientTableLoad1;1;9;;;;5\n"
        "TransientTableLoad1;2;9;;;Load;5\n"
        "TransientTableLoad1;3;9;;;Load;5\n"
        "TransientTableLoad1;4;9;;;1;5\n"
        "TransientTableLoad1;5;9;;;Displacement;5\n"
        "TransientTableLoad1;6;9;;;Velocity;5\n"
        "TransientTableLoad1;7;9;;;Acceleration;5\n"
        "TransientTableLoad1;8;9;;;4;5\n"
        "TransientTableLoad1;9;9;;;13;5");
    const std::vector<ExcitationType> expected = {
        ExcitationType::Load,         ExcitationType::Load,         ExcitationType::Load,
        ExcitationType::Displacement, ExcitationType::Displacement, ExcitationType::Velocity,
        ExcitationType::Acceleration, ExcitationType::Flow,         ExcitationType::RigidBodyForce};
    for (std::uint32_t row = 0; row < expected.size(); ++row)
        REQUIRE(cardAt<TransientTableLoad1Table>(model, row).kind == expected[row]);
    REQUIRE_THROWS_AS(readBulk("TransientTableLoad1;1;9;;;5;5"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TransientTableLoad1;1;9;;;LOADS;5"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TransientTableLoad1;1;9;;;X;5"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TransientTableLoad1;1;9;;;Disp;5"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TransientTableLoad1;1;9;;;displacement;5"), base::DeckError);
    // Written as the word for 0 to 3 and as the integer for the others; LOAD is the default and is left blank.

    oor::testing::requireModelFileRoundTrip(model);
}

TEST_CASE("a field that holds an ID or a real keeps them apart", "[model][cards]")
{
    const Model model = readBulk(
        "TransientTableLoad1;1;9;15;;;700\nTransientTableLoad1;2;9;;2.5;;;1.5\nTransientTableLoad1;3;9;;0.;;;2."
        "\nTransientTableLoad1;4;9;;;;700");
    const TransientTableLoad1Card id = cardAt<TransientTableLoad1Table>(model, 0);
    REQUIRE(id.delayId == 15);
    REQUIRE(id.delay == 0.0);
    REQUIRE(id.tableId == 700);
    REQUIRE(id.value == 0.0);
    const TransientTableLoad1Card value = cardAt<TransientTableLoad1Table>(model, 1);
    REQUIRE(value.delayId == 0);
    REQUIRE(value.delay == 2.5);
    REQUIRE(value.tableId == 0);
    REQUIRE(value.value == 1.5);
    const TransientTableLoad1Card none = cardAt<TransientTableLoad1Table>(model, 3);
    REQUIRE(none.delayId == 0);
    REQUIRE(none.delay == 0.0);

    oor::testing::requireModelFileRoundTrip(model);
    (void)0; // a delay of 0. is a blank
    REQUIRE_THROWS_AS(readBulk("TransientTableLoad1;1;9;-4;;;700"), base::DeckError);
}

TEST_CASE("TLOAD2 resolves a blank T2 to T1 and always writes it", "[model][cards]")
{
    const Model model = readBulk(
        "TransientFormulaLoad2;1;9;;;;1.5\nTransientFormulaLoad2;2;9;;;;1.5;4.;10.;30.;0.5;0.25;0.1;0."
        "2\nTransientFormulaLoad2;3;9;;;;;2.");
    const TransientFormulaLoad2Card resolved = cardAt<TransientFormulaLoad2Table>(model, 0);
    REQUIRE(resolved.time1 == 1.5);
    REQUIRE(resolved.time2 == 1.5);
    const TransientFormulaLoad2Card full = cardAt<TransientFormulaLoad2Table>(model, 1);
    REQUIRE(full.time2 == 4.0);
    REQUIRE(full.frequency == 10.0);
    REQUIRE(full.phase == 30.0);
    REQUIRE(full.coefficientC == 0.5);
    REQUIRE(full.exponentB == 0.25);
    REQUIRE(full.initialDisplacementFactor == 0.1);
    REQUIRE(full.initialVelocityFactor == 0.2);
    REQUIRE(cardAt<TransientFormulaLoad2Table>(model, 2).time1 == 0.0); // blank T1 is 0.0 and T2 follows it
    REQUIRE(cardAt<TransientFormulaLoad2Table>(model, 2).time2 == 2.0);

    oor::testing::requireModelFileRoundTrip(model);
    REQUIRE_THROWS_AS(readBulk("TransientFormulaLoad2;1;9;;;;3.;2."), base::DeckError);
}

TEST_CASE("RLOAD1 and RLOAD2 hold IDs or constants and need a function of frequency", "[model][cards]")
{
    const Model model = readBulk(
        "FreqResponseLoad1;1;9;15;;16;;;;701;;Acceleration\n"
        "FreqResponseLoad1;2;9;;3.5;;45.;;1.5;;;Velocity\n"
        "FreqResponseLoad2;3;9;;;;;701;;702\n"
        "FreqResponseLoad2;4;9;;3.5;;45.;;2.5;;10.;Displacement");
    const FreqResponseLoad1Card ids = cardAt<FreqResponseLoad1Table>(model, 0);
    REQUIRE(ids.delayId == 15);
    REQUIRE(ids.phaseId == 16);
    REQUIRE(ids.realTableId == 0);
    REQUIRE(ids.imaginaryTableId == 701);
    REQUIRE(ids.kind == ExcitationType::Acceleration);
    const FreqResponseLoad1Card values = cardAt<FreqResponseLoad1Table>(model, 1);
    REQUIRE(values.delay == 3.5);
    REQUIRE(values.phase == 45.0);
    REQUIRE(values.realValue == 1.5);
    REQUIRE(values.imaginaryTableId == 0);
    REQUIRE(values.imaginaryValue == 0.0);
    const FreqResponseLoad2Card tables = cardAt<FreqResponseLoad2Table>(model, 0);
    REQUIRE(tables.amplitudeTableId == 701);
    REQUIRE(tables.phaseTableId == 702);
    const FreqResponseLoad2Card constants = cardAt<FreqResponseLoad2Table>(model, 1);
    REQUIRE(constants.amplitude == 2.5);
    REQUIRE(constants.phaseAngle == 10.0);
    REQUIRE(constants.kind == ExcitationType::Displacement);
    REQUIRE_THROWS_AS(readBulk("FreqResponseLoad1;1;9"), base::DeckError);            // neither TC/RC nor TD/RD
    REQUIRE_THROWS_AS(readBulk("FreqResponseLoad1;1;9;;;;;5;;;;4"), base::DeckError); // a SOL 700 type
    REQUIRE_THROWS_AS(readBulk("FreqResponseLoad2;1;9;;;;;;;5"), base::DeckError);    // no B(f)
    REQUIRE_THROWS_AS(readBulk("FreqResponseLoad2;1;9;;;;;;0.;5"), base::DeckError);  // B is zero
    REQUIRE_NOTHROW(readBulk("FreqResponseLoad1;1;9;;;;;;;;3."));                     // D alone is enough
}

TEST_CASE("DLOAD keeps its terms in order and refuses a set that contains itself or repeats a term", "[model][cards]")
{
    const Model model =
        readBulk("DynamicLoadCombination;10;-0.5;1.;3;4.5;4\nDynamicLoadCombination;11;1.;1.;3;2.;4;3.;5;4.;6;5.;7");
    const DynamicLoadCombinationCard card = cardAt<DynamicLoadCombinationTable>(model, 1);
    REQUIRE(card.terms.size() == 5);
    REQUIRE(card.terms[1] == DynamicLoadCombinationTerm{2.0, 4});
    REQUIRE(card.terms[4] == DynamicLoadCombinationTerm{5.0, 7});
    REQUIRE(cardAt<DynamicLoadCombinationTable>(model, 0).scaleFactor == -0.5);
    REQUIRE_THROWS_AS(readBulk("DynamicLoadCombination;1;1.;1.;1"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("DynamicLoadCombination;1;1.;1.;2;2.;2"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("DynamicLoadCombination;1;1."), base::DeckError);
}

TEST_CASE("the dynamic load sets, the excitation sets and the tables are resolved", "[model][cards]")
{
    // Every reference resolves: DLOAD 20 uses the dynamic load sets 5 (TLOAD1) and 6 (TLOAD2, RLOAD2 shares set 7 with
    // DAREA); the TLOAD1 excites the DAREA set 7, the TLOAD2 the static load set 10; the tables 13 and 14 exist.
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "PointForceLoad;10;1;;1.;1.;0.;0.\n"
        "DynamicLoadScale;7;1;1;1.\n"
        "TableDynamic1;13;;;0.;0.;1.;1.;EndTable\n"
        "TableDynamic2;14;0.;0.;0.;1.;1.;EndTable\n"
        "TransientTableLoad1;5;7;;;;13\n"
        "TransientFormulaLoad2;6;10;;;;0.;1.;2.\n"
        "FreqResponseLoad1;8;7;;;;;14\n"
        "FreqResponseLoad2;9;7;;;;;13;;14\n"
        "DynamicLoadCombination;20;1.;1.;5;2.;6");
    REQUIRE(unresolvedIn(model) == 0);

    // A set that no card has, and a table that does not exist, do not resolve.
    Model wrong = readBulk(
        "TransientTableLoad1;5;7;;;;13\nTransientFormulaLoad2;6;10;;;;0.;1.;2.\nFreqResponseLoad1;8;7;;;;;"
        "14\nDynamicLoadCombination;20;1.;1.;5;2.;6;3.;7\nDynamicLoadScale;7;1;1;1.");
    // The GRID 1 of the DAREA, the static load set 10, and the tables 13 and 14. The set 7 of the DLOAD resolves: the
    // DAREA is a card of the dynamic load category.
    REQUIRE(unresolvedIn(wrong) == 4);

    REQUIRE(categoriesOfSet(ReferenceTarget::DynamicLoadSet).size() == 1);
    REQUIRE(categoryOfSet(ReferenceTarget::DynamicLoadSet) == CardCategory::DynamicLoad);
    REQUIRE(categoriesOfSet(ReferenceTarget::ExcitationSet).size() == 2);
    REQUIRE_FALSE(categoryOfSet(ReferenceTarget::ExcitationSet).has_value()); // two categories
    REQUIRE(categoriesOfSet(ReferenceTarget::Node).empty());
    REQUIRE(resolveReference(model, ReferenceTarget::ExcitationSet, 7).has_value());  // a DAREA
    REQUIRE(resolveReference(model, ReferenceTarget::ExcitationSet, 10).has_value()); // a FORCE
    REQUIRE_FALSE(resolveReference(model, ReferenceTarget::ExcitationSet, 99).has_value());
    // The category is the grain: a set of TLOAD1 cards is a dynamic load set, and an excitation set too.
    REQUIRE(resolveReference(model, ReferenceTarget::ExcitationSet, 5).has_value());
    REQUIRE(resolveReference(model, ReferenceTarget::DynamicLoadSet, 5).has_value());
    REQUIRE_FALSE(resolveReference(model, ReferenceTarget::DynamicLoadSet, 10).has_value());
    REQUIRE(resolveReference(model, ReferenceTarget::DynamicTable, 13).has_value());
    REQUIRE(setIdsOfCategory(model, CardCategory::DynamicLoad) ==
            std::unordered_set<base::EntityId>{5, 6, 7, 8, 9, 20});
}

TEST_CASE("the excitation of an enforced motion is not looked up and a removed set stops resolving", "[model][cards]")
{
    // TYPE other than LOAD: the EXCITEID is the set of an SPCD, which is not a typed card yet.
    Model motion = readBulk("TransientTableLoad1;5;99;;;Displacement;;5.");
    REQUIRE(unresolvedIn(motion) == 0);
    Model applied = readBulk("TransientTableLoad1;5;99;;;;;5.");
    REQUIRE(unresolvedIn(applied) == 1);
    Model model = readBulk("DynamicLoadScale;7;1;1;1.\nTransientTableLoad1;5;7;;;;;5.");
    REQUIRE(unresolvedIn(model) == 1); // only the GRID 1 of the DAREA
    model.remove(*cardsOfSet(model, CardCategory::DynamicLoad, 7).begin());
    REQUIRE(unresolvedIn(model) == 1); // the TLOAD1 now has no set 7
}

TEST_CASE("the dynamic loads list their references with what each points to", "[model][cards]")
{
    Model model = readBulk(
        "TransientTableLoad1;1;9;15;;;13\n"
        "TransientTableLoad1;2;9;;;Displacement;;5.\n"
        "TransientFormulaLoad2;3;9;15;;;0.;1.\n"
        "FreqResponseLoad1;4;9;15;;16;;13;;14\n"
        "FreqResponseLoad2;5;9;15;;16;;13;;14\n"
        "DynamicLoadCombination;6;1.;1.;1;2.;2");
    RecordingVisitor tload1;
    model.table<TransientTableLoad1Table>().visitReferences(0, tload1);
    REQUIRE(tload1.fields == std::vector<std::string>{"excitationId", "delayId", "tableId"});
    REQUIRE(tload1.targets == std::vector<ReferenceTarget>{ReferenceTarget::ExcitationSet, ReferenceTarget::DelaySet,
                                                           ReferenceTarget::DynamicTable});
    RecordingVisitor motion;
    model.table<TransientTableLoad1Table>().visitReferences(1, motion);
    REQUIRE(motion.fields == std::vector<std::string>{"excitationId"});
    REQUIRE(motion.targets == std::vector<ReferenceTarget>{ReferenceTarget::Other});
    RecordingVisitor tload2;
    model.table<TransientFormulaLoad2Table>().visitReferences(0, tload2);
    REQUIRE(tload2.fields == std::vector<std::string>{"excitationId", "delayId"});
    RecordingVisitor rload1;
    model.table<FreqResponseLoad1Table>().visitReferences(0, rload1);
    REQUIRE(rload1.fields ==
            std::vector<std::string>{"excitationId", "delayId", "phaseId", "realTableId", "imaginaryTableId"});
    REQUIRE(rload1.targets[3] == ReferenceTarget::DynamicTable);
    RecordingVisitor rload2;
    model.table<FreqResponseLoad2Table>().visitReferences(0, rload2);
    REQUIRE(rload2.fields ==
            std::vector<std::string>{"excitationId", "delayId", "phaseId", "amplitudeTableId", "phaseTableId"});
    RecordingVisitor dload;
    model.table<DynamicLoadCombinationTable>().visitReferences(0, dload);
    REQUIRE(dload.fields == std::vector<std::string>{"loadSetId1", "loadSetId1"});
    REQUIRE(dload.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::DynamicLoadSet, ReferenceTarget::DynamicLoadSet});
}

TEST_CASE("the dynamic loads share one set ID space with DAREA and with each other", "[model][cards]")
{
    REQUIRE_NOTHROW(
        readBulk("TransientTableLoad1;5;9;;;;;5.\nTransientFormulaLoad2;5;9;;;;0.;1.\nFreqResponseLoad1;5;9;;;;;;5."
                 "\nDynamicLoadScale;5;1;1;1."));
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;5;;;0.;0.;1.;1.;EndTable\nTableDynamic1;5;;;0.;0.;1.;1.;EndTable"),
                      base::DeckError);
}
