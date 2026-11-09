// The cards of the superelement family: what each reads (every variant of its format), what it refuses, the
// references it lists and how they are looked up. The generic tests of CardRoundTripTest.cpp cover reading, writing and
// copying every sample in every format; this file covers what each card means.

#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/superelements/axisymConeSector/AxisymConeSectorTable.hpp"
#include "oor/model/cards/superelements/axisymConeSupport/AxisymConeSupportTable.hpp"
#include "oor/model/cards/superelements/fixedBoundaryDofs/FixedBoundaryDofsTable.hpp"
#include "oor/model/cards/superelements/fixedBoundaryDofs1/FixedBoundaryDofs1Table.hpp"
#include "oor/model/cards/superelements/freeBoundaryDofs/FreeBoundaryDofsTable.hpp"
#include "oor/model/cards/superelements/freeBoundaryDofs1/FreeBoundaryDofs1Table.hpp"
#include "oor/model/cards/superelements/massCaseCombination/MassCaseCombinationTable.hpp"
#include "oor/model/cards/superelements/superelementBoundarySearch/SuperelementBoundarySearchTable.hpp"
#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsTable.hpp"
#include "oor/model/cards/superelements/superelementConnection/SuperelementConnectionTable.hpp"
#include "oor/model/cards/superelements/superelementCopyPoints/SuperelementCopyPointsTable.hpp"
#include "oor/model/cards/superelements/superelementCopySequence/SuperelementCopySequenceTable.hpp"
#include "oor/model/cards/superelements/superelementDesignLink/SuperelementDesignLinkTable.hpp"
#include "oor/model/cards/superelements/superelementDesignResp2/SuperelementDesignResp2Table.hpp"
#include "oor/model/cards/superelements/superelementDesignResp3/SuperelementDesignResp3Table.hpp"
#include "oor/model/cards/superelements/superelementElemTransfer/SuperelementElemTransferTable.hpp"
#include "oor/model/cards/superelements/superelementExcludedNodes/SuperelementExcludedNodesTable.hpp"
#include "oor/model/cards/superelements/superelementExteriorPoints/SuperelementExteriorPointsTable.hpp"
#include "oor/model/cards/superelements/superelementExternalBoundary/SuperelementExternalBoundaryTable.hpp"
#include "oor/model/cards/superelements/superelementFixedDofSet/SuperelementFixedDofSetTable.hpp"
#include "oor/model/cards/superelements/superelementFixedDofSet1/SuperelementFixedDofSet1Table.hpp"
#include "oor/model/cards/superelements/superelementFreeDofSet/SuperelementFreeDofSetTable.hpp"
#include "oor/model/cards/superelements/superelementFreeDofSet1/SuperelementFreeDofSet1Table.hpp"
#include "oor/model/cards/superelements/superelementGeneralizedDofSet/SuperelementGeneralizedDofSetTable.hpp"
#include "oor/model/cards/superelements/superelementGeneralizedDofSet1/SuperelementGeneralizedDofSet1Table.hpp"
#include "oor/model/cards/superelements/superelementGeneralizedPointCount/SuperelementGeneralizedPointCountTable.hpp"
#include "oor/model/cards/superelements/superelementInteriorNodes/SuperelementInteriorNodesTable.hpp"
#include "oor/model/cards/superelements/superelementLabel/SuperelementLabelTable.hpp"
#include "oor/model/cards/superelements/superelementLoadScale/SuperelementLoadScaleTable.hpp"
#include "oor/model/cards/superelements/superelementLocation/SuperelementLocationTable.hpp"
#include "oor/model/cards/superelements/superelementMirrorPlane/SuperelementMirrorPlaneTable.hpp"
#include "oor/model/cards/superelements/superelementReleasedDofs/SuperelementReleasedDofsTable.hpp"
#include "oor/model/cards/superelements/superelementSupport/SuperelementSupportTable.hpp"
#include "oor/model/cards/superelements/superelementTree/SuperelementTreeTable.hpp"
#include "oor/model/cards/superelements/superelementUserDofSet/SuperelementUserDofSetTable.hpp"
#include "oor/model/cards/superelements/superelementUserDofSet1/SuperelementUserDofSet1Table.hpp"
#include "support/SuperelementTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

IdRun run(base::EntityId first, base::EntityId last)
{
    return IdRun{first, last};
}

} // namespace

// --- the lists of points and elements ------------------------------------------------------------------------------

TEST_CASE("SESET reads points, THRU ranges and both, and keeps them as written", "[model][superelements]")
{
    const Model model = readBulk(
        "SuperelementInteriorNodes;5;3;19;26;27;158\nSuperelementInteriorNodes;2;19;Through;"
        "158\nSuperelementInteriorNodes;7;1;Through;5;10;12;Through;14");
    REQUIRE(cardAt<SuperelementInteriorNodesTable>(model, 0).superelementId == 5);
    REQUIRE(cardAt<SuperelementInteriorNodesTable>(model, 0).grids ==
            std::vector<IdRun>{run(3, 3), run(19, 19), run(26, 26), run(27, 27), run(158, 158)});
    REQUIRE(cardAt<SuperelementInteriorNodesTable>(model, 1).grids == std::vector<IdRun>{run(19, 158)});
    REQUIRE(cardAt<SuperelementInteriorNodesTable>(model, 2).grids ==
            std::vector<IdRun>{run(1, 5), run(10, 10), run(12, 14)});
    requireRoundTrip(model);
}

TEST_CASE("SESET is filed under the superelement it defines and its points may name nothing", "[model][superelements]")
{
    Model model = readBulk("SuperelementInteriorNodes;5;2;17;Through;20");
    REQUIRE(SuperelementInteriorNodesTable::INFO.category == CardCategory::Superelement);
    REQUIRE(SuperelementInteriorNodesTable::INFO.idSpace == base::IdSpace::Group);
    REQUIRE(model.findTable<SuperelementInteriorNodesTable>()->idAtRow(0) == 5);
    // a point that no card defines is not reported, whether it is named or in a range.
    REQUIRE(unresolvedIn(model) == 0);
    const ScopedRecorder recorder = referencesOf<SuperelementInteriorNodesTable>(model);
    REQUIRE(recorder.seen.size() == 3);
    for (const ScopedReference& reference : recorder.seen)
    {
        REQUIRE(reference.target == ReferenceTarget::NodeInRange);
        REQUIRE_FALSE(reference.scoped);
    }
}

TEST_CASE("SESET refuses what the format does not allow", "[model][superelements]")
{
    expectInvalid("SuperelementInteriorNodes;0;1;2", "superelementId");
    expectInvalid("SuperelementInteriorNodes;5", "nodes");
    expectInvalid("SuperelementInteriorNodes;5;10;Through;4", "nodes");
    expectInvalid("SuperelementInteriorNodes;5;Through;10", "Through");
    expectInvalid("SuperelementInteriorNodes;5;0", "nodes");
}

TEST_CASE("SEELT lists elements that must exist, and the ends of a range that may not", "[model][superelements]")
{
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "IsoMat;1;1.E+7;;0.3\n"
        "RodProp;1;1;1.\n"
        "Rod2;10;1;1;2\n"
        "SuperelementElemTransfer;2;10\n"
        "SuperelementElemTransfer;0;10;Through;20");
    REQUIRE(cardAt<SuperelementElemTransferTable>(model, 0).elements == std::vector<IdRun>{run(10, 10)});
    REQUIRE(cardAt<SuperelementElemTransferTable>(model, 1).superelementId == 0); // 0 is valid with a BEGIN SUPER
    REQUIRE(unresolvedIn(model) == 0); // 10 exists, the range ends are allowed not to
    Model missing = readBulk("SuperelementElemTransfer;2;10;11");
    REQUIRE(unresolvedIn(missing) == 2); // a single element no card defines is an unresolved reference
    // The residual structure keeps its number whatever the renumbering does.
    Renumbering::apply(model, RenumberSpace::Superelement, Renumbering::offset(100));
    REQUIRE(cardAt<SuperelementElemTransferTable>(model, 0).superelementId == 102);
    REQUIRE(cardAt<SuperelementElemTransferTable>(model, 1).superelementId == 0);
    expectInvalid("SuperelementElemTransfer;2", "elements");
    expectInvalid("SuperelementElemTransfer;-1;5", "superelementId");
}

TEST_CASE("CSUPEXT lists exterior points and refuses an empty list", "[model][superelements]")
{
    Model model = readBulk(
        "Node;153;;0.;0.;0.\nNode;574;;1.;0.;0.\nSuperelementExteriorPoints;2;153;574;941\nSuperelementExteriorPoints;"
        "5;13006;Through;13050");
    REQUIRE(cardAt<SuperelementExteriorPointsTable>(model, 0).points ==
            std::vector<IdRun>{run(153, 153), run(574, 574), run(941, 941)});
    REQUIRE(cardAt<SuperelementExteriorPointsTable>(model, 1).points == std::vector<IdRun>{run(13006, 13050)});
    REQUIRE(unresolvedIn(model) == 1); // the point 941; the ends of the range may not exist
    expectInvalid("SuperelementExteriorPoints;2", "nodes");
}

// --- SEBULK, CSUPER, SEQSEP, SETREE --------------------------------------------------------------------------------

TEST_CASE("SEBULK reads a repeated, an external and a primary superelement and resolves its defaults",
          "[model][superelements]")
{
    const Model model = readBulk(
        "SuperelementBulkOptions;1400;Repeat;40;Automatic;0.0025\nSuperelementBulkOptions;2000;ExternalReducedModel;;"
        "Manual;;;"
        "31\nSuperelementBulkOptions;1;Primary");
    const SuperelementBulkOptionsCard repeat = cardAt<SuperelementBulkOptionsTable>(model, 0);
    REQUIRE(repeat.superelementId == 1400);
    REQUIRE(repeat.partType == SuperelementBulkOptionsType::Repeat);
    REQUIRE(repeat.referenceSuperelementId == 40);
    REQUIRE(repeat.method == SuperelementBulkOptionsMethod::Auto);
    REQUIRE(repeat.tolerance == 2.5e-3);
    REQUIRE(repeat.location == SuperelementBulkOptionsLocation::Yes);
    const SuperelementBulkOptionsCard smart = cardAt<SuperelementBulkOptionsTable>(model, 1);
    REQUIRE(smart.partType == SuperelementBulkOptionsType::ExternalReducedModel);
    REQUIRE(smart.method == SuperelementBulkOptionsMethod::Manual);
    REQUIRE(smart.unitNumber == 31);
    // A TOL of 0.0 is the default: it means the value of PARAM,CONFAC.
    REQUIRE(cardAt<SuperelementBulkOptionsTable>(model, 2).tolerance == 0.0);
    requireRoundTrip(model);
}

TEST_CASE("SEBULK knows every type and refuses the others", "[model][superelements]")
{
    const char* const typeWords[] = {"Primary",
                                     "CollectorSuperelement",
                                     "External",
                                     "ExternalBinaryResults",
                                     "ExternalMatrixFile",
                                     "ExternalPunchFile",
                                     "Repeat",
                                     "Mirror",
                                     "ExternalReducedModel"};
    for (std::size_t index = 0; index < 9; ++index)
    {
        const Model model = readBulk(std::string("SuperelementBulkOptions;3;") + typeWords[index]);
        REQUIRE(static_cast<std::size_t>(cardAt<SuperelementBulkOptionsTable>(model).partType) == index);
    }
    expectInvalid("SuperelementBulkOptions;3", "partType");
    expectInvalid("SuperelementBulkOptions;3;SECONDARY", "partType");
    expectInvalid("SuperelementBulkOptions;3;Primary;;SEMI", "method");
    expectInvalid("SuperelementBulkOptions;3;Primary;;;-1.", "tolerance");
    expectInvalid("SuperelementBulkOptions;3;Primary;;;;MAYBE", "location");
    expectInvalid("SuperelementBulkOptions;-3;Primary", "superelementId");
    // SEID 0 is valid for the residual structure.
    REQUIRE(
        cardAt<SuperelementBulkOptionsTable>(readBulk("SuperelementBulkOptions;0;Primary;;;1.E-4")).superelementId ==
        0);
}

TEST_CASE("SEBULK names its reference superelement", "[model][superelements]")
{
    Model model = readBulk("SuperelementInteriorNodes;40;1\nSuperelementBulkOptions;1400;Repeat;40");
    const ScopedRecorder recorder = referencesOf<SuperelementBulkOptionsTable>(model);
    REQUIRE(recorder.of("referenceSuperelementId") == std::vector<std::pair<base::EntityId, base::EntityId>>{{40, -1}});
    REQUIRE(unresolvedIn(model) == 0); // SESET defines superelement 40
    Model missing = readBulk("SuperelementBulkOptions;1400;Repeat;90");
    REQUIRE(unresolvedIn(missing) == 1);
}

TEST_CASE("CSUPER codes the mirror in the number of the superelement", "[model][superelements]")
{
    Model model = readBulk(
        "SuperelementCopyPoints;120004;23;5;8;2;11\nSuperelementCopyPoints;7;-23;1;2\nSuperelementCopyPoints;5;;1;2");
    const SuperelementCopyPointsCard mirror = cardAt<SuperelementCopyPointsTable>(model, 0);
    REQUIRE(mirror.superelementId == 120004);
    REQUIRE(mirror.number() == 4);
    REQUIRE(mirror.reversalCode() == 12);
    REQUIRE(mirror.primarySuperelementId == 23);
    REQUIRE(mirror.points == std::vector<base::EntityId>{5, 8, 2, 11});
    REQUIRE(cardAt<SuperelementCopyPointsTable>(model, 1).primarySuperelementId == -23); // a z-reversal mirror
    REQUIRE(cardAt<SuperelementCopyPointsTable>(model, 2).primarySuperelementId ==
            0); // an external superelement has no primary
    // The ID of the card is the number: CSUPER defines the superelement.
    REQUIRE(model.findTable<SuperelementCopyPointsTable>()->idAtRow(0) == 4);
    // A renumbering changes the number and keeps the code, and the sign of the primary.
    model.table<SuperelementCopyPointsTable>().setIdAtRow(0, 9);
    REQUIRE(cardAt<SuperelementCopyPointsTable>(model, 0).superelementId == 120009);
    REQUIRE_THROWS_AS(model.table<SuperelementCopyPointsTable>().setIdAtRow(0, 10000), base::UsageError);
    requireRoundTrip(model);
}

TEST_CASE("CSUPER refuses a bad number, a bad code and an empty list", "[model][superelements]")
{
    expectInvalid("SuperelementCopyPoints;120000;21;3", "superelementId"); // the number is 0
    expectInvalid("SuperelementCopyPoints;450003;21;3", "superelementId"); // 45 is not a code of the format reference
    expectInvalid("SuperelementCopyPoints;3;21", "nodes");
    expectInvalid("SuperelementCopyPoints;3;21;0", "node");
    expectInvalid("SuperelementCopyPoints;-3;21;1", "superelementId");
}

TEST_CASE("CSUPER renumbers its primary with its sign", "[model][superelements]")
{
    Model model = readBulk("SuperelementInteriorNodes;23;1\nSuperelementCopyPoints;7;-23;1;2");
    Renumbering::apply(model, RenumberSpace::Superelement, Renumbering::offset(100));
    REQUIRE(cardAt<SuperelementCopyPointsTable>(model).primarySuperelementId == -123);
    REQUIRE(cardAt<SuperelementCopyPointsTable>(model).superelementId == 107);
}

TEST_CASE("SEQSEP names the primary superelement and its exterior points", "[model][superelements]")
{
    Model model = readBulk("SuperelementCopySequence;125;23;108;116;127;133");
    const SuperelementCopySequenceCard card = cardAt<SuperelementCopySequenceTable>(model);
    REQUIRE(card.superelementId == 125);
    REQUIRE(card.primarySuperelementId == 23);
    REQUIRE(card.points == std::vector<base::EntityId>{108, 116, 127, 133});
    const ScopedRecorder recorder = referencesOf<SuperelementCopySequenceTable>(model);
    REQUIRE(recorder.of("primarySuperelementId") == std::vector<std::pair<base::EntityId, base::EntityId>>{{23, -1}});
    REQUIRE(recorder.of("node").size() == 4);
    REQUIRE(recorder.of("node").front().second == 23); // the points are those of the primary superelement
    expectInvalid("SuperelementCopySequence;125;23", "nodes");
    expectInvalid("SuperelementCopySequence;125", "primarySuperelementId");
}

TEST_CASE("SETREE lists the superelements upstream of one", "[model][superelements]")
{
    Model model = readBulk(
        "SuperelementInteriorNodes;15;1\nSuperelementInteriorNodes;25;2\nSuperelementInteriorNodes;35;"
        "3\nSuperelementInteriorNodes;45;4\nSuperelementInteriorNodes;350;5\nSuperelementTree;350;15;25;35;"
        "45\nSuperelementTree;0;350");
    REQUIRE(cardAt<SuperelementTreeTable>(model, 0).upstream == std::vector<base::EntityId>{15, 25, 35, 45});
    REQUIRE(cardAt<SuperelementTreeTable>(model, 1).superelementId ==
            0); // the residual structure is downstream of the others
    REQUIRE(unresolvedIn(model) == 0);
    expectInvalid("SuperelementTree;350", "upstreamSuperelementId1");
    expectInvalid("SuperelementTree;350;0", "upstreamSuperelementId");
    expectInvalid("SuperelementTree;-1;15", "superelementId");
}

// --- the connection of two superelements -----------------------------------------------------------------------------

TEST_CASE("SECONCT reads its connections from the first continuation line", "[model][superelements]")
{
    const Model model = readBulk(
        "SuperelementConnection;12;24;2.E-4;Yes;1011;4011;2232;4454\nSuperelementConnection;10;20;;;101;Through;110;"
        "201;"
        "Through;210");
    const SuperelementConnectionCard pairs = cardAt<SuperelementConnectionTable>(model, 0);
    REQUIRE(pairs.superelementIdA == 12);
    REQUIRE(pairs.superelementIdB == 24);
    REQUIRE(pairs.tolerance == 2.0e-4);
    REQUIRE(pairs.location == SuperelementBulkOptionsLocation::Yes);
    REQUIRE(pairs.pairs == std::vector<SuperelementConnectionPair>{{run(1011, 1011), run(4011, 4011)},
                                                                   {run(2232, 2232), run(4454, 4454)}});
    const SuperelementConnectionCard ranges = cardAt<SuperelementConnectionTable>(model, 1);
    REQUIRE(ranges.pairs == std::vector<SuperelementConnectionPair>{{run(101, 110), run(201, 210)}});
    REQUIRE(ranges.tolerance == 0.0);
    requireRoundTrip(model);
}

TEST_CASE("SECONCT keeps a THRU out of the first field of a line when it writes", "[model][superelements]")
{
    // Seven single connections fill the first line (field 9 to 16) and the second one up to field 22; the range that
    // follows would put its first THRU in field 24 or its second in the first field of the next line.
    std::string deck =
        "SuperelementConnection;1;2;;;1;101;2;102;3;103;4;104;5;105;6;106;7;107;8;108;9;109;10;110;11;Through;15;111;"
        "Through;"
        "115";
    const Model model = readBulk(deck);
    REQUIRE(cardAt<SuperelementConnectionTable>(model).pairs.back() ==
            SuperelementConnectionPair{run(11, 15), run(111, 115)});

    // No line of the text starts with THRU (the lexer would read it as a continuation without a field).
    requireRoundTrip(model);
}

TEST_CASE("SECONCT refuses ranges of different lengths and cards without a connection", "[model][superelements]")
{
    expectInvalid("SuperelementConnection;1;2;;;101;Through;110;201;Through;209", "nodeIdB");
    expectInvalid("SuperelementConnection;1;2;;;101;Through;110;201", "nodeIdB");
    expectInvalid("SuperelementConnection;1;2", "nodeIdA1");
    expectInvalid("SuperelementConnection;1;2;-1.;;1;2", "tolerance");
    expectInvalid("SuperelementConnection;1;2;;MAYBE;1;2", "location");
    expectInvalid("SuperelementConnection;1;2;;;101;Through", "nodeIdA");
    expectInvalid("SuperelementConnection;1;2;;;101", "nodeIdB");
}

TEST_CASE("the points of an SECONCT go to the model of each superelement", "[model][superelements]")
{
    Model model = readBulk("SuperelementConnection;12;0;;;1011;4011;2232;4454");
    const ScopedRecorder recorder = referencesOf<SuperelementConnectionTable>(model);
    REQUIRE(recorder.of("superelementIdA") == std::vector<std::pair<base::EntityId, base::EntityId>>{{12, -1}});
    REQUIRE(recorder.of("nodeIdA") == std::vector<std::pair<base::EntityId, base::EntityId>>{{1011, 12}, {2232, 12}});
    // SEIDB is 0: the points of that side are those of the residual structure.
    REQUIRE(recorder.of("nodeIdB") == std::vector<std::pair<base::EntityId, base::EntityId>>{{4011, 0}, {4454, 0}});
}

TEST_CASE("SEBNDRY and SEEXCLD take ALL for the other superelement", "[model][superelements]")
{
    const Model model = readBulk(
        "SuperelementBoundarySearch;350;6;11;21;31;41\n"
        "SuperelementBoundarySearch;350;All;11;21;31;Through;41\n"
        "SuperelementBoundarySearch;350;;10\n"
        "SuperelementExcludedNodes;120;10;47;682;394\n"
        "SuperelementExcludedNodes;350;All;11;21;31;Through;41");
    REQUIRE(cardAt<SuperelementBoundarySearchTable>(model, 0).superelementIdB == std::optional<base::EntityId>(6));
    REQUIRE_FALSE(cardAt<SuperelementBoundarySearchTable>(model, 1).superelementIdB.has_value());
    REQUIRE_FALSE(cardAt<SuperelementBoundarySearchTable>(model, 2)
                      .superelementIdB.has_value()); // a blank SEIDB is ALL (the default)
    REQUIRE(cardAt<SuperelementBoundarySearchTable>(model, 1).grids ==
            std::vector<IdRun>{run(11, 11), run(21, 21), run(31, 41)});
    REQUIRE(cardAt<SuperelementExcludedNodesTable>(model, 0).superelementIdA == 120);
    REQUIRE(cardAt<SuperelementExcludedNodesTable>(model, 0).superelementIdB == std::optional<base::EntityId>(10));
    REQUIRE_FALSE(cardAt<SuperelementExcludedNodesTable>(model, 1).superelementIdB.has_value());
    // A blank SEIDB is written blank and an explicit ALL stays readable as ALL.
    requireRoundTrip(model);
    const std::string text = writeModelText(model, false);
    REQUIRE(text.find("All") != std::string::npos);
    expectInvalid("SuperelementBoundarySearch;350;6", "boundaryNodeIds");
    expectInvalid("SuperelementBoundarySearch;0;4;1", "superelementIdA");
    expectInvalid("SuperelementBoundarySearch;350;0;1", "superelementIdB");
    expectInvalid("SuperelementExcludedNodes;350;SOME;1", "superelementIdB");
}

TEST_CASE("the points of SEBNDRY and SEEXCLD are those of SEIDA", "[model][superelements]")
{
    Model model = readBulk("SuperelementBoundarySearch;350;6;10;Through;12\nSuperelementExcludedNodes;120;All;47");
    const ScopedRecorder boundary = referencesOf<SuperelementBoundarySearchTable>(model);
    REQUIRE(boundary.of("superelementIdA") == std::vector<std::pair<base::EntityId, base::EntityId>>{{350, -1}});
    REQUIRE(boundary.of("superelementIdB") == std::vector<std::pair<base::EntityId, base::EntityId>>{{6, -1}});
    REQUIRE(boundary.of("boundaryNodeIds") ==
            std::vector<std::pair<base::EntityId, base::EntityId>>{{10, 350}, {12, 350}});
    const ScopedRecorder excluded = referencesOf<SuperelementExcludedNodesTable>(model);
    REQUIRE(excluded.of("seidb").empty()); // ALL is not a superelement
    REQUIRE(excluded.of("excludedNodeIds") == std::vector<std::pair<base::EntityId, base::EntityId>>{{47, 120}});
}

// --- labels, locations, planes -------------------------------------------------------------------------------------

TEST_CASE("SELABEL keeps a label of several words as it was written", "[model][superelements]")
{
    const Model model = readBulk(
        "SuperelementLabel;1;Fuselage\nSuperelementLabel;12;Mirrored Outer Flap Panel\nSuperelementLabel;15;A  B   "
        "C\nSuperelementLabel;3");
    REQUIRE(cardAt<SuperelementLabelTable>(model, 0).entryLabel == "Fuselage");
    REQUIRE(cardAt<SuperelementLabelTable>(model, 1).entryLabel == "Mirrored Outer Flap Panel");
    REQUIRE(cardAt<SuperelementLabelTable>(model, 2).entryLabel == "A  B   C");
    REQUIRE(cardAt<SuperelementLabelTable>(model, 3).entryLabel.empty());
    requireRoundTrip(model);
    // The small format starts the label in field 3 and lets it run over the fields it needs.
}

TEST_CASE("SELOC aligns three points of a superelement to three of the main Bulk Data", "[model][superelements]")
{
    Model model = readBulk("SuperelementLocation;120;14;104;115;1020;116;34");
    const SuperelementLocationCard card = cardAt<SuperelementLocationTable>(model);
    REQUIRE(card.superelementId == 120);
    REQUIRE(card.pointsA == std::array<base::EntityId, 3>{14, 104, 115});
    REQUIRE(card.pointsB == std::array<base::EntityId, 3>{1020, 116, 34});
    const ScopedRecorder recorder = referencesOf<SuperelementLocationTable>(model);
    REQUIRE(recorder.of("pointA1") ==
            std::vector<std::pair<base::EntityId, base::EntityId>>{{14, 120}, {104, 120}, {115, 120}});
    REQUIRE(recorder.of("pointB1") ==
            std::vector<std::pair<base::EntityId, base::EntityId>>{{1020, -1}, {116, -1}, {34, -1}});
    expectInvalid("SuperelementLocation;120;14;104;115;1020;116", "pointB3");
    expectInvalid("SuperelementLocation;120;14;14;115;1020;116;34",
                  "pointA1"); // three noncolinear points are three different ones
    expectInvalid("SuperelementLocation;120;14;104;115;1020;116;1020", "pointB1");
}

TEST_CASE("SEMPLN needs the word PLANE and three different points", "[model][superelements]")
{
    Model model = readBulk("SuperelementMirrorPlane;120;Plane;13;47;1135\nSuperelementMirrorPlane;5;Plane;1;2;3");
    REQUIRE(cardAt<SuperelementMirrorPlaneTable>(model).points == std::array<base::EntityId, 3>{13, 47, 1135});
    oor::testing::requireModelFileRoundTrip(model);
    const ScopedRecorder recorder = referencesOf<SuperelementMirrorPlaneTable>(model);
    REQUIRE(recorder.of("point").size() == 3);
    REQUIRE(recorder.of("point").front().second == -1); // the points are those of the main Bulk Data
    expectInvalid("SuperelementMirrorPlane;120;LINE;13;47;1135", "plane");
    expectInvalid("SuperelementMirrorPlane;120;;13;47;1135", "plane");
    expectInvalid("SuperelementMirrorPlane;120;Plane;13;47", "point3");
    expectInvalid("SuperelementMirrorPlane;120;Plane;13;13;1135", "point");
}

TEST_CASE("SENQSET takes a superelement or ALL", "[model][superelements]")
{
    const Model model = readBulk(
        "SuperelementGeneralizedPointCount;120;47\nSuperelementGeneralizedPointCount;All;"
        "10\nSuperelementGeneralizedPointCount;7");
    REQUIRE(cardAt<SuperelementGeneralizedPointCountTable>(model, 0).superelementId ==
            std::optional<base::EntityId>(120));
    REQUIRE(cardAt<SuperelementGeneralizedPointCountTable>(model, 0).count == 47);
    REQUIRE_FALSE(cardAt<SuperelementGeneralizedPointCountTable>(model, 1).superelementId.has_value());
    REQUIRE(cardAt<SuperelementGeneralizedPointCountTable>(model, 2).count == 0); // the default
    requireRoundTrip(model);
    expectInvalid("SuperelementGeneralizedPointCount;;5", "superelementId");
    expectInvalid("SuperelementGeneralizedPointCount;120;-1", "count");
}

// --- RELEASE, EXTRN and the sets of degrees of freedom
// -------------------------------------------------------------------

TEST_CASE("RELEASE reads points, a range and ALL", "[model][superelements]")
{
    const Model model = readBulk(
        "SuperelementReleasedDofs;16;456;4;8;13;2;158;9;154;164\nSuperelementReleasedDofs;6;2;15;Through;"
        "127\nSuperelementReleasedDofs;127;156;All");
    REQUIRE(cardAt<SuperelementReleasedDofsTable>(model, 0).components == 456);
    REQUIRE(cardAt<SuperelementReleasedDofsTable>(model, 0).points.size() == 8);
    REQUIRE(cardAt<SuperelementReleasedDofsTable>(model, 1).points == std::vector<IdRun>{run(15, 127)});
    REQUIRE(cardAt<SuperelementReleasedDofsTable>(model, 2).all);
    REQUIRE(cardAt<SuperelementReleasedDofsTable>(model, 2).points.empty());
    requireRoundTrip(model);
    expectInvalid("SuperelementReleasedDofs;0;456;3", "superelementId"); // never the residual structure
    expectInvalid("SuperelementReleasedDofs;15;0;3", "components");
    expectInvalid("SuperelementReleasedDofs;15;1177;3", "components");
    expectInvalid("SuperelementReleasedDofs;15;456", "node1");
    expectInvalid("SuperelementReleasedDofs;15;456;All;3", "node1"); // no continuation after ALL
}

TEST_CASE("EXTRN reads pairs, ranges and blank pairs and defines its points", "[model][superelements]")
{
    Model model = readBulk(
        "SuperelementExternalBoundary;1011;123;1130;123456;1211;123\n"
        "SuperelementExternalBoundary;1;0;20112101;Through;20112139;0;5;1\n"
        "SuperelementExternalBoundary;10;123;;;11;123456;12;123;13;123");
    const SuperelementExternalBoundaryCard first = cardAt<SuperelementExternalBoundaryTable>(model, 0);
    REQUIRE(first.items.size() == 3);
    REQUIRE(first.items[1].points == run(1130, 1130));
    REQUIRE(first.items[1].components == 123456);
    const SuperelementExternalBoundaryCard ranged = cardAt<SuperelementExternalBoundaryTable>(model, 1);
    REQUIRE(ranged.items.size() == 3);
    REQUIRE(ranged.items[1].points == run(20112101, 20112139));
    REQUIRE(ranged.items[1].components == 0);
    REQUIRE(ranged.items[2].points == run(5, 5));
    REQUIRE(ranged.items[2].components == 1);                                       // a scalar point may say 1
    REQUIRE(cardAt<SuperelementExternalBoundaryTable>(model, 2).items.size() == 4); // the blank pair is padding
    // The points of an EXTRN resolve in the part that holds it, which has no GRID for them.
    REQUIRE(unresolvedIn(model) == 0);
    const std::vector<IdRun> points = model.findTable<SuperelementExternalBoundaryTable>()->connectionPoints();
    REQUIRE(points.front() == run(1, 1));
    requireRoundTrip(model);
    expectInvalid("SuperelementExternalBoundary", "nodeId1");
    expectInvalid("SuperelementExternalBoundary;1001;7", "components");
    expectInvalid("SuperelementExternalBoundary;10;0;5;Through", "nodeId");
}

TEST_CASE("EXTRN leaves the last pair of a line blank rather than put a THRU there", "[model][superelements]")
{
    const Model model = readBulk("SuperelementExternalBoundary;1;123;2;123;3;123;4;Through;9;123");
    REQUIRE(cardAt<SuperelementExternalBoundaryTable>(model).items.back().points == run(4, 9));

    requireRoundTrip(model);
}

TEST_CASE("the DOF sets of a superelement read their pairs and their lists", "[model][superelements]")
{
    const Model model = readBulk(
        "SuperelementFixedDofSet;5;3;246;15;4\n"
        "SuperelementFixedDofSet1;5;3;6;Through;32\n"
        "SuperelementFreeDofSet;3;125;2;6;24;4;16\n"
        "SuperelementFreeDofSet1;5;3;6;Through;32\n"
        "SuperelementFreeDofSet1;5;All\n"
        "SuperelementGeneralizedDofSet;15;2;123456;8;5;24;3\n"
        "SuperelementGeneralizedDofSet1;16;0;101;Through;110\n"
        "SuperelementUserDofSet;15;U4;2;123456;8;5;24;3\n"
        "SuperelementUserDofSet1;15;u4;1;12;Through;27\n"
        "SuperelementSupport;5;18;325");
    REQUIRE(cardAt<SuperelementFixedDofSetTable>(model).superelementId == 5);
    REQUIRE(cardAt<SuperelementFixedDofSetTable>(model).dofs == std::vector<DofPair>{DofPair{3, 246}, DofPair{15, 4}});
    REQUIRE(cardAt<SuperelementFixedDofSet1Table>(model).components == 3);
    REQUIRE(cardAt<SuperelementFixedDofSet1Table>(model).points == std::vector<IdRun>{run(6, 32)});
    REQUIRE(cardAt<SuperelementFreeDofSetTable>(model).dofs.size() == 3);
    REQUIRE(cardAt<SuperelementFreeDofSet1Table>(model, 0).components == 3);
    REQUIRE(cardAt<SuperelementFreeDofSet1Table>(model, 1).all);
    REQUIRE(cardAt<SuperelementGeneralizedDofSetTable>(model).dofs.size() == 3);
    REQUIRE(cardAt<SuperelementGeneralizedDofSet1Table>(model).components == 0);
    REQUIRE(cardAt<SuperelementGeneralizedDofSet1Table>(model).points == std::vector<IdRun>{run(101, 110)});
    REQUIRE(cardAt<SuperelementUserDofSetTable>(model).setName == "U4");
    REQUIRE(cardAt<SuperelementUserDofSet1Table>(model).setName == "u4"); // exact input spelling
    REQUIRE(cardAt<SuperelementSupportTable>(model).dofs ==
            std::vector<DofPair>{DofPair{18, 235}}); // the digits are sorted
    requireRoundTrip(model);
}

TEST_CASE("the pairs of an SEBSET continue on the lines after three pairs", "[model][superelements]")
{
    // Three pairs fill fields 2 to 7; the next ones go four to a line from field 9.
    const Model model = readBulk("SuperelementFixedDofSet;5;1;1;2;1;3;1;4;1;5;1;6;1;7;1");
    REQUIRE(cardAt<SuperelementFixedDofSetTable>(model).dofs.size() == 7);
    // SEUSET has the set name in the first line, so it holds three pairs there as well and goes on from field 9.
    REQUIRE(cardAt<SuperelementUserDofSetTable>(readBulk("SuperelementUserDofSet;5;U1;1;1;2;1;3;1;4;1")).dofs.size() ==
            4);
    requireRoundTrip(model);
}

TEST_CASE("the DOF set cards refuse what a set cannot hold", "[model][superelements]")
{
    expectInvalid("SuperelementFixedDofSet;5", "node1");
    expectInvalid("SuperelementFixedDofSet;0;1;1", "superelementId");
    expectInvalid("SuperelementFixedDofSet;5;1;7", "components1");
    expectInvalid("SuperelementFixedDofSet1;5;1", "nodes");
    expectInvalid("SuperelementFixedDofSet1;5;7;1", "components");
    expectInvalid("SuperelementFreeDofSet1;5;All;3", "components");
    expectInvalid("SuperelementUserDofSet;5;;1;1", "setName");
    REQUIRE_NOTHROW(readBulk("SuperelementUserDofSet;5;TOOLONGNAME;1;1"));
    expectInvalid("SuperelementUserDofSet1;5;U1;1", "nodes");
    expectInvalid("SuperelementGeneralizedDofSet;5;0", "node1");
    expectInvalid("SuperelementSupport;5", "node1");
}

TEST_CASE("the points of the DOF set cards of a superelement are those of that superelement", "[model][superelements]")
{
    Model model = readBulk(
        "SuperelementFixedDofSet;5;3;246;15;4\nSuperelementFreeDofSet1;5;3;6;Through;32\nSuperelementUserDofSet;15;U4;"
        "1;"
        "123456");
    const ScopedRecorder pairs = referencesOf<SuperelementFixedDofSetTable>(model);
    REQUIRE(pairs.of("superelementId") == std::vector<std::pair<base::EntityId, base::EntityId>>{{5, -1}});
    REQUIRE(pairs.of("node") == std::vector<std::pair<base::EntityId, base::EntityId>>{{3, 5}, {15, 5}});
    const ScopedRecorder lists = referencesOf<SuperelementFreeDofSet1Table>(model);
    REQUIRE(lists.of("nodes") == std::vector<std::pair<base::EntityId, base::EntityId>>{{6, 5}, {32, 5}});
    for (const ScopedReference& reference : lists.seen)
    {
        if (reference.field == "nodes")
            REQUIRE(reference.target == ReferenceTarget::NodeInRange); // the points may not exist
    }
    REQUIRE(referencesOf<SuperelementUserDofSetTable>(model).of("node").front().second == 15);
}

TEST_CASE("BNDFIX and BNDFREE are the BSET and CSET cards under other names", "[model][superelements]")
{
    Model model = readBulk(
        "Node;3;;0.;0.;0.\n"
        "Node;15;;1.;0.;0.\n"
        "FixedBoundaryDofs;3;246;15;4\n"
        "FixedBoundaryDofs1;3;137;15;Through;34\n"
        "FreeBoundaryDofs;126;2;5;24;7;13\n"
        "FreeBoundaryDofs1;125;2;5;8;6;9\n"
        "FreeBoundaryDofs1;All\n"
        "FreeBoundaryDofs;1;123456");
    REQUIRE(FixedBoundaryDofsTable::INFO.category == CardCategory::Set);
    REQUIRE(FixedBoundaryDofsTable::INFO.idSpace == base::IdSpace::None);
    REQUIRE(cardAt<FixedBoundaryDofsTable>(model).dofs == std::vector<DofPair>{DofPair{3, 246}, DofPair{15, 4}});
    REQUIRE(cardAt<FixedBoundaryDofs1Table>(model).components == 3);
    REQUIRE(cardAt<FixedBoundaryDofs1Table>(model).nodes == std::vector<IdRun>{run(137, 137), run(15, 34)});
    REQUIRE(cardAt<FreeBoundaryDofsTable>(model, 0).dofs.size() == 3);
    REQUIRE(cardAt<FreeBoundaryDofs1Table>(model, 0).components == 125);
    REQUIRE(cardAt<FreeBoundaryDofs1Table>(model, 1).all);
    REQUIRE(cardAt<FreeBoundaryDofsTable>(model, 1).dofs == std::vector<DofPair>{DofPair{1, 123456}});
    requireRoundTrip(model);
    // The points are those of this model: nothing goes to another superelement.
    REQUIRE(referencesOf<FixedBoundaryDofsTable>(model).seen.front().scoped == false);
    expectInvalid("FixedBoundaryDofs", "node1");
    expectInvalid("FixedBoundaryDofs1;2", "nodes");
    expectInvalid("FreeBoundaryDofs1;All;3", "components");
    expectInvalid("FreeBoundaryDofs1;125", "nodes");
}

// --- MASSSET, SELOAD, the conical shell cards, the design cards
// ----------------------------------------------------------

TEST_CASE("MASSSET combines the mass cases of a deck", "[model][superelements]")
{
    Model model = readBulk(
        "MassCaseCombination;11;1.;1.;0;1.;105\n"
        "MassCaseCombination;33;1.;1.;0;1.;300\n"
        "MassCaseCombination;12;;;0;2.;300;3.;301;4.;302");
    const MassCaseCombinationCard card = cardAt<MassCaseCombinationTable>(model, 0);
    REQUIRE(card.id == 11);
    REQUIRE(card.overallScale == 1.0);
    REQUIRE(card.terms == std::vector<MassCaseCombinationTerm>{{1.0, 0}, {1.0, 105}});
    REQUIRE(cardAt<MassCaseCombinationTable>(model, 2).overallScale == 0.0);
    REQUIRE(cardAt<MassCaseCombinationTable>(model, 2).terms.size() == 4);
    // The mass case 0 is the base case: only the others are references, to the parts that carry them.
    const ScopedRecorder recorder = referencesOf<MassCaseCombinationTable>(model);
    REQUIRE(recorder.seen.size() == 1);
    REQUIRE(recorder.seen[0].target == ReferenceTarget::MassIncrement);
    REQUIRE(recorder.seen[0].id == 105);
    requireRoundTrip(model);
    expectInvalid("MassCaseCombination;0;1.", "id");
    expectInvalid("MassCaseCombination;11;-1.", "overallScale");
    expectInvalid("MassCaseCombination;11;1.;-1.;0", "caseScales");
    expectInvalid("MassCaseCombination;11;1.;1.", "caseIds");
}

TEST_CASE("a MASSSET names mass increments that parts carry", "[model][superelements]")
{
    Model model = readModelText(
        "[model]\n"
        "MassCaseCombination;33;1.;1.;0;1.;300\n"
        "[model;SuperelementPart;1;massId=300]\n"
        "Node;1;;0.;0.;0.\n",
        true);
    REQUIRE(unresolvedIn(model) == 0);
    Model without = readBulk("MassCaseCombination;33;1.;1.;0;1.;300");
    REQUIRE(unresolvedIn(without) == 1);
    Model standalone = readModelText(
        "[model]\nMassCaseCombination;33;1.;1.;0;1.;300\n[model;MassModel;300]\nNode;1;;0.;0.;0.\n", true);
    REQUIRE(unresolvedIn(standalone) == 0);
}

TEST_CASE("SELOAD reads a scale for each superelement, by label or by subcase", "[model][superelements]")
{
    // SID S, then two lines for each superelement: SEID, S, LBLTYP; and the label or the number.
    const Model model = readBulk("SuperelementLoadScale;1100;1.5;410;0.75;;wing gust;420;1.25;SubcaseId;30");
    const SuperelementLoadScaleCard card = cardAt<SuperelementLoadScaleTable>(model);
    REQUIRE(card.id == 1100);
    REQUIRE(card.scale == 1.5);
    REQUIRE(card.entries.size() == 2);
    REQUIRE(card.entries[0].superelementId == 410);
    REQUIRE(card.entries[0].scale == 0.75);
    REQUIRE(card.entries[0].selector == SuperelementLoadScaleSelector::Label);
    REQUIRE(card.entries[0].label == "wing gust");
    REQUIRE(card.entries[1].superelementId == 420);
    REQUIRE(card.entries[1].selector == SuperelementLoadScaleSelector::SubcaseId);
    REQUIRE(card.entries[1].number == 30);
    REQUIRE(SuperelementLoadScaleTable::INFO.category == CardCategory::Load); // SID is the set a LOAD command selects
    requireRoundTrip(model);
}

TEST_CASE("SELOAD knows the selectors of the format and their synonyms", "[model][superelements]")
{
    const Model model =
        readBulk("SuperelementLoadScale;12;;1;;LoadId;7;2;;ColumnNumber;3;3;2.;Column;4;4;;Subcase;LC01;5;;;Plain");
    const SuperelementLoadScaleCard card = cardAt<SuperelementLoadScaleTable>(model);
    REQUIRE(card.scale == 1.0); // the format reference gives no default: a blank scale leaves the vector as it is
    REQUIRE(card.entries.size() == 5);
    REQUIRE(card.entries[0].selector == SuperelementLoadScaleSelector::LoadId);
    REQUIRE(card.entries[1].selector == SuperelementLoadScaleSelector::ColumnIndex);
    REQUIRE(card.entries[2].selector == SuperelementLoadScaleSelector::ColumnIndex);
    REQUIRE(card.entries[2].scale == 2.0);
    REQUIRE(card.entries[3].selector == SuperelementLoadScaleSelector::Label);
    REQUIRE(card.entries[3].label == "LC01");
    REQUIRE(card.entries[4].label == "Plain");
    requireRoundTrip(model);
    expectInvalid("SuperelementLoadScale;12;;1;;BADWORD;7", "labelKind");
    expectInvalid("SuperelementLoadScale;12;;1;;LoadId", "labelledId");
    expectInvalid("SuperelementLoadScale;12;;1;;Label", "entryLabel");
    expectInvalid("SuperelementLoadScale;12", "superelementId1");
    expectInvalid("SuperelementLoadScale;0;;1;;;x", "id");
}

TEST_CASE("the superelements of an SELOAD are references and its SID a load set", "[model][superelements]")
{
    Model model = readBulk(
        "SuperelementInteriorNodes;410;1\nSuperelementLoadScale;1100;1.5;410;0.75;;wing gust;420;1.25;SubcaseId;30");
    const ScopedRecorder recorder = referencesOf<SuperelementLoadScaleTable>(model);
    REQUIRE(recorder.of("superelementId") ==
            std::vector<std::pair<base::EntityId, base::EntityId>>{{410, -1}, {420, -1}});
    REQUIRE(unresolvedIn(model) == 1); // superelement 420 is defined by nothing
    // The set is found by the commands and the loads that name it.
    REQUIRE(setIdsOfCategory(model, CardCategory::Load).contains(1100));
}

TEST_CASE("SECTAX and SUPAX read the cards of the conical shell", "[model][superelements]")
{
    const Model model = readBulk(
        "AxisymConeSector;1;2;2.5;25.;35.\nAxisymConeSupport;5;2;4\nAxisymConeSupport;4;0;12;5;1;3;6;1;3;2;7;1");
    const AxisymConeSectorCard sector = cardAt<AxisymConeSectorTable>(model);
    REQUIRE(sector.id == 1);
    REQUIRE(sector.ringId == 2);
    REQUIRE(sector.radius == 2.5);
    REQUIRE(sector.phiAngle2 == 35.0);
    REQUIRE(AxisymConeSectorTable::INFO.idSpace == base::IdSpace::PerCardType);
    const AxisymConeSupportCard single = cardAt<AxisymConeSupportTable>(model, 0);
    REQUIRE(single.dofs == std::vector<AxisymConeSupportDof>{{5, 2, 4}});
    REQUIRE(cardAt<AxisymConeSupportTable>(model, 1).dofs.size() == 4);
    REQUIRE(cardAt<AxisymConeSupportTable>(model, 1).dofs[0].harmonic == 0); // the first harmonic
    requireRoundTrip(model);
    expectInvalid("AxisymConeSector;1;2;3.;30.", "phiAngle2");
    expectInvalid("AxisymConeSector;0;2;3.;30.;40.", "id");
    expectInvalid("AxisymConeSupport", "ringId1");
    expectInvalid("AxisymConeSupport;4;3", "components");
    expectInvalid("AxisymConeSupport;4;-1;2", "harmonicId");
    expectInvalid("AxisymConeSupport;4;3;7", "components");
}

TEST_CASE("SEDLINK reads its terms and looks its variables up in their parts", "[model][superelements]")
{
    Model model = readBulk("SuperelementDesignLink;40;30;5;0.25;0.5;30;7;-1.5;12;9;-0.5;25;7;3.5;12;4;1.25");
    const SuperelementDesignLinkCard card = cardAt<SuperelementDesignLinkTable>(model);
    REQUIRE(card.id == 40);
    REQUIRE(card.dependentSuperelementId == 30);
    REQUIRE(card.dependentVarId == 5);
    REQUIRE(card.constant0 == 0.25);
    REQUIRE(card.multiplier == 0.5);
    REQUIRE(card.terms ==
            std::vector<SuperelementDesignLinkTerm>{{30, 7, -1.5}, {12, 9, -0.5}, {25, 7, 3.5}, {12, 4, 1.25}});
    const ScopedRecorder recorder = referencesOf<SuperelementDesignLinkTable>(model);
    REQUIRE(recorder.of("dependentVarId") == std::vector<std::pair<base::EntityId, base::EntityId>>{{5, 30}});
    REQUIRE(recorder.of("independentVarIds").size() == 4);
    REQUIRE(recorder.of("independentVarIds")[2] == std::pair<base::EntityId, base::EntityId>{7, 25});
    requireRoundTrip(model);
    expectInvalid("SuperelementDesignLink;40;30;5;0.25;0.5", "independentSuperelementIds");
    expectInvalid("SuperelementDesignLink;10;0;2;;;8;8;1.", "dependentSuperelementId");
    expectInvalid("SuperelementDesignLink;10;10;2;;;8;8", "coefficients");
}

TEST_CASE("SEDRSP2 and SEDRSP3 read groups led by the part superelement of each item", "[model][superelements]")
{
    const Model model = readBulk(
        "SuperelementDesignResp2;1;RBUCK;5;;3;;;;;DesignVar;4;0;101;1;3;1;4;10;201;DesignConstants;2;1;YM;10;L;"
        "DesignResp1;3;0;14;1;1;1;"
        "4;DesignNodes;1;10;14;1;DesignPropRelation1;2;0;101;1;102;DesignElemRelation1;2;1;201;10;202;"
        "DesignMatRelation1;1;0;301;DesignPropRelation2;2;10;401;1;402;DesignElemRelation2;"
        "1;0;501;DesignMatRelation2;3;0;601;1;602;10;603\n"
        "SuperelementDesignResp3;10;RBUCK;FINGRP;BUCK;;DesignVar;4;0;101;1;3;1;4;10;201;DesignConstants;2;1;YM;10;L;"
        "UserData;1;"
        "Constants: 24680\n");
    const SuperelementDesignResp2Card equation = cardAt<SuperelementDesignResp2Table>(model);
    REQUIRE(equation.id == 1);
    REQUIRE(equation.entryLabel == "RBUCK");
    REQUIRE(equation.equationId == 5);
    REQUIRE(equation.regionId == 3);
    REQUIRE(equation.constant1 == 100.0); // the defaults of this format
    REQUIRE(equation.constant2 == 0.005);
    REQUIRE(equation.groups.size() == 10);
    REQUIRE(equation.groups[0].keyword == "DesignVar");
    REQUIRE(equation.groups[0].items.size() == 4);
    REQUIRE(equation.groups[0].items[3].superelementId == 10);
    REQUIRE(equation.groups[0].items[3].id == 201);
    REQUIRE(equation.groups[1].items[1].label == "L");
    REQUIRE(equation.groups[3].items[0].component == 1);
    const SuperelementDesignResp3Card external = cardAt<SuperelementDesignResp3Table>(model);
    REQUIRE(external.groupName == "FINGRP");
    REQUIRE(external.kind == "BUCK");
    REQUIRE(external.groups.back().keyword == "UserData");
    REQUIRE(external.groups.back().lines == std::vector<std::string>{"Constants: 24680"});
    requireRoundTrip(model);
}

TEST_CASE("SEDRSP2 takes a function for its equation and refuses a group it does not know", "[model][superelements]")
{
    const Model model = readBulk("SuperelementDesignResp2;2;TOTAL;;Sum;;;;;;DesignResp1;2;1;1;2;2");
    REQUIRE(cardAt<SuperelementDesignResp2Table>(model).function == "Sum");
    REQUIRE(cardAt<SuperelementDesignResp2Table>(model).equationId == 0);
    expectInvalid("SuperelementDesignResp2;2;TOTAL;;AVERAGE;;;;;;DesignResp1;1;1;1", "function");
    expectInvalid("SuperelementDesignResp2;2;TOTAL;;;;;;;;DesignResp1;1;1;1", "equationId");
    expectInvalid("SuperelementDesignResp2;2;TOTAL;5;;;;;;;DesignVar;1;1", "designVarIds");
    expectInvalid("SuperelementDesignResp2;2;TOTAL;5;;;;;;;DesignNodes;1;1;14;4", "nodeComponents");
    expectInvalid("SuperelementDesignResp3;2;LBL;GROUP;;;DesignVar;1;0;1", "kind");
    requireRoundTrip(readBulk("SuperelementDesignResp3;2;LBL;GROUP;TYPE;;UserData;1;" + std::string(600, 'x')));
}

TEST_CASE("the items of an SEDRSP2 go to the model of their part superelement", "[model][superelements]")
{
    Model model = readBulk(
        "SuperelementDesignResp2;1;LBUCK;5;;;;;;;DesignVar;2;0;101;1;3;DesignConstants;1;1;YM;DesignResp1;1;2;14;"
        "DesignNodes;1;10;14;1;"
        "DesignPropRelation1;1;3;7");
    const ScopedRecorder recorder = referencesOf<SuperelementDesignResp2Table>(model);
    REQUIRE(recorder.of("equationId") == std::vector<std::pair<base::EntityId, base::EntityId>>{{5, -1}});
    REQUIRE(recorder.of("designVarIds") == std::vector<std::pair<base::EntityId, base::EntityId>>{{101, 0}, {3, 1}});
    REQUIRE(recorder.of("responseIds") == std::vector<std::pair<base::EntityId, base::EntityId>>{{14, 2}});
    REQUIRE(recorder.of("nodeIds") == std::vector<std::pair<base::EntityId, base::EntityId>>{{14, 10}});
    REQUIRE(recorder.of("propRelationIds") == std::vector<std::pair<base::EntityId, base::EntityId>>{{7, 3}});
    // The label of a DTABLE names no card, but the superelement before it is one.
    bool labelVisited = false;
    for (const ScopedReference& reference : recorder.seen)
        labelVisited = labelVisited || reference.field == "constantLabels";
    REQUIRE_FALSE(labelVisited);
}

// --- the superelement numbers
// --------------------------------------------------------------------------------------------

TEST_CASE("a superelement number is defined by a part, a card or the SEID of a grid point", "[model][superelements]")
{
    Model model = readModelText(
        "[model]\n"
        "Node;1;;0.;0.;0.;;;7\n"
        "SuperelementInteriorNodes;5;1\n"
        "SuperelementBulkOptions;6;Primary\n"
        "SuperelementExteriorPoints;8;1\n"
        "SuperelementCopyPoints;120009;5;1\n"
        "SuperelementLabel;10;Not defined\n"
        "SuperelementTree;5;6;7;8;9;3\n"
        "[model;SuperelementPart;3]\n"
        "Node;1;;0.;0.;0.\n",
        true);
    // 5 (SESET), 6 (SEBULK), 7 (the SEID of a GRID), 8 (CSUPEXT), 9 (CSUPER 120009) and 3 (a part) are defined; 10 is
    // not.
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    REQUIRE(ReferenceChecker::check(model, sink) == 1);
    REQUIRE(sink.diagnostics().front().location.fieldName == "superelementId");
    REQUIRE(sink.diagnostics().front().message.find("refers to 10") != std::string::npos);
}

TEST_CASE("a card of the family in a part resolves its superelement through the whole deck", "[model][superelements]")
{
    Model model = readModelText(
        "[model]\n"
        "SuperelementInteriorNodes;5;1\n"
        "[model;SuperelementPart;3]\n"
        "Node;1;;0.;0.;0.\n"
        "SuperelementLabel;5;Five\n"
        "SuperelementLabel;3;Three\n",
        true);
    // Superelement 5 is defined in the residual structure and 3 by this part: both exist for the part's cards.
    REQUIRE(unresolvedIn(*model.parts().front()) == 0);
}

TEST_CASE("the Case Control selects superelements by number or by a SET of numbers", "[model][superelements]")
{
    Model model = readModelText(
        "[executive]\n"
        "SolutionSequence;101\n"
        "[case]\n"
        "SetDefinition;100;0;2;3\n"
        "SuperelementAllSteps;100\n"
        "SuperelementDataRecovery;2\n"
        "SubcaseStart;1\n"
        "SuperelementSelection;7\n"
        "SubcaseStart;2\n"
        "SuperelementSelection;All\n"
        "[model]\n"
        "SuperelementInteriorNodes;2;1\n"
        "[model;SuperelementPart;3]\n"
        "Node;1;;0.;0.;0.\n",
        true);
    // 2 and 3 exist, the SET 100 is a SET (not a superelement) and 7 does not exist: one warning, on SUPER.
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    REQUIRE(ReferenceChecker::check(model, sink) == 1);
    REQUIRE(sink.diagnostics().front().code == "model.unresolved-case-control-reference");
    REQUIRE(sink.diagnostics().front().message.find("refers to 7") != std::string::npos);
}

TEST_CASE("the superelement numbers are renumbered with every card, grid point, command and part that holds one",
          "[model][superelements]")
{
    Model model = readModelText(
        "[executive]\n"
        "SolutionSequence;101\n"
        "[case]\n"
        "SuperelementAllSteps;2\n"
        "SubcaseStart;1\n"
        "SuperelementSelection;3\n"
        "[model]\n"
        "Node;1;;0.;0.;0.;;;2\n"
        "SuperelementInteriorNodes;2;1\n"
        "SuperelementBulkOptions;3;Primary\n"
        "SuperelementBulkOptions;4;Repeat;3\n"
        "SuperelementLabel;3;Three\n"
        "SuperelementFixedDofSet1;3;123;1\n"
        "SuperelementTree;0;2;3\n"
        "SuperelementConnection;3;0;;;1;1\n"
        "[model;SuperelementPart;3]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;SuperelementPart;3;massId=300]\n"
        "Node;2;;0.;0.;0.\n",
        true);
    const std::size_t changed = Renumbering::apply(model, RenumberSpace::Superelement, Renumbering::offset(10));
    REQUIRE(changed > 0);
    REQUIRE(cardAt<SuperelementInteriorNodesTable>(model).superelementId == 12);
    REQUIRE(cardAt<SuperelementBulkOptionsTable>(model, 0).superelementId == 13);
    REQUIRE(cardAt<SuperelementBulkOptionsTable>(model, 1).superelementId == 14);
    REQUIRE(cardAt<SuperelementBulkOptionsTable>(model, 1).referenceSuperelementId == 13);
    REQUIRE(cardAt<SuperelementLabelTable>(model).superelementId == 13);
    REQUIRE(cardAt<SuperelementFixedDofSet1Table>(model).superelementId == 13);
    REQUIRE(cardAt<SuperelementTreeTable>(model).superelementId == 0);
    REQUIRE(cardAt<SuperelementTreeTable>(model).upstream == std::vector<base::EntityId>{12, 13});
    REQUIRE(cardAt<SuperelementConnectionTable>(model).superelementIdA == 13);
    REQUIRE(cardAt<SuperelementConnectionTable>(model).superelementIdB == 0);
    REQUIRE(model.findTable<NodeTable>()->superelementId(0) == 12);
    // The parts take the new number while the associated mass ID stays.
    REQUIRE(model.parts()[0]->partInfo()->id == 13);
    REQUIRE(model.parts()[1]->partInfo()->id == 13);
    REQUIRE(model.parts()[1]->partInfo()->massId == 300);
    // The Case Control selections follow.

    oor::testing::requireModelFileRoundTrip(model);
    // The deck still has no reference that names nothing.
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("renumbering the superelements is refused on a part and when two numbers would meet",
          "[model][superelements]")
{
    Model model = readModelText(
        "[model]\nSuperelementInteriorNodes;2;1\nSuperelementInteriorNodes;3;2\n[model;SuperelementPart;5]\nNode;1;;0.;"
        "0.;0.\n",
        true);
    REQUIRE_THROWS_AS(Renumbering::apply(*model.parts().front(), RenumberSpace::Superelement, Renumbering::offset(1)),
                      base::UsageError);
    // 2 -> 3 and 3 -> 3 would give one number to two superelements: nothing changes.
    REQUIRE_THROWS_AS(Renumbering::apply(model, RenumberSpace::Superelement,
                                         Renumbering::fromFunction(
                                             [](base::EntityId id)
                                             { return id == 2 ? std::optional<base::EntityId>(3) : std::nullopt; })),
                      base::DuplicateIdError);
    REQUIRE(cardAt<SuperelementInteriorNodesTable>(model, 0).superelementId == 2);
    REQUIRE(model.parts().front()->partInfo()->id == 5);
    // A consecutive renumbering numbers all of them in ascending order, the parts included.
    Renumbering::apply(model, RenumberSpace::Superelement,
                       Renumbering::consecutive(RenumberSpace::Superelement, 10, 10));
    REQUIRE(cardAt<SuperelementInteriorNodesTable>(model, 0).superelementId == 10);
    REQUIRE(cardAt<SuperelementInteriorNodesTable>(model, 1).superelementId == 20);
    REQUIRE(model.parts().front()->partInfo()->id == 30);
}

TEST_CASE("a REPEAT or MIRROR superelement has the points of the superelement it copies", "[model][superelements]")
{
    Model model = readModelText(
        "[model]\n"
        "Node;200;;0.;0.;0.\n"
        "Node;201;;1.;0.;0.\n"
        "Node;202;;1.;1.;0.\n"
        "SuperelementBulkOptions;2;Primary\n"
        "SuperelementBulkOptions;12;Mirror;2;Automatic\n"
        "SuperelementBulkOptions;13;Repeat;12\n"
        "SuperelementMirrorPlane;12;Plane;200;201;202\n"
        "SuperelementLocation;2;10;11;12;200;201;202\n"
        "SuperelementLocation;12;10;11;12;200;201;202\n"
        "SuperelementLocation;13;10;11;12;200;201;202\n"
        "SuperelementConnection;12;0;;;10;200\n"
        "[model;SuperelementPart;2]\n"
        "Node;10;;0.;0.;0.\n"
        "Node;11;;1.;0.;0.\n"
        "Node;12;;1.;1.;0.\n",
        true);
    REQUIRE(model.parts().size() == 1);
    Model& part = *model.parts().front();
    // The copy has no part: its points are looked up in the part of the original, also through a copy of a copy.
    REQUIRE(&model.superelementModel(12) == &part);
    REQUIRE(&model.superelementModel(13) == &part);
    REQUIRE(&model.superelementModel(2) == &part);
    REQUIRE(&model.superelementModel(14) == &model); // a number the deck does not define has no model of its own
    REQUIRE(unresolvedIn(model) == 0);
    // A part renumbered on its own brings the points that the cards of its copies name along.
    Renumbering::apply(part, RenumberSpace::Node, Renumbering::offset(1000));
    const std::array<base::EntityId, 3> moved{1010, 1011, 1012};
    REQUIRE(cardAt<SuperelementLocationTable>(model, 0).pointsA == moved);
    REQUIRE(cardAt<SuperelementLocationTable>(model, 1).pointsA == moved);
    REQUIRE(cardAt<SuperelementLocationTable>(model, 2).pointsA == moved);
    REQUIRE(cardAt<SuperelementLocationTable>(model, 1).pointsB ==
            std::array<base::EntityId, 3>{200, 201, 202}); // the main Bulk Data
    REQUIRE(cardAt<SuperelementConnectionTable>(model).pairs ==
            std::vector<SuperelementConnectionPair>{SuperelementConnectionPair{run(1010, 1010), run(200, 200)}});
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("a loop of copies has no part and does not hang the search", "[model][superelements]")
{
    Model model = readModelText(
        "[model]\nSuperelementBulkOptions;3;Repeat;4\nSuperelementBulkOptions;4;Repeat;3\n[model;SuperelementPart;9]"
        "\nNode;1;;0.;0.;0.\n",
        false);
    REQUIRE(&model.superelementModel(3) == &model);
    REQUIRE(&model.superelementModel(4) == &model);
    REQUIRE(&model.superelementModel(9) == model.parts().front().get());
}
