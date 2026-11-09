// Checks of the RBE2GS, RCONN and the spline blending, rigid body and relaxation cards: SPBLND1, SPBLND2,
// SPLINRB and SPRELAX. Each card is read with its fields in the places of the format reference, written back in the
// small format with its defaults left blank, and shown to list the references it has and to follow their renumbering.
// The values the cards refuse are listed with the field that reports them.

#include "oor/base/text/NumberParser.hpp"
#include "oor/model/cards/aero/aeroRigidSpline/AeroRigidSplineTable.hpp"
#include "oor/model/cards/aero/aeroSplineCurveBlend/AeroSplineCurveBlendTable.hpp"
#include "oor/model/cards/aero/aeroSplineRelaxation/AeroSplineRelaxationTable.hpp"
#include "oor/model/cards/aero/aeroSplineStripBlend/AeroSplineStripBlendTable.hpp"
#include "oor/model/cards/aero/aeroSurfaceSpline1/AeroSurfaceSpline1Table.hpp"
#include "oor/model/cards/nonlinear_contact/contactElemSurface/ContactElemSurfaceTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBodySearched/RigidBodySearchedTable.hpp"
#include "oor/model/cards/rigid_elements/rigidMeshConnection/RigidMeshConnectionTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;
using Catch::Approx;

namespace
{

// A line of the small format as the writer gives it: each field in eight characters, a number right-justified and a
// word left-justified, the blanks at the end of the line dropped. The first field is the name of the card, or the blank
// marker of a continuation line.

using Runs = std::vector<IdRun>;

} // namespace

TEST_CASE(
    "an RBE2GS has its fields in the places of the format reference, the defaults of CM, TREF and ALPHA and the TYPE "
    "words",
    "[model][cards][rigid]")
{
    Model model = readBulk(
        "RigidBodySearched;3;17;;;1.7;;7.5E-6;;;;;52;94\n"
        "RigidBodySearched;4;;Swapped;2.5;0.3;345;1.E-5;1.;2.;3.\n"
        "RigidBodySearched;5;18;ClosestRigidBodyStars;;4.\n"
        "RigidBodySearched;6;;ClosestRigidBodyStarsSwapped;;-1.;;;;;0.");
    REQUIRE((RigidBodySearchedTable::INFO.category == CardCategory::Element &&
             RigidBodySearchedTable::INFO.idSpace == base::IdSpace::Element));

    // The format example: GS and a radius, the exclusion list of GN empty (the ENDL stands first) and two grids of GM.
    const RigidBodySearchedCard example = cardAt<RigidBodySearchedTable>(model, 0);
    REQUIRE((example.id == 3 && example.spotNode == 17 && !example.type && example.referenceTemperature == 0.0 &&
             example.radius == 1.7));
    REQUIRE((example.dependentComponents == 123456 && example.alphaCoefficient == Approx(7.5e-6)));
    REQUIRE((!example.xStation && !example.yStation && !example.zStation));
    REQUIRE(example.independentNodes.empty());
    REQUIRE(example.dependentNodes == Runs{{52, 52}, {94, 94}});

    // A search point instead of GS, a TYPE, every real and the components in any order.
    const RigidBodySearchedCard point = cardAt<RigidBodySearchedTable>(model, 1);
    REQUIRE((point.id == 4 && point.spotNode == 0 && point.type == RigidBodySearchedType::Nmflip));
    REQUIRE((point.referenceTemperature == 2.5 && point.radius == 0.3 && point.dependentComponents == 345 &&
             point.alphaCoefficient == Approx(1.0e-5)));
    REQUIRE((point.xStation == 1.0 && point.yStation == 2.0 && point.zStation == 3.0));
    REQUIRE((point.independentNodes.empty() && point.dependentNodes.empty()));

    // The words are read in either case; a blank TYPE is std::nullopt (search the complete model).
    const RigidBodySearchedCard lower = cardAt<RigidBodySearchedTable>(model, 2);
    REQUIRE((lower.type == RigidBodySearchedType::Iirbe2 && lower.spotNode == 18 && lower.radius == 4.0));

    // A negative radius, and a search point whose only coordinate is a zero that was written (blank is not zero).
    const RigidBodySearchedCard negative = cardAt<RigidBodySearchedTable>(model, 3);
    REQUIRE((negative.type == RigidBodySearchedType::Nmiirbe2 && negative.radius == -1.0));
    REQUIRE((!negative.xStation && !negative.yStation && negative.zStation == 0.0));
    requireRoundTrip(model);

    // The defaults are blank, a missing coordinate stays blank, and the ENDL appears only with a list.
}

TEST_CASE("an RBE2GS reads the exclusion lists of the alternate formats of the format", "[model][cards][rigid]")
{
    Model model = readBulk(
        "RigidBodySearched;15;35;;;-0.58;;;;;;3;84;Through;103;15;72;84\n"
        "RigidBodySearched;25;;;;0.58;;;4.265;0.;17.4125;3;84;95;104;;15;Through;84\n"
        "RigidBodySearched;35;28;;;-0.58;;;;;;3;52;Through;98;17;23;Through;190\n"
        "RigidBodySearched;45;16;;;-0.58;;;;;;8;52;Through;98;1125;;1142;Through;1190");

    const RigidBodySearchedCard first = cardAt<RigidBodySearchedTable>(model, 0);
    REQUIRE((first.id == 15 && first.spotNode == 35 && first.radius == -0.58));
    REQUIRE(first.independentNodes == Runs{{84, 103}});
    REQUIRE(first.dependentNodes == Runs{{15, 15}, {72, 72}, {84, 84}});

    // GS blank: the search point is on the second line, with a zero that was written.
    const RigidBodySearchedCard second = cardAt<RigidBodySearchedTable>(model, 1);
    REQUIRE((second.spotNode == 0 && second.xStation == 4.265 && second.yStation == 0.0 && second.zStation == 17.4125));
    REQUIRE(second.independentNodes == Runs{{84, 84}, {95, 95}, {104, 104}});
    REQUIRE(second.dependentNodes == Runs{{15, 84}});

    const RigidBodySearchedCard third = cardAt<RigidBodySearchedTable>(model, 2);
    REQUIRE(third.independentNodes == Runs{{52, 98}});
    REQUIRE(third.dependentNodes == Runs{{17, 17}, {23, 190}});

    // A GNi list with its ENDL at the very end: no GMj list.
    const RigidBodySearchedCard fourth = cardAt<RigidBodySearchedTable>(model, 3);
    REQUIRE(fourth.independentNodes == Runs{{52, 98}, {1125, 1125}, {1142, 1190}});
    REQUIRE(fourth.dependentNodes.empty());
    requireRoundTrip(model);

    // The writer puts the lists where the format does, and a THRU never starts a line: the range 1142 THRU 1190 would
    // straddle the first two lines, so it starts the third one.

    // A GMj list alone starts with the ENDL, and a bare ENDL is two empty lists (it is not written back).
    Model onlyGm = readBulk("RigidBodySearched;5;17;;;1.;;;;;;;40;41\nRigidBodySearched;6;17;;;1.");
    REQUIRE(cardAt<RigidBodySearchedTable>(onlyGm, 0).independentNodes.empty());
    REQUIRE(cardAt<RigidBodySearchedTable>(onlyGm, 0).dependentNodes == Runs{{40, 40}, {41, 41}});
    REQUIRE((cardAt<RigidBodySearchedTable>(onlyGm, 1).independentNodes.empty() &&
             cardAt<RigidBodySearchedTable>(onlyGm, 1).dependentNodes.empty()));
    requireRoundTrip(onlyGm);

    // Blank fields between the entries are allowed, and a THRU may start a continuation line (the format advises
    // against it, the reader takes it).
    Model blanks = readBulk("RigidBodySearched;7;17;;;1.;;;;;;3;40;;41;50;Through;60");
    REQUIRE(cardAt<RigidBodySearchedTable>(blanks, 0).independentNodes == Runs{{40, 40}, {41, 41}});
    REQUIRE(cardAt<RigidBodySearchedTable>(blanks, 0).dependentNodes == Runs{{50, 60}});
}

TEST_CASE("an RBE2GS names its search point and the grids of its lists, and follows their renumbering",
          "[model][cards][rigid]")
{
    Model model = readBulk(
        "Node;35;;0.;0.;0.\nRigidBodySearched;15;35;;;-0.58;;;;;;3;84;Through;103;15;72;84\nRigidBodySearched;25;;;;0."
        "58;;"
        ";1.;2.;3.");

    // GS is a node, a single ID of a list is a node and the two ends of a range are nodes that may not exist.
    RecordingVisitor visitor;
    model.table<RigidBodySearchedTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"spotNode", "independentNodes", "independentNodes",
                                                       "dependentNodes", "dependentNodes", "dependentNodes"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::NodeInRange,
                                                            ReferenceTarget::NodeInRange, ReferenceTarget::Node,
                                                            ReferenceTarget::Node, ReferenceTarget::Node});
    visitor = RecordingVisitor{};
    model.table<RigidBodySearchedTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields.empty());   // GS blank, no list: nothing to look up
    REQUIRE(unresolvedIn(model) == 3); // the three single grids of GMj; the range ends may be missing

    // The grids follow a renumbering of the nodes (the column of GS too), the elements one of the elements.
    const std::size_t nodes = Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(nodes >= 1);
    const RigidBodySearchedCard moved = cardAt<RigidBodySearchedTable>(model, 0);
    REQUIRE(moved.spotNode == 1035);
    REQUIRE(moved.independentNodes == Runs{{1084, 1103}});
    REQUIRE(moved.dependentNodes == Runs{{1015, 1015}, {1072, 1072}, {1084, 1084}});
    REQUIRE(model.findTable<RigidBodySearchedTable>()->gsAt(0) == 1035);
    REQUIRE(cardAt<RigidBodySearchedTable>(model, 1).xStation == 1.0); // the coordinates are not references
    const std::size_t elements = Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(elements == 2);
    REQUIRE((model.findTable<RigidBodySearchedTable>()->idAtRow(0) == 115 &&
             cardAt<RigidBodySearchedTable>(model, 1).id == 125));

    // The EID is in the space of all the elements.
    REQUIRE_THROWS_AS(readBulk("RigidBodyStar;10;1;123456;2\nRigidBodySearched;10;17;;;1."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("RigidBodySearched;10;17;;;1.\nRigidBodySearched;10;18;;;1."), base::DeckError);
}

TEST_CASE("an RBE2GS refuses a value, a search location given twice or not at all and a list that is not closed",
          "[model][cards][rigid]")
{
    expectInvalid("RigidBodySearched;0;17;;;1.", "id");
    expectInvalid("RigidBodySearched;1;-1;;;1.", "spotNode");
    expectInvalid("RigidBodySearched;1;17;FLIP;;1.", "kind");
    expectInvalid("RigidBodySearched;1;17;;x;1.", "referenceTemperature");
    expectInvalid("RigidBodySearched;1;17", "radius");
    expectInvalid("RigidBodySearched;1;17;;;0.", "radius");
    expectInvalid("RigidBodySearched;1;17;;;1.;7", "dependentComponents");
    expectInvalid("RigidBodySearched;1;17;;;1.;0", "dependentComponents");
    expectInvalid("RigidBodySearched;1;17;;;1.;-123", "dependentComponents");
    expectInvalid("RigidBodySearched;1;17;;;1.;;x", "alphaCoefficient");
    expectInvalid("RigidBodySearched;1;;;;1.;;;x", "xStation");
    expectInvalid("RigidBodySearched;1;;;;1.;;;;y", "yStation");
    expectInvalid("RigidBodySearched;1;;;;1.;;;;;z", "zStation");

    // GS or a search point, one of them.
    expectInvalid("RigidBodySearched;1;17;;;1.;;;1.;2.;3.", "spotNode");
    expectInvalid("RigidBodySearched;1;17;;;1.;;;;;0.", "spotNode");
    expectInvalid("RigidBodySearched;1;;;;1.", "xStation");
    expectInvalid("RigidBodySearched;1;0;;;1.", "xStation");

    // The lists: a GNi list ends with ENDL, which comes once, a THRU follows a single ID and is followed by the end.
    expectInvalid("RigidBodySearched;1;17;;;1.;;;;;;;40;EndList", "listEndMarker");
    expectInvalid("RigidBodySearched;1;17;;;1.;;;;;;2;Through;41", "through");
    expectInvalid("RigidBodySearched;1;17;;;1.;;;;;;;Through;41", "through");
    expectInvalid("RigidBodySearched;1;17;;;1.;;;;;;5;40;Through;50;Through;60", "through");
    expectInvalid("RigidBodySearched;1;17;;;1.;;;;;;3;50;Through;40", "independentNodes");
    expectInvalid("RigidBodySearched;1;17;;;1.;;;;;;1;4.5", "independentNodes");
    expectInvalid("RigidBodySearched;1;17;;;1.;;;;;;1;0", "independentNodes");
    expectInvalid("RigidBodySearched;1;17;;;1.;;;;;;;x", "dependentNodes");
    expectInvalid("RigidBodySearched;1;17;;;1.;;;;;;;-3", "dependentNodes");
    expectInvalid("RigidBodySearched;1;17;;;1.;;;;;;;50;Through;40", "dependentNodes");
}

TEST_CASE(
    "an RCONN has its fields in the places of the format reference, the blank line of the format and static defaults",
    "[model][cards][rigid]")
{
    Model model = readBulk(
        "RigidMeshConnection;6;Node;Surface;2;8;Normal\n"
        "RigidMeshConnection;8;;;4;9\n"
        "RigidMeshConnection;9;Node;Surface;2;8;Shell;Yes;Distance;2.5\n"
        "RigidMeshConnection;10;Surface;Surface;5;6;Normal;Yes");
    REQUIRE((RigidMeshConnectionTable::INFO.category == CardCategory::Other &&
             RigidMeshConnectionTable::INFO.idSpace == base::IdSpace::PerCardType));

    const RigidMeshConnectionCard example = cardAt<RigidMeshConnectionTable>(model, 0);
    REQUIRE((example.id == 6 && example.secondaryType == RigidMeshConnectionSecondary::Grid &&
             example.primaryType == RigidMeshConnectionPrimary::Surface));
    REQUIRE(
        (example.secondaryId == 2 && example.primaryId == 8 && example.option == RigidMeshConnectionOption::Normal));
    REQUIRE((example.closeGaps == RigidMeshConnectionGapClosing::No &&
             example.gapMeasure == RigidMeshConnectionGapMeasure::Distance));
    REQUIRE(example.gapValue == 1.0E20);

    // Every default is the word or the number of the format.
    const RigidMeshConnectionCard defaults = cardAt<RigidMeshConnectionTable>(model, 1);
    REQUIRE((defaults.secondaryType == RigidMeshConnectionSecondary::Surface &&
             defaults.primaryType == RigidMeshConnectionPrimary::Surface));
    REQUIRE((defaults.option == RigidMeshConnectionOption::Normal &&
             defaults.closeGaps == RigidMeshConnectionGapClosing::No));
    REQUIRE((defaults.gapMeasure == RigidMeshConnectionGapMeasure::Distance && defaults.gapValue == 1.0E20));

    // CLSGAP, GAPDIS and GAPDISV are on the third line, after the blank one.
    const RigidMeshConnectionCard full = cardAt<RigidMeshConnectionTable>(model, 2);
    REQUIRE((full.option == RigidMeshConnectionOption::Shell && full.closeGaps == RigidMeshConnectionGapClosing::Yes &&
             full.gapValue == 2.5));
    const RigidMeshConnectionCard lower = cardAt<RigidMeshConnectionTable>(model, 3);
    REQUIRE((lower.secondaryType == RigidMeshConnectionSecondary::Surface &&
             lower.closeGaps == RigidMeshConnectionGapClosing::Yes));
    requireRoundTrip(model);

    // A card with the default third line is written on its first line alone; otherwise the blank line is written.

    // Two RCONN with one number are refused; the number is not shared with any other card.
    REQUIRE_THROWS_AS(readBulk("RigidMeshConnection;7;;;4;9\nRigidMeshConnection;7;;;5;9"), base::DeckError);

    expectInvalid("RigidMeshConnection;0;;;4;9", "id");
    expectInvalid("RigidMeshConnection;7;BODY;;4;9", "secondaryType");
    expectInvalid("RigidMeshConnection;7;;Node;4;9", "primaryType");
    expectInvalid("RigidMeshConnection;7;;;0;9", "secondaryId");
    expectInvalid("RigidMeshConnection;7;;;4", "primaryId");
    expectInvalid("RigidMeshConnection;7;;;4;-9", "primaryId");
    expectInvalid("RigidMeshConnection;7;;;4;9;TIED", "option");
    expectInvalid("RigidMeshConnection;7;;;4;9;;MAYBE", "closeGaps");
    expectInvalid("RigidMeshConnection;7;;;4;9;;;FACTOR", "gapMeasure");
    expectInvalid("RigidMeshConnection;7;;;4;9;;;;x", "gapValue");
}

TEST_CASE("an RCONN names a BSURF as its secondary side only when STYPE says so, and follows their renumbering",
          "[model][cards][rigid]")
{
    Model model = readBulk(
        "ContactElemSurface;3;101;Through;110\nContactElemSurface;7;111;Through;120\nRigidMeshConnection;1;Surface;"
        "Surface;3;"
        "7\nRigidMeshConnection;2;Node;Surface;3;7");

    // With STYPE GRID the SID is the ID of a SET1 or a BCGRID: the card cannot tell which, so it names nothing.
    RecordingVisitor visitor;
    model.table<RigidMeshConnectionTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"secondaryId", "primaryId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::ContactSurface});
    visitor = RecordingVisitor{};
    model.table<RigidMeshConnectionTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"primaryId"});
    REQUIRE(resolveReference(model, ReferenceTarget::ContactSurface, 7).has_value());

    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(100));
    REQUIRE((cardAt<RigidMeshConnectionTable>(model, 0).secondaryId == 103 &&
             cardAt<RigidMeshConnectionTable>(model, 0).primaryId == 107));
    REQUIRE((cardAt<RigidMeshConnectionTable>(model, 1).secondaryId == 3 &&
             cardAt<RigidMeshConnectionTable>(model, 1).primaryId == 107)); // a SET1 is not moved
    REQUIRE(cardAt<ContactElemSurfaceTable>(model, 0).id == 103);
}

TEST_CASE(
    "a SPBLND1 has its fields in the places of the format reference and needs the reference grid unless it is a "
    "weighted average",
    "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroSplineStripBlend;140;105;125;Cubic;;236;3.85;3.85;1.;0.;0.;105\n"
        "AeroSplineStripBlend;141;105;125\n"
        "AeroSplineStripBlend;142;105;125;WeightedAverage;0.3\n"
        "AeroSplineStripBlend;143;105;125;Linear;;5;1.;2.;0.;1.;0.\n"
        "AeroSplineStripBlend;144;105;125;;1.;7;3.");
    REQUIRE((AeroSplineStripBlendTable::INFO.category == CardCategory::AeroSpline &&
             AeroSplineStripBlendTable::INFO.idSpace == base::IdSpace::PerCardType));

    const AeroSplineStripBlendCard example = cardAt<AeroSplineStripBlendTable>(model, 0);
    REQUIRE((example.id == 140 && example.splineId1 == 105 && example.splineId2 == 125 &&
             example.option == AeroSplineStripBlendOption::Cubic));
    REQUIRE((example.weight1 == 0.5 && example.nodeId == 236 && example.depth1 == 3.85 && example.depth2 == 3.85));
    REQUIRE((example.directionX == 1.0 && example.directionY == 0.0 && example.directionZ == 0.0 &&
             example.frameId == 105));

    // A weighted average with nothing else: the reference grid, the depths and the direction stay blank.
    const AeroSplineStripBlendCard minimal = cardAt<AeroSplineStripBlendTable>(model, 1);
    REQUIRE((minimal.option == AeroSplineStripBlendOption::WeightedAverage && minimal.weight1 == 0.5 &&
             minimal.nodeId == 0));
    REQUIRE((!minimal.depth1 && !minimal.depth2 && !minimal.directionX && !minimal.directionY && !minimal.directionZ &&
             minimal.frameId == 0));
    REQUIRE(cardAt<AeroSplineStripBlendTable>(model, 2).weight1 == 0.3);
    const AeroSplineStripBlendCard linear = cardAt<AeroSplineStripBlendTable>(model, 3);
    REQUIRE((linear.option == AeroSplineStripBlendOption::Linear && linear.frameId == 0 && linear.directionY == 1.0));
    const AeroSplineStripBlendCard partial =
        cardAt<AeroSplineStripBlendTable>(model, 4); // WAVG: a part of the optional values is accepted
    REQUIRE((partial.weight1 == 1.0 && partial.nodeId == 7 && partial.depth1 == 3.0 && !partial.depth2));
    requireRoundTrip(model);

    expectInvalid("AeroSplineStripBlend;0;105;125", "id");
    expectInvalid("AeroSplineStripBlend;140;0;125", "splineId1");
    expectInvalid("AeroSplineStripBlend;140;105;-2", "splineId2");
    expectInvalid("AeroSplineStripBlend;140;105;125;AVG", "option");
    expectInvalid("AeroSplineStripBlend;140;105;125;WeightedAverage;0.", "weight1");
    expectInvalid("AeroSplineStripBlend;140;105;125;WeightedAverage;1.5", "weight1");
    expectInvalid("AeroSplineStripBlend;140;105;125;;-0.2", "weight1");
    expectInvalid("AeroSplineStripBlend;140;105;125;;;-1", "nodeId");
    expectInvalid("AeroSplineStripBlend;140;105;125;WeightedAverage;;;0.", "depth1");
    expectInvalid("AeroSplineStripBlend;140;105;125;WeightedAverage;;;;-2.", "depth2");
    expectInvalid("AeroSplineStripBlend;140;105;125;WeightedAverage;;;;;x", "directionX");
    expectInvalid("AeroSplineStripBlend;140;105;125;WeightedAverage;;;;;;y", "directionY");
    expectInvalid("AeroSplineStripBlend;140;105;125;WeightedAverage;;;;;;;z", "directionZ");
    expectInvalid("AeroSplineStripBlend;140;105;125;WeightedAverage;;;;;;;;-3", "frameId");
    // Only a weighted average can leave out the reference grid, the depths and the direction.
    expectInvalid("AeroSplineStripBlend;140;105;125;Linear;;;1.;2.;0.;1.;0.", "nodeId");
    expectInvalid("AeroSplineStripBlend;140;105;125;Cubic;;5;;2.;0.;1.;0.", "depth1");
    expectInvalid("AeroSplineStripBlend;140;105;125;Cubic;;5;1.;;0.;1.;0.", "depth2");
    expectInvalid("AeroSplineStripBlend;140;105;125;Cubic;;5;1.;2.", "directionX");
    expectInvalid("AeroSplineStripBlend;140;105;125;Cubic;;5;1.;2.;0.;0.;0.", "directionX");
}

TEST_CASE("a SPBLND2 has its fields in the places of the format reference and a linear blend by default",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroSplineCurveBlend;140;105;125;Linear;4;1.5;2.5\n"
        "AeroSplineCurveBlend;141;105;125;;4;1.;1.\n"
        "AeroSplineCurveBlend;142;105;125;Cubic;4;3.;3.5");
    REQUIRE((AeroSplineCurveBlendTable::INFO.category == CardCategory::AeroSpline &&
             AeroSplineCurveBlendTable::INFO.idSpace == base::IdSpace::PerCardType));
    const AeroSplineCurveBlendCard example = cardAt<AeroSplineCurveBlendTable>(model, 0);
    REQUIRE((example.id == 140 && example.splineId1 == 105 && example.splineId2 == 125 &&
             example.option == AeroSplineCurveBlendOption::Linear));
    REQUIRE((example.aeroBoxListId == 4 && example.depth1 == 1.5 && example.depth2 == 2.5));
    REQUIRE(cardAt<AeroSplineCurveBlendTable>(model, 1).option == AeroSplineCurveBlendOption::Linear);
    REQUIRE((cardAt<AeroSplineCurveBlendTable>(model, 2).option == AeroSplineCurveBlendOption::Cubic &&
             cardAt<AeroSplineCurveBlendTable>(model, 2).depth2 == 3.5));
    requireRoundTrip(model);

    // LIN is the default, so it is a blank; CUB is written.

    expectInvalid("AeroSplineCurveBlend;0;105;125;;4;1.;1.", "id");
    expectInvalid("AeroSplineCurveBlend;140;0;125;;4;1.;1.", "splineId1");
    expectInvalid("AeroSplineCurveBlend;140;105;0;;4;1.;1.", "splineId2");
    expectInvalid("AeroSplineCurveBlend;140;105;125;WAVG;4;1.;1.",
                  "option"); // the weighted average is the SPBLND1 option
    expectInvalid("AeroSplineCurveBlend;140;105;125", "aeroBoxListId");
    expectInvalid("AeroSplineCurveBlend;140;105;125;;0;1.;1.", "aeroBoxListId");
    expectInvalid("AeroSplineCurveBlend;140;105;125;;4", "depth1");
    expectInvalid("AeroSplineCurveBlend;140;105;125;;4;0.;1.", "depth1");
    expectInvalid("AeroSplineCurveBlend;140;105;125;;4;1.", "depth2");
    expectInvalid("AeroSplineCurveBlend;140;105;125;;4;1.;-1.", "depth2");
}

TEST_CASE("a SPLINRB has up to six pairs of a grid point and components that select six degrees of freedom",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroRigidSpline;105;30;2;Both;1084;123456\n"
        "AeroRigidSpline;106;;3;;5;123456\n"
        "AeroRigidSpline;107;31;4;Displacement;10;321;11;12;12;3\n"
        "AeroRigidSpline;108;;5;Force;1;1;2;2;3;3;4;4;5;5;6;6\n"
        "AeroRigidSpline;109;;6;;1;123;;;2;12;3;3");
    REQUIRE((AeroRigidSplineTable::INFO.category == CardCategory::AeroSpline &&
             AeroRigidSplineTable::INFO.idSpace == base::IdSpace::PerCardType));

    const AeroRigidSplineCard example = cardAt<AeroRigidSplineTable>(model, 0);
    REQUIRE((example.id == 105 && example.aeroPanelId == 30 && example.aeroBoxListId == 2 &&
             example.usage == SplineUsage::Both));
    REQUIRE(example.dofs == std::vector<DofPair>{{1084, 123456}});

    // CAERO may be blank and USAGE is BOTH when blank.
    const AeroRigidSplineCard minimal = cardAt<AeroRigidSplineTable>(model, 1);
    REQUIRE((minimal.aeroPanelId == 0 && minimal.usage == SplineUsage::Both));

    // The components are kept in ascending order, and three pairs make six degrees of freedom.
    const AeroRigidSplineCard three = cardAt<AeroRigidSplineTable>(model, 2);
    REQUIRE(three.usage == SplineUsage::Displacement);
    REQUIRE(three.dofs == std::vector<DofPair>{{10, 123}, {11, 12}, {12, 3}});

    // Six pairs fill both lines; a blank pair is padding and is dropped.
    const AeroRigidSplineCard six = cardAt<AeroRigidSplineTable>(model, 3);
    REQUIRE(six.usage == SplineUsage::Force);
    REQUIRE(six.dofs == std::vector<DofPair>{{1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}});
    REQUIRE(cardAt<AeroRigidSplineTable>(model, 4).dofs == std::vector<DofPair>{{1, 123}, {2, 12}, {3, 3}});
    requireRoundTrip(model);

    expectInvalid("AeroRigidSpline;0;30;2;;1;123456", "id");
    expectInvalid("AeroRigidSpline;105;-1;2;;1;123456", "aeroPanelId");
    expectInvalid("AeroRigidSpline;105;30;0;;1;123456", "aeroBoxListId");
    expectInvalid("AeroRigidSpline;105;30;2;SOME;1;123456", "usage");
    expectInvalid("AeroRigidSpline;105;30;2", "node1");
    expectInvalid("AeroRigidSpline;105;30;2;;0;123456", "node1");
    expectInvalid("AeroRigidSpline;105;30;2;;;123456", "node1");
    expectInvalid("AeroRigidSpline;105;30;2;;1084", "components1");
    expectInvalid("AeroRigidSpline;105;30;2;;1;1237", "components1");
    expectInvalid("AeroRigidSpline;105;30;2;;1;0", "components1");
    expectInvalid("AeroRigidSpline;105;30;2;;1;-123456", "components1");
    expectInvalid("AeroRigidSpline;105;30;2;;1;12;2", "components2");
    expectInvalid("AeroRigidSpline;105;30;2;;1;12;2;x", "components2");
    // Exactly six degrees of freedom in all.
    expectInvalid("AeroRigidSpline;105;30;2;;1;12345", "components1");
    expectInvalid("AeroRigidSpline;105;30;2;;1;123456;2;1", "components1");
}

TEST_CASE("a SPRELAX has no ID of its own, so several cards relax the same spline", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroSplineRelaxation;140;125;55;4.\n"
        "AeroSplineRelaxation;140;106;56;2.5;65");
    REQUIRE((AeroSplineRelaxationTable::INFO.category == CardCategory::Other &&
             AeroSplineRelaxationTable::INFO.idSpace == base::IdSpace::None));
    const AeroSplineRelaxationCard example = cardAt<AeroSplineRelaxationTable>(model, 0);
    REQUIRE((example.splineId1 == 140 && example.splineId2 == 125 && example.curvePointListId == 55 &&
             example.referenceDistance == 4.0));
    REQUIRE(example.relaxedPointListId == 0); // the format reference default
    const AeroSplineRelaxationCard second = cardAt<AeroSplineRelaxationTable>(model, 1);
    REQUIRE((second.splineId1 == 140 && second.splineId2 == 106 && second.curvePointListId == 56 &&
             second.referenceDistance == 2.5));
    REQUIRE(second.relaxedPointListId == 65);
    REQUIRE(rowCount<AeroSplineRelaxationTable>(model) == 2);
    requireRoundTrip(model);

    expectInvalid("AeroSplineRelaxation;0;125;55;5.", "splineId1");
    expectInvalid("AeroSplineRelaxation;140;0;55;5.", "splineId2");
    expectInvalid("AeroSplineRelaxation;140;125;0;5.", "curvePointListId");
    expectInvalid("AeroSplineRelaxation;140;125;55", "referenceDistance");
    expectInvalid("AeroSplineRelaxation;140;125;55;0.", "referenceDistance");
    expectInvalid("AeroSplineRelaxation;140;125;55;-1.", "referenceDistance");
    expectInvalid("AeroSplineRelaxation;140;125;55;5.;-3", "relaxedPointListId");
}

TEST_CASE(
    "the splines that blend, relax and move as a rigid body name splines of any type and follow their renumbering",
    "[model][cards][aero]")
{
    // A SPBLND1 blends a SPLINE1 and a SPBLND2, which blends the SPLINE1 and a SPLINRB; a SPRELAX relaxes the blend.
    const std::string text =
        "AeroSurfaceSpline1;105;106;117;124;14;0.\n"
        "AeroRigidSpline;106;;2;;1084;123456\n"
        "AeroSplineCurveBlend;125;105;106;;4;1.5;2.5\n"
        "AeroSplineStripBlend;140;105;125;Cubic;;236;3.85;3.85;1.;0.;0.;105\n"
        "AeroSplineRelaxation;140;125;55;4.\n";
    Model model = readBulk(text);

    // SID1 and SID2 are splines, whatever their card type is: the same ID space of splines for every card of the
    // family.
    REQUIRE(resolveReference(model, ReferenceTarget::AeroSpline, 105)->table == typeIndexOf<AeroSurfaceSpline1Table>());
    REQUIRE(resolveReference(model, ReferenceTarget::AeroSpline, 106)->table == typeIndexOf<AeroRigidSplineTable>());
    REQUIRE(resolveReference(model, ReferenceTarget::AeroSpline, 125)->table ==
            typeIndexOf<AeroSplineCurveBlendTable>());
    REQUIRE(resolveReference(model, ReferenceTarget::AeroSpline, 140)->table ==
            typeIndexOf<AeroSplineStripBlendTable>());

    RecordingVisitor visitor;
    model.table<AeroSplineStripBlendTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"splineId1", "splineId2", "nodeId", "frameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroSpline, ReferenceTarget::AeroSpline,
                                                            ReferenceTarget::AeroNode, ReferenceTarget::Coordinate});
    visitor = RecordingVisitor{};
    model.table<AeroSplineCurveBlendTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"splineId1", "splineId2", "aeroBoxListId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroSpline, ReferenceTarget::AeroSpline,
                                                            ReferenceTarget::AeroList});
    visitor = RecordingVisitor{};
    model.table<AeroRigidSplineTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroBoxListId", "node1"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroList, ReferenceTarget::Node});
    visitor = RecordingVisitor{};
    model.table<AeroSplineRelaxationTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"splineId1", "splineId2", "curvePointListId"}); // LIST1 is blank
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroSpline, ReferenceTarget::AeroSpline,
                                                            ReferenceTarget::AeroList});

    // The two splines of a blend resolve: the cards of the deck without the blend and the relaxation have the same
    // unresolved references as the deck with them, except the grid, the system and the list that the new cards name.
    Model without = readBulk(
        "AeroSurfaceSpline1;105;106;117;124;14;0.\nAeroRigidSpline;106;;2;;1084;123456\nAeroSplineCurveBlend;125;105;"
        "106;;4;1.5;2.5\n");
    Model blended = readBulk(
        "AeroSurfaceSpline1;105;106;117;124;14;0.\n"
        "AeroRigidSpline;106;;2;;1084;123456\n"
        "AeroSplineCurveBlend;125;105;106;;4;1.5;2.5\n"
        "AeroSplineStripBlend;140;105;125");
    REQUIRE(unresolvedIn(blended) == unresolvedIn(without));
    Model dangling = readBulk("AeroSplineStripBlend;140;105;125\nAeroSplineRelaxation;150;140;55;5.");
    REQUIRE(unresolvedIn(dangling) ==
            4); // SID1 and SID2 of the blend, SID1 and LIST2 of the relaxation (its SID2 is the blend)

    // One renumbering of the splines moves the SID of every card of the family and every SID1 and SID2 that names one.
    const std::size_t changed = Renumbering::apply(model, RenumberSpace::AeroSpline, Renumbering::offset(1000));
    REQUIRE(changed == 4);
    REQUIRE(cardAt<AeroSurfaceSpline1Table>(model, 0).id == 1105);
    REQUIRE(cardAt<AeroRigidSplineTable>(model, 0).id == 1106);
    const AeroSplineCurveBlendCard blend2 = cardAt<AeroSplineCurveBlendTable>(model, 0);
    REQUIRE((blend2.id == 1125 && blend2.splineId1 == 1105 && blend2.splineId2 == 1106));
    const AeroSplineStripBlendCard blend1 = cardAt<AeroSplineStripBlendTable>(model, 0);
    REQUIRE((blend1.id == 1140 && blend1.splineId1 == 1105 && blend1.splineId2 == 1125));
    const AeroSplineRelaxationCard relax = cardAt<AeroSplineRelaxationTable>(model, 0);
    REQUIRE((relax.splineId1 == 1140 && relax.splineId2 == 1125));
    REQUIRE(model.table<AeroSplineRelaxationTable>().splineId1Column().get(0) == 1140); // the column follows the card
    REQUIRE((blend1.nodeId == 236 && blend1.frameId == 105 && blend2.aeroBoxListId == 4)); // what is not a spline stays

    // A SPLINE1 and a SPLINRB with one ID are one selection: both get the same new ID.
    Model shared = readBulk("AeroSurfaceSpline1;55;106;117;124;14;0.\nAeroRigidSpline;55;;2;;1;123456");
    Renumbering::apply(shared, RenumberSpace::AeroSpline, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<AeroSurfaceSpline1Table>(shared, 0).id == 1055 && cardAt<AeroRigidSplineTable>(shared, 0).id == 1055));

    // The lists and grids follow their own spaces.
    Renumbering::apply(model, RenumberSpace::AeroList, Renumbering::offset(100));
    REQUIRE((cardAt<AeroSplineCurveBlendTable>(model, 0).aeroBoxListId == 104 &&
             cardAt<AeroRigidSplineTable>(model, 0).aeroBoxListId == 102));
    REQUIRE(cardAt<AeroSplineRelaxationTable>(model, 0).curvePointListId == 155);
    Renumbering::apply(model, RenumberSpace::AeroNode, Renumbering::offset(1));
    REQUIRE(cardAt<AeroSplineStripBlendTable>(model, 0).nodeId == 237);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(1));
    REQUIRE(cardAt<AeroSplineStripBlendTable>(model, 0).frameId == 106);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1));
    REQUIRE(cardAt<AeroRigidSplineTable>(model, 0).dofs == std::vector<DofPair>{{1085, 123456}});
}
