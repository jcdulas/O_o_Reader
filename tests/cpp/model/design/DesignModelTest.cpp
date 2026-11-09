// The design model of a deck: the values of the variables, the relations and what they drive, and the
// application of a design point: DLINK first, then the linear relations and the equations with the constants of a
// DTABLE and a DTABLE2, the limits of a relation, the fields of the cards that a relation can name, and the refusal of
// the rest. Every expected number is worked out in a comment next to it.

#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaTable.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/springDofInline/SpringDofInlineTable.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/optimisation/designVar/DesignVarTable.hpp"
#include "oor/model/cards/properties/barProp/BarPropTable.hpp"
#include "oor/model/cards/properties/barShapeProp/BarShapePropTable.hpp"
#include "oor/model/cards/properties/beamProp/BeamPropTable.hpp"
#include "oor/model/cards/properties/beamShapeProp/BeamShapePropTable.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"
#include "oor/model/cards/properties/tubeProp/TubePropTable.hpp"
#include "oor/model/design/DesignErrors.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/model/design/DesignUpdate.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cmath>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{

// A deck with a plate, a bar, a rod and a material, and the design cards that drive them:
//   T of the PSHELL 10     = 0.1 + 2 x1 + 0.5 x2                      (DVPREL1 11, two variables and a constant)
//   A of the PBAR 20       = SCALE * x3**2, SCALE = 2 from a DTABLE   (DVPREL2 12, an equation)
//   RHO of the MAT1 3      = 1000 x4, x4 = 0.5 + 2 (x1 + 3 x2)         (DVMREL1 13, x4 depends on x1 and x2 by a DLINK)
//   A of the PROD 30       = 1.0E-4 x3                                 (DVPREL1 15, one variable and no constant)
// x1 = 0.05 and x2 = 0.2 give T = 0.1 + 0.1 + 0.1 = 0.3, x4 = 0.5 + 2 (0.05 + 0.6) = 1.8 and RHO = 1800.
const std::string DECK =
    "IsoMat;3;2.1E+11;;0.3;7850.\n"
    "ShellProp;10;3;0.01\n"
    "BarProp;20;3;0.002;1.E-6;1.E-6;1.E-6\n"
    "RodProp;30;3;5.E-4\n"
    "DesignVar;1;T1;0.05;0.01;0.1\n"
    "DesignVar;2;T2;0.2;0.;1.\n"
    "DesignVar;3;AREA;3.;1.;10.\n"
    "DesignVar;4;DEP;0.;-100.;100.\n"
    "DesignVarLink;1;4;0.5;2.;1;1.;2;3.\n"
    "DesignPropRelation1;11;ShellProp;10;thickness;;;0.1;1;2.;2;0.5\n"
    "DesignPropRelation2;12;BarProp;20;area;;;1;DesignVar;1;3;DesignConstants;1;SCALE\n"
    "DesignEquation;1;F(X,S)=S*X**2\n"
    "DesignConstants;SCALE;2.\n"
    "DesignMatRelation1;13;IsoMat;3;density;;;0.;4;1.E+3\n"
    "DesignPropRelation1;15;RodProp;30;area;;;;3;1.E-4\n";

const std::string GRIDS = "Node;1;;0.;0.;0.\nNode;2;;1.;0.;0.\nNode;3;;1.;1.;0.\nNode;4;;0.;1.;0.\n";

std::size_t relationIndex(const DesignModel& design, const std::string& card, base::EntityId id)
{
    for (std::size_t index = 0; index < design.relations().size(); ++index)
    {
        if (design.relations()[index].card == card && design.relations()[index].id == id)
            return index;
    }
    FAIL("no relation " << card << " " << id);
    return 0;
}

DesignPoint pointOf(const DesignModel& design, std::initializer_list<std::pair<base::EntityId, double>> values)
{
    DesignPoint point = design.initialPoint();
    for (const auto& [id, value] : values)
        point.values[*design.findVariable(id)] = value;
    return point;
}

} // namespace

TEST_CASE("the design model lists the variables with their bounds, the move limit and the DLINK that gives them",
          "[model][design]")
{
    Model model = readBulk(DECK + "DesignOptimParams;DesignVarMoveLimit;0.25\n");
    const DesignModel design = DesignModel::read(model);
    REQUIRE(design.variables().size() == 4);
    const DesignVariable& thickness = design.variables()[0];
    REQUIRE(thickness.id == 1);
    REQUIRE(thickness.label == "T1");
    REQUIRE(thickness.initial == 0.05);
    REQUIRE(thickness.lower == 0.01);
    REQUIRE(thickness.upper == 0.1);
    REQUIRE(thickness.moveLimit == 0.25); // no DELXV: the DELX of the DOPTPRM
    REQUIRE(thickness.link == 0);
    REQUIRE(design.variables()[3].link == 1); // the variable 4 is given by the DLINK 1
    REQUIRE(*design.findVariable("AREA") == 2);
    REQUIRE_FALSE(design.findVariable("area").has_value());
    REQUIRE_FALSE(design.findVariable(99).has_value());
    REQUIRE(design.relations().size() == 4);
    for (const DesignRelation& relation : design.relations())
        REQUIRE(relation.problem.empty());
}

TEST_CASE(
    "a design point gives the fields the relations compute: DLINK, a linear relation, an equation with a DTABLE "
    "constant",
    "[model][design]")
{
    Model model = readBulk(DECK);
    const DesignModel design = DesignModel::read(model);
    const DesignPoint point = pointOf(design, {{1, 0.05}, {2, 0.2}, {3, 3.0}});
    const DesignEvaluation evaluation = design.evaluate(model, point, true);

    // x4 = 0.5 + 2 (1 x1 + 3 x2) = 0.5 + 2 (0.05 + 0.6) = 1.8; the value given for it (0.0) is ignored
    REQUIRE(evaluation.variables[3] == Approx(1.8));
    // T = 0.1 + 2 x1 + 0.5 x2 = 0.1 + 0.1 + 0.1
    REQUIRE(evaluation.relations[relationIndex(design, "DesignPropRelation1", 11)].requested == Approx(0.3));
    // A = SCALE x3**2 = 2 * 9
    REQUIRE(evaluation.relations[relationIndex(design, "DesignPropRelation2", 12)].requested == Approx(18.0));
    // RHO = 1000 x4
    REQUIRE(evaluation.relations[relationIndex(design, "DesignMatRelation1", 13)].requested == Approx(1800.0));
    // A of the rod = 1.0E-4 x3
    REQUIRE(evaluation.relations[relationIndex(design, "DesignPropRelation1", 15)].requested == Approx(3.0e-4));
    // what the model holds now
    REQUIRE(evaluation.relations[relationIndex(design, "DesignPropRelation1", 11)].current == 0.01);
    REQUIRE(evaluation.relations[relationIndex(design, "DesignMatRelation1", 13)].current == 7850.0);
}

TEST_CASE("apply writes every field in one operation and the thickness takes the default fibre distances along",
          "[model][design]")
{
    Model model = readBulk(DECK);
    const DesignModel design = DesignModel::read(model);
    const std::uint64_t before = model.contentVersion();
    const DesignUpdate update = applyDesign(model, design, pointOf(design, {{1, 0.05}, {2, 0.2}, {3, 3.0}}));
    REQUIRE(update.changedFields == 4);
    REQUIRE(model.contentVersion() != before);
    const ShellPropCard shell = cardAt<ShellPropTable>(model);
    REQUIRE(shell.thickness == Approx(0.3));
    // Z1 and Z2 were blank: they stay blank, and so follow the new thickness
    REQUIRE_FALSE(shell.fiberDistance1.has_value());
    REQUIRE_FALSE(shell.fiberDistance2.has_value());
    REQUIRE(cardAt<BarPropTable>(model).area == Approx(18.0));
    REQUIRE(cardAt<IsoMatTable>(model).density == Approx(1800.0));
    REQUIRE(cardAt<RodPropTable>(model).area == Approx(3.0e-4));
    // the design variables keep their initial values unless asked
    REQUIRE(cardAt<DesignVarTable>(model).initialValue == 0.05);
}

TEST_CASE("a Z1 that was written is not moved by the thickness", "[model][design]")
{
    Model model = readBulk(
        "ShellProp;10;3;0.01;;;;;;-0.002;0.004\n"
        "IsoMat;3;2.1E+11;;0.3\n"
        "DesignVar;1;T;0.01;0.001;1.\n"
        "DesignPropRelation1;11;ShellProp;10;thickness;;;;1;1.\n");
    const DesignModel design = DesignModel::read(model);
    (void)applyDesign(model, design, pointOf(design, {{1, 0.3}}));
    const ShellPropCard shell = cardAt<ShellPropTable>(model);
    REQUIRE(shell.thickness == Approx(0.3));
    REQUIRE(shell.fiberDistance1.value() == Approx(-0.002));
    REQUIRE(shell.fiberDistance2.value() == Approx(0.004));
}

TEST_CASE("the initial values leave a model whose relations are consistent exactly as it was", "[model][design]")
{
    // T = 0.1 + 2 x1 + 0.5 x2 = 0.3 at x1 = 0.05, x2 = 0.2
    Model model = readBulk(
        "IsoMat;3;2.1E+11;;0.3;7850.\n"
        "ShellProp;10;3;0.3\n"
        "DesignVar;1;T1;0.05;0.01;0.1\n"
        "DesignVar;2;T2;0.2\n"
        "DesignPropRelation1;11;ShellProp;10;thickness;;;0.1;1;2.;2;0.5\n");
    const DesignModel design = DesignModel::read(model);
    const std::uint64_t before = model.contentVersion();
    const DesignUpdate update = applyDesign(model, design, design.initialPoint());
    REQUIRE(update.changedFields == 0);
    REQUIRE(update.fields.size() == 1);
    REQUIRE_FALSE(update.fields[0].changed);
    REQUIRE(model.contentVersion() == before); // nothing was written, not even the same number again
}

TEST_CASE("a relation that drives a field the card does not have refuses the whole point and writes nothing",
          "[model][design]")
{
    Model model = readBulk(DECK + "DesignPropRelation1;16;RodProp;30;t;;;;3;1.\n");
    const DesignModel design = DesignModel::read(model);
    const std::size_t bad = relationIndex(design, "DesignPropRelation1", 16);
    REQUIRE_FALSE(design.relations()[bad].applicable);
    REQUIRE_THAT(design.relations()[bad].problem, ContainsSubstring("RodProp"));
    REQUIRE_THAT(design.relations()[bad].problem, ContainsSubstring("named t"));
    const std::uint64_t before = model.contentVersion();
    REQUIRE_THROWS_AS(applyDesign(model, design, pointOf(design, {{1, 0.05}, {3, 3.0}})), DesignError);
    REQUIRE(model.contentVersion() == before);
    REQUIRE(cardAt<ShellPropTable>(model).thickness == 0.01); // the relations that could be applied were not
    // with skipUnapplicable the others are applied and the relation is reported
    DesignApplyOptions options;
    options.skipUnapplicable = true;
    const DesignUpdate update = applyDesign(model, design, pointOf(design, {{1, 0.05}, {3, 3.0}}), options);
    REQUIRE(update.skipped == std::vector<std::size_t>{bad});
    REQUIRE(cardAt<ShellPropTable>(model).thickness != 0.01);
}

TEST_CASE(
    "a relation on a missing property, a property of another type and a design variable that is not there are "
    "refused with a message that names them",
    "[model][design]")
{
    Model model = readBulk(
        "IsoMat;3;2.1E+11;;0.3\n"
        "ShellProp;10;3;0.01\n"
        "DesignVar;1;T;0.01\n"
        "DesignPropRelation1;11;BarProp;10;area;;;;1;1.\n"
        "DesignPropRelation1;12;ShellProp;99;thickness;;;;1;1.\n"
        "DesignPropRelation1;13;ShellProp;10;thickness;;;;7;1.\n");
    const DesignModel design = DesignModel::read(model);
    REQUIRE_THAT(design.relations()[0].problem, ContainsSubstring("is a ShellProp, not a BarProp"));
    REQUIRE_THAT(design.relations()[1].problem, ContainsSubstring("property 99 does not exist"));
    REQUIRE_THAT(design.relations()[2].problem, ContainsSubstring("design variable 7 does not exist"));
    REQUIRE_THROWS_AS(applyDesign(model, design, design.initialPoint()), DesignError);
}

TEST_CASE("the limits of a relation clip the value, refuse it or are ignored, and the report says which was crossed",
          "[model][design]")
{
    // PMIN 0.5 on the thickness: x1 = 0.05 and x2 = 0.2 give 0.3
    const std::string deck =
        "IsoMat;3;2.1E+11;;0.3\n"
        "ShellProp;10;3;0.01\n"
        "DesignVar;1;T1;0.05\n"
        "DesignVar;2;T2;0.2\n"
        "DesignPropRelation1;11;ShellProp;10;thickness;0.5;;0.1;1;2.;2;0.5\n";
    {
        Model model = readBulk(deck);
        const DesignModel design = DesignModel::read(model);
        const DesignUpdate update = applyDesign(model, design, design.initialPoint());
        REQUIRE(update.fields[0].requested == Approx(0.3));
        REQUIRE(update.fields[0].after == Approx(0.5));
        REQUIRE(update.fields[0].limit == DesignLimitHit::Minimum);
        REQUIRE(cardAt<ShellPropTable>(model).thickness == Approx(0.5));
    }
    {
        Model model = readBulk(deck);
        const DesignModel design = DesignModel::read(model);
        DesignApplyOptions options;
        options.limits = DesignLimitMode::Error;
        REQUIRE_THROWS_WITH(applyDesign(model, design, design.initialPoint(), options),
                            ContainsSubstring("DesignPropRelation1 11 gives ShellProp 10 thickness"));
        REQUIRE(cardAt<ShellPropTable>(model).thickness == 0.01);
    }
    {
        Model model = readBulk(deck);
        const DesignModel design = DesignModel::read(model);
        DesignApplyOptions options;
        options.limits = DesignLimitMode::Ignore;
        const DesignUpdate update = applyDesign(model, design, design.initialPoint(), options);
        REQUIRE(update.fields[0].limit == DesignLimitHit::Minimum); // still reported
        REQUIRE(cardAt<ShellPropTable>(model).thickness == Approx(0.3));
    }
}

TEST_CASE(
    "the default PMIN follows the field: 1.0E-15 for a thickness, none for an offset, and none for a relation of "
    "one variable with no constant",
    "[model][design]")
{
    Model model = readBulk(GRIDS +
                           "IsoMat;3;2.1E+11;;0.3\n"
                           "ShellProp;10;3;0.01\n"
                           "ShellQuad4;1;10;1;2;3;4\n"
                           "DesignVar;1;X;1.\n"
                           "DesignPropRelation1;11;ShellProp;10;thickness;;;0.5;1;1.\n"
                           "DesignElemRelation1;12;ShellQuad4;1;offset;;;0.5;1;1.\n"
                           "DesignPropRelation1;13;ShellProp;10;nonStructMass;;;;1;1.\n"
                           "DesignPropRelation2;14;ShellProp;10;fiberDistance1;;;1;DesignVar;1;1\n"
                           "DesignPropRelation2;15;ShellProp;10;shearThicknessRatio;;;1;DesignVar;1;1\n"
                           "DesignEquation;1;F(X)=X\n");
    const DesignModel design = DesignModel::read(model);
    REQUIRE(design.relations()[relationIndex(design, "DesignPropRelation1", 11)].lowest == 1.0e-15);
    REQUIRE(design.relations()[relationIndex(design, "DesignElemRelation1", 12)].lowest == -1.0e35);
    const DesignRelation& oneVariable = design.relations()[relationIndex(design, "DesignPropRelation1", 13)];
    REQUIRE_FALSE(oneVariable.defaultsApplied);
    REQUIRE(oneVariable.lowest == -1.0e35);
    REQUIRE(design.relations()[relationIndex(design, "DesignPropRelation2", 14)].lowest == -1.0e35);
    REQUIRE(design.relations()[relationIndex(design, "DesignPropRelation2", 15)].lowest == 1.0e-15);
}

TEST_CASE("PVAL takes the first coefficient from the field, and a DTABLE2 constant is read before anything is written",
          "[model][design]")
{
    Model model = readBulk(
        "IsoMat;3;2.1E+11;;0.3\n"
        "ShellProp;10;3;0.2\n"
        "RodProp;30;3;5.E-4\n"
        "DesignVar;1;S;1.\n"
        "DesignVar;2;P;1.\n"
        "DesignPropRelation1;11;RodProp;30;area;;;;1;PropertyValue\n"
        "DesignPropRelation2;12;ShellProp;10;thickness;;;1;DesignVar;1;2;DesignConstants;1;BASET\n"
        "DesignEquation;1;F(X,B)=B*X+0.1\n"
        "DesignFieldConstants;BASET;ShellProp;10;thickness\n");
    const DesignModel design = DesignModel::read(model);
    // A = A0 * x1 = 5.0E-4 * 1.5; T = BASET * x2 + 0.1 = 0.2 * 2 + 0.1 (BASET is the thickness before the update)
    (void)applyDesign(model, design, pointOf(design, {{1, 1.5}, {2, 2.0}}));
    REQUIRE(cardAt<RodPropTable>(model).area == Approx(7.5e-4));
    REQUIRE(cardAt<ShellPropTable>(model).thickness == Approx(0.5));
}

TEST_CASE("an equation whose DTABLE constant is a table or does not exist cannot be applied", "[model][design]")
{
    Model model = readBulk(
        "IsoMat;3;2.1E+11;;0.3\n"
        "ShellProp;10;3;0.2\n"
        "DesignVar;1;S;1.\n"
        "DesignPropRelation2;12;ShellProp;10;thickness;;;1;DesignVar;1;1;DesignConstants;1;FREQ\n"
        "DesignPropRelation2;13;ShellProp;10;nonStructMass;;;1;DesignVar;1;1;DesignConstants;1;NOPE\n"
        "DesignEquation;1;F(X,B)=B*X\n"
        "DesignConstants;FREQ;7\n");
    const DesignModel design = DesignModel::read(model);
    REQUIRE_THAT(design.relations()[0].problem, ContainsSubstring("dynamic table 7"));
    REQUIRE_THAT(design.relations()[1].problem, ContainsSubstring("NOPE is not defined"));
}

TEST_CASE("the fields of property cards use neutral names and indexed selectors", "[model][design]")
{
    Model model = readBulk(
        "IsoMat;3;2.1E+11;;0.3\n"
        "BarProp;20;3;0.002;1.E-6;1.E-6;1.E-6\n"
        "BarShapeProp;21;3;;Tube;0.05;0.004\n"
        "BeamProp;22;3;1.;2.;3.;0.;4.;0.1;;;;;;;;;Yes;1.;5.;6.;7.;0.;8.;0.2\n"
        "BeamShapeProp;23;3;;Tube;0.05;0.004;;Yes;1.;0.03;0.003\n"
        "CompProp;24;;;;;;;;3;0.1;0.;Yes;3;0.2;45.;Yes\n"
        "RodProp;25;3;5.E-4;1.E-8;0.;0.01\n"
        "TubeProp;26;3;0.05;0.004\n"
        "DesignVar;1;X;1.\n"
        "DesignPropRelation1;1;BarProp;20;bendingInertia1;;;;1;2.\n"
        "DesignPropRelation1;2;BarProp;20;area;;;;1;3.\n"
        "DesignPropRelation1;3;BarShapeProp;21;dimension(2);;;;1;0.002\n"
        "DesignPropRelation1;4;BarShapeProp;21;nonStructMass;;;;1;0.5\n"
        "DesignPropRelation1;5;BeamProp;22;area(EndA);;;;1;1.5\n"
        "DesignPropRelation1;6;BeamProp;22;bendingInertia1(EndB);;;;1;9.\n"
        "DesignPropRelation1;7;BeamShapeProp;23;dimension(2)(EndB);;;;1;0.004\n"
        "DesignPropRelation1;8;CompProp;24;plyThickness(2);;;;1;0.25\n"
        "DesignPropRelation1;9;CompProp;24;plyAngle(1);;;;1;10.\n"
        "DesignPropRelation1;10;RodProp;25;torsionStressCoefficient;;;;1;0.001\n"
        "DesignPropRelation1;11;TubeProp;26;outerDiameter;;;;1;0.06\n"
        "DesignPropRelation1;12;BeamProp;22;area(2);;;;1;0.5\n"); // the first station after end A: the end B of this
                                                                  // two station card
    const DesignModel design = DesignModel::read(model);
    for (const DesignRelation& relation : design.relations())
    {
        INFO(relation.card << " " << relation.id << ": " << relation.problem);
        REQUIRE(relation.applicable);
    }
    (void)applyDesign(model, design, pointOf(design, {{1, 2.0}}));
    REQUIRE(cardAt<BarPropTable>(model).bendingInertia1 == Approx(4.0e0));  // 2 x
    REQUIRE(cardAt<BarPropTable>(model).area == Approx(6.0));               // 3 x
    REQUIRE(cardAt<BarShapePropTable>(model).dims[1] == Approx(0.004));     // 0.002 x
    REQUIRE(cardAt<BarShapePropTable>(model).nonStructMass == Approx(1.0)); // 0.5 x
    const BeamPropCard beam = cardAt<BeamPropTable>(model);
    REQUIRE(beam.stations[0].area == Approx(3.0));                                   // 1.5 x
    REQUIRE(beam.stations[1].bendingInertia1 == Approx(18.0));                       // 9 x
    REQUIRE(beam.stations[1].area == Approx(1.0));                                   // A(1) is the station B: 0.5 x
    REQUIRE(cardAt<BeamShapePropTable>(model).stations[1].dims[1] == Approx(0.008)); // 0.004 x
    const CompPropCard laminate = cardAt<CompPropTable>(model);
    REQUIRE(laminate.plies[1].thickness == Approx(0.5));       // 0.25 x
    REQUIRE(laminate.plies[0].thetaInDegrees == Approx(20.0)); // 10 x
    REQUIRE(cardAt<RodPropTable>(model).stressCoefficient == Approx(2.0e-3));
    const TubePropCard tube = cardAt<TubePropTable>(model);
    REQUIRE(tube.outerDiameter == Approx(0.12));
}

TEST_CASE(
    "a field that is not designable, and a station the card does not have, are refused with the card and the field",
    "[model][design]")
{
    Model model = readBulk(
        "IsoMat;3;2.1E+11;;0.3\n"
        "BarProp;20;3;0.002;1.E-6;1.E-6;1.E-6\n"
        "ShellProp;10;3;0.01\n"
        "BeamProp;22;3;1.;2.;3.;0.;4.;0.1\n"
        "BeamShapeProp;23;3;;Tube;0.05;0.004\n"
        "BushProp;30;Stiffness;1.E+3;1.E+3;1.E+3;1.E+3;1.E+3;1.E+3\n"
        "DesignVar;1;X;1.\n"
        "DesignPropRelation1;1;BarProp;20;area(EndB);;;;1;1.\n"
        "DesignPropRelation1;2;ShellProp;10;bendingStiffnessRatio;;;;1;1.\n"
        "DesignPropRelation1;3;ShellProp;10;nonStructMass;;;;1;1.\n"
        "DesignPropRelation1;4;BeamProp;22;area(EndB);;;;1;1.\n"
        "DesignPropRelation1;5;BeamShapeProp;23;sectionGroup;;;;1;1.\n"
        "DesignPropRelation1;6;BushProp;30;stiffness1;;;;1;1.\n"
        "DesignPropRelation1;7;ShellProp;10;membraneMatId;;;;1;1.\n");
    const DesignModel design = DesignModel::read(model);
    const auto problem = [&](base::EntityId id) { return design.relations()[id - 1].problem; };
    REQUIRE_THAT(problem(1), ContainsSubstring("BarProp has no field named area(EndB)"));
    REQUIRE(design.relations()[1].applicable);
    REQUIRE(design.relations()[2].applicable); // field 9 is the NSM
    REQUIRE_THAT(problem(4), ContainsSubstring("no station at X/XB = 1.0"));
    REQUIRE_THAT(problem(5), ContainsSubstring("BeamShapeProp has no field named sectionGroup"));
    REQUIRE_THAT(problem(6), ContainsSubstring("cannot design the fields of a BushProp"));
    REQUIRE_THAT(problem(7), ContainsSubstring("ShellProp has no field named membraneMatId"));
}

TEST_CASE("the fields of a material and of an element", "[model][design]")
{
    Model model = readBulk(GRIDS +
                           "IsoMat;3;2.1E+11;;0.3\n"
                           "PointMassInertia;40;1;;2.\n"
                           "SpringDofInline;41;1.E+3;1;1\n"
                           "ShellQuad4;42;10;1;2;3;4\n"
                           "ShellProp;10;3;0.01\n"
                           "DesignVar;1;X;1.\n"
                           "DesignMatRelation1;1;IsoMat;3;youngsModulus;;;;1;1.E+11\n"
                           "DesignElemRelation1;2;PointMassInertia;40;mass;;;;1;3.\n"
                           "DesignElemRelation1;3;SpringDofInline;41;stiffness;;;;1;2.\n"
                           "DesignElemRelation1;4;ShellQuad4;42;offset;;;0.1;1;0.5\n");
    const DesignModel design = DesignModel::read(model);
    for (const DesignRelation& relation : design.relations())
    {
        INFO(relation.card << " " << relation.id << ": " << relation.problem);
        REQUIRE(relation.applicable);
    }
    (void)applyDesign(model, design, pointOf(design, {{1, 2.0}}));
    const IsoMatCard material = cardAt<IsoMatTable>(model);
    REQUIRE(material.youngsModulus == Approx(2.0e11));
    // G was E/(2 (1 + NU)) for the old E and NU, so it is that for the new E
    REQUIRE(material.shearModulus == Approx(2.0e11 / (2.0 * 1.3)));
    REQUIRE(cardAt<PointMassInertiaTable>(model).mass == Approx(6.0));
    REQUIRE(cardAt<SpringDofInlineTable>(model).stiffness == Approx(4.0));
    REQUIRE(cardAt<ShellQuad4Table>(model).offset == Approx(1.1));
}

TEST_CASE("two relations on one field are an error, and so are two DLINK cards for one variable", "[model][design]")
{
    {
        Model model = readBulk(
            "IsoMat;3;2.1E+11;;0.3\n"
            "ShellProp;10;3;0.01\n"
            "DesignVar;1;X;1.\n"
            "DesignPropRelation1;1;ShellProp;10;thickness;;;;1;1.\n"
            "DesignPropRelation1;2;ShellProp;10;thickness;;;;1;2.\n");
        const DesignModel design = DesignModel::read(model);
        REQUIRE_THROWS_WITH(
            applyDesign(model, design, design.initialPoint()),
            ContainsSubstring("DesignPropRelation1 1 and DesignPropRelation1 2 both drive ShellProp 10 thickness"));
    }
    {
        Model model = readBulk(
            "DesignVar;1;X;1.\nDesignVar;2;Y;1.\nDesignVar;3;Z;1.\nDesignVarLink;1;3;0.;1.;1;1.\nDesignVarLink;2;3;0.;"
            "1.;2;1.\n");
        const DesignModel design = DesignModel::read(model);
        REQUIRE_FALSE(design.problems().empty());
        REQUIRE_THROWS_AS(design.evaluate(model, design.initialPoint(), true), DesignError);
    }
}

TEST_CASE("a DVGRID is a shape term of the design, and a ply by its global ID is not applicable", "[model][design]")
{
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "IsoMat;3;2.1E+11;;0.3\n"
        "DesignVar;1;X;1.\n"
        "DesignNodeMove;1;1;;1.;1.;0.;0.\n"
        "DesignPropRelation1;1;GlobalPly;100;t;;;;1;1.\n");
    const DesignModel design = DesignModel::read(model);
    REQUIRE(design.notApplied().empty());
    REQUIRE(design.shapeTerms().size() == 1);
    REQUIRE((design.shapeTerms()[0].variable == 1 && design.shapeTerms()[0].grid == 1));
    REQUIRE(design.shapeTerms()[0].coefficient == 1.0);
    REQUIRE(design.isUsed(1));
    REQUIRE_THAT(design.relations()[0].problem, ContainsSubstring("kind GlobalPly"));
}

TEST_CASE("TOMVAR, BEADVAR, DVLREL1 and DVPSURF are listed as not applied, with the variables of the last two used",
          "[model][design]")
{
    // The regions of a topometry and a topography optimisation name no design variable (the solver makes theirs); a
    // designed load and a designed control surface setting name one each. DESVAR 3 is named by nothing.
    Model model = readBulk(
        "IsoMat;3;2.1E+11;;0.3\n"
        "ShellProp;10;3;0.01\n"
        "DesignVar;1;X;1.\n"
        "DesignVar;2;Y;1.\n"
        "DesignVar;3;Z;1.\n"
        "DesignTopometryRegion;7;ShellProp;10;thickness;0.5\n"
        "DesignBeadRegion;8;ShellProp;10;1.;2.;70.\n"
        "DesignLoadRelation1;9;PointForceLoad;300;magnitude;;;;;;;;;1;2.\n"
        "DesignControlSurface;11;FLAP;5;2;0.5\n");
    const DesignModel design = DesignModel::read(model);
    // Not applied: each card with its ID and a reason that says what it is. They are left alone.
    REQUIRE(design.notApplied().size() == 4);
    REQUIRE((design.notApplied()[0].card == "DesignBeadRegion" && design.notApplied()[0].id == 8));
    REQUIRE((design.notApplied()[1].card == "DesignTopometryRegion" && design.notApplied()[1].id == 7));
    REQUIRE_THAT(design.notApplied()[1].reason, ContainsSubstring("topometry"));
    REQUIRE((design.notApplied()[2].card == "DesignLoadRelation1" && design.notApplied()[2].id == 9));
    REQUIRE_THAT(design.notApplied()[2].reason, ContainsSubstring("PointForceLoad"));
    REQUIRE_THAT(design.notApplied()[2].reason, ContainsSubstring("300"));
    REQUIRE((design.notApplied()[3].card == "DesignControlSurface" && design.notApplied()[3].id == 11));
    REQUIRE_THAT(design.notApplied()[3].reason, ContainsSubstring("FLAP"));
    REQUIRE(design.relations().empty());
    // The variable of a DVLREL1 term and the one of a DVPSURF are used, the third is not.
    REQUIRE(design.isUsed(1));
    REQUIRE(design.isUsed(2));
    REQUIRE_FALSE(design.isUsed(3));
    // Applying the initial point changes nothing in the model: the cards are only listed.
    const std::string before = writeModelText(model);
    (void)applyDesign(model, design, design.initialPoint());
    REQUIRE(writeModelText(model) == before);
}

TEST_CASE("the values of the variables out of their bounds are reported and the point is applied", "[model][design]")
{
    Model model = readBulk(
        "IsoMat;3;2.1E+11;;0.3\n"
        "ShellProp;10;3;0.01\n"
        "DesignVar;1;T;0.05;0.01;0.1\n"
        "DesignPropRelation1;1;ShellProp;10;thickness;;;;1;1.\n");
    const DesignModel design = DesignModel::read(model);
    const DesignUpdate update = applyDesign(model, design, pointOf(design, {{1, 0.5}}));
    REQUIRE(update.variableBounds == std::vector<std::int8_t>{1});
    REQUIRE(cardAt<ShellPropTable>(model).thickness == Approx(0.5));
    const DesignModel again = DesignModel::read(model); // the model changed: its design is read again
    const DesignUpdate below = applyDesign(model, again, pointOf(again, {{1, 0.001}}));
    REQUIRE(below.variableBounds == std::vector<std::int8_t>{-1});
}

TEST_CASE("setting the initial values gives the DESVAR cards the design point, the dependent ones included",
          "[model][design]")
{
    Model model = readBulk(DECK);
    const DesignModel design = DesignModel::read(model);
    DesignApplyOptions options;
    options.setInitialValues = true;
    (void)applyDesign(model, design, pointOf(design, {{1, 0.06}, {2, 0.2}, {3, 3.0}}), options);
    const DesignVarTable& table = *model.findTable<DesignVarTable>();
    REQUIRE(table.card(0).initialValue == Approx(0.06));
    REQUIRE(table.card(3).initialValue == Approx(0.5 + 2.0 * (0.06 + 0.6))); // the DLINK value
}

TEST_CASE("a design that changed since it was read is refused", "[model][design]")
{
    Model model = readBulk(DECK);
    const DesignModel design = DesignModel::read(model);
    model.noteContentChange();
    REQUIRE_THROWS_AS(applyDesign(model, design, design.initialPoint()), DesignError);
    REQUIRE_THROWS_AS(DesignModel::read(model).evaluate(model, DesignPoint{{1.0}}, true), DesignError);
}
