// Checks of what each card type reads: the values, the defaults and the dependent defaults the format reference
// describes.

#include "oor/model/cards/constraints/singlePointConstrList/SinglePointConstrListTable.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/loads/pointForceLoad/PointForceLoadTable.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/parameters/parameter/ParameterTable.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

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

} // namespace

TEST_CASE("GRID reads its fields and defaults the coordinate systems to 0", "[model][cards]")
{
    const Model model = readBulk("Node;5;;1.5;-2.5;1.E+3");
    const NodeTable& grid = *model.findTable<NodeTable>();
    const NodeCard card = grid.card(model.locate(base::IdSpace::Node, 5)->row);
    REQUIRE(card.id == 5);
    REQUIRE(card.positionFrameId == 0);
    REQUIRE(card.displacementFrameId == 0);
    REQUIRE(card.xyz.x == 1.5);
    REQUIRE(card.xyz.y == -2.5);
    REQUIRE(card.xyz.z == 1000.0);
}

TEST_CASE("MAT1 fills in the blank one of E, G and NU", "[model][cards]")
{
    const Model model = readBulk("IsoMat;1;2.1E+11;;0.3\nIsoMat;2;2.1E+11;8.1E+10\nIsoMat;3;;8.E+10;0.25");
    const IsoMatTable& mat = *model.findTable<IsoMatTable>();
    const IsoMatCard first = mat.card(model.locate(base::IdSpace::Material, 1)->row);
    REQUIRE(first.shearModulus == Approx(2.1e11 / 2.6));
    const IsoMatCard second = mat.card(model.locate(base::IdSpace::Material, 2)->row);
    REQUIRE(second.poissonRatio == Approx(2.1e11 / 1.62e11 - 1.0));
    const IsoMatCard third = mat.card(model.locate(base::IdSpace::Material, 3)->row);
    REQUIRE(third.youngsModulus == Approx(2.0 * 8.0e10 * 1.25));
}

TEST_CASE("PSHELL defaults", "[model][cards]")
{
    const Model model = readBulk("ShellProp;7;1;0.02");
    const ShellPropTable& table = *model.findTable<ShellPropTable>();
    const ShellPropCard card = table.card(model.locate(base::IdSpace::Property, 7)->row);
    REQUIRE(card.thickness == 0.02);
    REQUIRE(card.membraneMatId == 1);
}

TEST_CASE("PCOMP resolves Z0 and repeats the previous ply", "[model][cards]")
{
    const Model model = readBulk("CompProp;10;;;;;;;;1;0.1;0.;Yes;;;45.\nCompProp;11;;;;;;;Symmetry;2;0.25;0.");
    const CompPropTable& table = *model.findTable<CompPropTable>();
    const CompPropCard full = table.card(model.locate(base::IdSpace::Property, 10)->row);
    REQUIRE(full.plies.size() == 2);
    REQUIRE(full.plies[0].stressOutput);
    REQUIRE(full.plies[1].matId == 1);       // repeated
    REQUIRE(full.plies[1].thickness == 0.1); // repeated
    REQUIRE(full.plies[1].thetaInDegrees == 45.0);
    REQUIRE(full.bottomOffset == Approx(-0.1)); // minus half the total thickness
    const CompPropCard symmetric = table.card(model.locate(base::IdSpace::Property, 11)->row);
    REQUIRE(symmetric.option == LaminateOption::Sym);
    REQUIRE(symmetric.totalThickness() == Approx(0.5));
    REQUIRE(symmetric.bottomOffset == Approx(-0.25));
}

TEST_CASE("CQUAD4 takes an integer in field 7 as MCID and a real as THETA", "[model][cards]")
{
    const Model model = readBulk("ShellQuad4;1;1;1;2;3;4;;7\nShellQuad4;2;1;1;2;3;4;30.");
    const ShellQuad4Table& table = *model.findTable<ShellQuad4Table>();
    const ShellQuad4Card byMcid = table.card(model.locate(base::IdSpace::Element, 1)->row);
    REQUIRE(byMcid.matFrameId == 7);
    REQUIRE(byMcid.usesMaterialSystem());
    const ShellQuad4Card byTheta = table.card(model.locate(base::IdSpace::Element, 2)->row);
    REQUIRE_FALSE(byTheta.usesMaterialSystem());
    REQUIRE(byTheta.thetaInDegrees == 30.0);
}

TEST_CASE("CQUAD4 PID defaults to EID and repeated nodes are refused", "[model][cards]")
{
    const Model model = readBulk("ShellQuad4;9;;1;2;3;4");
    REQUIRE(model.findTable<ShellQuad4Table>()->card(0).propId == 9);
    REQUIRE_THROWS_AS(readBulk("ShellQuad4;9;1;1;2;3;1"), base::DeckError);
}

TEST_CASE("SPC1 keeps singles and THRU ranges in order", "[model][cards]")
{
    const Model model = readBulk("SinglePointConstrList;3;123;1;2;10;Through;20;30");
    const SinglePointConstrListCard card = model.findTable<SinglePointConstrListTable>()->card(0);
    REQUIRE(card.id == 3);
    REQUIRE(card.components == 123);
    REQUIRE(card.nodes.size() == 4);
    REQUIRE(card.nodes[2] == NodeRun{10, 20});
    REQUIRE(card.nodes[3] == NodeRun{30, 30});
}

TEST_CASE("cards of one load set share the set ID without clashing", "[model][cards]")
{
    const Model model = readBulk("PointForceLoad;1;1;;10.;1.;0.;0.\nPointForceLoad;1;2;;20.;0.;1.;0.");
    REQUIRE(model.findTable<PointForceLoadTable>()->rows().aliveCount() == 2);
}

TEST_CASE("PARAM recognises integers, reals, words and complex values", "[model][cards]")
{
    const Model model = readBulk(
        "Parameter;PostprocessingFile;-1\nParameter;WeightToMass;0.00259\nParameter;AutoSingularityConstr;"
        "Yes\nParameter;CP;1.;2.");
    const ParameterTable& table = *model.findTable<ParameterTable>();
    REQUIRE(table.card(0).kind == ParameterKind::Integer);
    REQUIRE(table.card(0).integerValue == -1);
    REQUIRE(table.card(1).kind == ParameterKind::Real);
    REQUIRE(table.card(2).kind == ParameterKind::Text);
    REQUIRE(table.card(2).textValue == "Yes");
    REQUIRE(table.card(3).kind == ParameterKind::Complex);
}
