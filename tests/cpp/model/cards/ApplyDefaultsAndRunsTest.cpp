// Checks of two hooks: Model::applyDefaults (the post-read hook run again, for a model built or edited in
// memory) and the writer's runs (consecutive scalar points written as "first THRU last").

#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/bar2Defaults/Bar2DefaultsTable.hpp"
#include "oor/model/cards/elements/beam2/Beam2Table.hpp"
#include "oor/model/cards/elements/beam2Defaults/Beam2DefaultsTable.hpp"
#include "oor/model/cards/nodes/extraPoint/ExtraPointTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/nodes/nodeDefaults/NodeDefaultsTable.hpp"
#include "oor/model/cards/nodes/scalarPoint/ScalarPointTable.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using oor::testing::readModelText;
using oor::testing::writeModelText;

namespace
{

Model readBulk(const std::string& cards, bool strict = true)
{
    return readModelText(cards, strict);
}

template <typename TableT>
typename TableT::CardType cardAt(const Model& model, std::uint32_t row = 0)
{
    return model.findTable<TableT>()->card(row);
}

} // namespace

TEST_CASE("applyDefaults gives a GRDSET added after the read to the grid points that left their fields blank",
          "[model][cards]")
{
    Model model = readBulk("Node;1;;0.;0.;0.\nNode;2;3;0.;0.;0.;0\nNode;3;;0.;0.;0.;;456;9");
    // No GRDSET yet: the fields stay noted as blank.
    REQUIRE(cardAt<NodeTable>(model, 0).blankFields ==
            (NodeCard::BLANK_POSITION_FRAME | NodeCard::BLANK_DISPLACEMENT_FRAME |
             NodeCard::BLANK_PERMANENT_CONSTRAINTS | NodeCard::BLANK_SUPERELEMENT));
    NodeDefaultsCard defaults;
    defaults.positionFrameId = 14;
    defaults.displacementFrameId = 28;
    defaults.permanentConstraints = 2345;
    defaults.superelementId = 7;
    model.add<NodeDefaultsTable>(defaults);
    REQUIRE(cardAt<NodeTable>(model, 0).positionFrameId == 0); // nothing happens until it is asked for
    model.applyDefaults();
    const NodeCard first = cardAt<NodeTable>(model, 0);
    REQUIRE(first.positionFrameId == 14);
    REQUIRE(first.displacementFrameId == 28);
    REQUIRE(first.permanentConstraints == 2345);
    REQUIRE(first.superelementId == 7);
    REQUIRE(first.blankFields == 0);
    // A value that was given is kept, a zero too: CP 3 and CD 0 of the second grid, PS 456 and SEID 9 of the third.
    const NodeCard second = cardAt<NodeTable>(model, 1);
    REQUIRE(second.positionFrameId == 3);
    REQUIRE(second.displacementFrameId == 0);
    REQUIRE(second.permanentConstraints == 2345);
    REQUIRE(second.superelementId == 7);
    const NodeCard third = cardAt<NodeTable>(model, 2);
    REQUIRE(third.positionFrameId == 14);
    REQUIRE(third.displacementFrameId == 28);
    REQUIRE(third.permanentConstraints == 456);
    REQUIRE(third.superelementId == 9);
    // It is applied once: another call changes nothing, and a changed GRDSET gives nothing to those that took it.
    model.table<NodeDefaultsTable>().store(0, NodeDefaultsCard{99, 99, 99, 99});
    model.applyDefaults();
    REQUIRE(cardAt<NodeTable>(model, 0).positionFrameId == 14);
}

TEST_CASE("applyDefaults on a model with no default card changes nothing", "[model][cards]")
{
    Model model = readBulk("Node;1;;0.;0.;0.\nBar2;1;;1;2\nBeam2;2;;1;2");
    const std::string before = writeModelText(model);
    model.applyDefaults();
    REQUIRE(writeModelText(model) == before);
    REQUIRE(cardAt<NodeTable>(model).blankFields != 0); // still noted
}

TEST_CASE("applyDefaults gives BAROR and BEAMOR to the bars and beams that left their fields blank", "[model][cards]")
{
    Model model = readBulk(
        "Bar2;1;;1;2\nBar2;2;5;1;2;0.;;0.;1.;OffsetsBasicGlobalGlobal\nBeam2;3;;1;2\nBeam2;4;6;1;2;1.;;0.;"
        "0.;OffsetsGlobalGlobalOrient");
    Bar2DefaultsCard bar;
    bar.propId = 37;
    bar.orientation = {0.4, 2.3, -4.75};
    bar.offsetKind = "OffsetsGlobalOrientGlobal";
    model.add<Bar2DefaultsTable>(bar);
    Beam2DefaultsCard beam;
    beam.propId = 40;
    beam.orientation = {0.0, 1.0, 0.0};
    beam.offsetKind = "OffsetsBasicGlobalOrient";
    model.add<Beam2DefaultsTable>(beam);
    model.applyDefaults();
    const Bar2Card blankBar = cardAt<Bar2Table>(model, 0);
    REQUIRE(blankBar.propId == 37);
    REQUIRE(blankBar.orientation == std::array<double, 3>{0.4, 2.3, -4.75});
    REQUIRE(blankBar.offsetKind == "OffsetsGlobalOrientGlobal");
    REQUIRE(blankBar.blankFields == 0);
    const Bar2Card givenBar = cardAt<Bar2Table>(model, 1);
    REQUIRE(givenBar.propId == 5);
    REQUIRE(givenBar.orientation == std::array<double, 3>{0.0, 0.0, 1.0});
    REQUIRE(givenBar.offsetKind == "OffsetsBasicGlobalGlobal");
    const Beam2Card blankBeam = cardAt<Beam2Table>(model, 0);
    REQUIRE(blankBeam.propId == 40);
    REQUIRE(blankBeam.orientation == std::array<double, 3>{0.0, 1.0, 0.0});
    REQUIRE(blankBeam.offsetKind == "OffsetsBasicGlobalOrient");
    REQUIRE(cardAt<Beam2Table>(model, 1).propId == 6);
    REQUIRE(cardAt<Beam2Table>(model, 1).offsetKind == "OffsetsGlobalGlobalOrient");
}

TEST_CASE("a card made with the blank bits set gets the defaults, one without them does not", "[model][cards]")
{
    Model model;
    NodeCard blank;
    blank.id = 1;
    blank.blankFields = NodeCard::BLANK_POSITION_FRAME | NodeCard::BLANK_SUPERELEMENT;
    model.add<NodeTable>(blank);
    NodeCard plain;
    plain.id = 2;
    model.add<NodeTable>(plain);
    NodeDefaultsCard defaults;
    defaults.positionFrameId = 14;
    defaults.displacementFrameId = 28;
    defaults.superelementId = 7;
    model.add<NodeDefaultsTable>(defaults);
    model.applyDefaults();
    REQUIRE(cardAt<NodeTable>(model, 0).positionFrameId == 14);
    REQUIRE(cardAt<NodeTable>(model, 0).displacementFrameId == 0); // not noted as blank
    REQUIRE(cardAt<NodeTable>(model, 0).superelementId == 7);
    REQUIRE(cardAt<NodeTable>(model, 1).positionFrameId == 0);
    REQUIRE(cardAt<NodeTable>(model, 1).superelementId == 0);
}

TEST_CASE("applyDefaults reports a second default card: a strict model throws, a tolerant one notes it",
          "[model][cards]")
{
    Model strict = readBulk("Node;1;;0.;0.;0.");
    NodeDefaultsCard first;
    first.positionFrameId = 14;
    strict.add<NodeDefaultsTable>(first);
    NodeDefaultsCard second;
    second.positionFrameId = 17;
    strict.add<NodeDefaultsTable>(second);
    Model tolerant = strict.copy();
    tolerant.setStrict(false);
    REQUIRE_THROWS_AS(strict.applyDefaults(), base::DeckError);
    REQUIRE(cardAt<NodeTable>(strict).positionFrameId == 14); // the first one was applied before the error was reported
    const std::size_t before = tolerant.diagnostics().errorCount();
    tolerant.applyDefaults();
    REQUIRE(tolerant.diagnostics().errorCount() == before + 1);
    REQUIRE(cardAt<NodeTable>(tolerant).positionFrameId == 14);
}

TEST_CASE("applyDefaults works on the parts of a model, each with its own default card", "[model][cards]")
{
    Model model = readBulk("[model]\nNode;1;;0.;0.;0.\n[model;SuperelementPart;1]\nNode;1;;0.;0.;0.", false);
    REQUIRE(model.parts().size() == 1);
    NodeDefaultsCard defaults;
    defaults.positionFrameId = 14;
    model.parts()[0]->add<NodeDefaultsTable>(defaults);
    model.applyDefaults();
    REQUIRE(cardAt<NodeTable>(*model.parts()[0]).positionFrameId == 14);
    REQUIRE(cardAt<NodeTable>(model).positionFrameId == 0); // the main model has no GRDSET
}

TEST_CASE("scalar and extra points preserve their values, comments and record order", "[model][cards]")
{
    const Model model = readBulk("ScalarPoint;5010;Through;5014\n# a note\nExtraPoint;5110;Through;5112");
    REQUIRE(model.findTable<ScalarPointTable>()->rows().aliveCount() == 5);
    REQUIRE(model.findTable<ExtraPointTable>()->rows().aliveCount() == 3);
    const std::string text = writeModelText(model);
    REQUIRE(text.find("# a note") != std::string::npos);
    REQUIRE(text.find("ScalarPoint") < text.find("ExtraPoint"));
    oor::testing::requireModelFileRoundTrip(model);
}
