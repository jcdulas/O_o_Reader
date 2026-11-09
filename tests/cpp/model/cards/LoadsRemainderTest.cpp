// Checks of the remaining loads and temperatures: FORCE1, FORCE2, MOMENT1, MOMENT2, PLOAD, PLOAD1, PLOAD2, PLOADX1,
// SLOAD, RFORCE, LSEQ, LOADCYN, LOADCYH, LOADCYT, TEMPD, TEMPP1, TEMPRB, TEMPAX, DEFORM, FORCEAX, ACSRCE, NOLIN1 to
// NOLIN4, and the forms of PLOAD4 and GRAV: field positions, defaults, variants, references, renumbering and rules.

#include "oor/model/cards/dynamics/acousticSourceLoad/AcousticSourceLoadTable.hpp"
#include "oor/model/cards/dynamics/nonlinearNegativePowerLoad4/NonlinearNegativePowerLoad4Table.hpp"
#include "oor/model/cards/dynamics/nonlinearPositivePowerLoad3/NonlinearPositivePowerLoad3Table.hpp"
#include "oor/model/cards/dynamics/nonlinearProductLoad2/NonlinearProductLoad2Table.hpp"
#include "oor/model/cards/dynamics/nonlinearTableLoad1/NonlinearTableLoad1Table.hpp"
#include "oor/model/cards/loads/axialDeformationLoad/AxialDeformationLoadTable.hpp"
#include "oor/model/cards/loads/axisymConeTemperatureLoad/AxisymConeTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/axisymRingForceLoad/AxisymRingForceLoadTable.hpp"
#include "oor/model/cards/loads/axisymTractionLoad/AxisymTractionLoadTable.hpp"
#include "oor/model/cards/loads/beamDistributedLoad/BeamDistributedLoadTable.hpp"
#include "oor/model/cards/loads/cyclicAzimuthTableLoad/CyclicAzimuthTableLoadTable.hpp"
#include "oor/model/cards/loads/cyclicHarmonicLoad/CyclicHarmonicLoadTable.hpp"
#include "oor/model/cards/loads/cyclicSegmentLoad/CyclicSegmentLoadTable.hpp"
#include "oor/model/cards/loads/defaultTemperatureLoad/DefaultTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"
#include "oor/model/cards/loads/gravityLoad/GravityLoadTable.hpp"
#include "oor/model/cards/loads/lineElemTemperatureLoad/LineElemTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/plateTemperatureLoad/PlateTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad1/PointForceLoad1Table.hpp"
#include "oor/model/cards/loads/pointForceLoad2/PointForceLoad2Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad1/PointMomentLoad1Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad2/PointMomentLoad2Table.hpp"
#include "oor/model/cards/loads/pointsPressureLoad/PointsPressureLoadTable.hpp"
#include "oor/model/cards/loads/rotationalForceLoad/RotationalForceLoadTable.hpp"
#include "oor/model/cards/loads/scalarPointLoad/ScalarPointLoadTable.hpp"
#include "oor/model/cards/loads/shellPressureLoad/ShellPressureLoadTable.hpp"
#include "oor/model/cards/loads/staticLoadSequence/StaticLoadSequenceTable.hpp"
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

// Nodes 1 to 40 and elements 1 to 20 so that the loads below have something to refer to.
std::string grids()
{
    std::string text;
    for (int id = 1; id <= 40; ++id)
        text += "Node;" + std::to_string(id) + ";;0.;0.;" + std::to_string(id) + ".\n";
    return text;
}

std::string rods()
{
    std::string text = grids() + "IsoMat;1;2.1E+11;;0.3\nRodProp;1;1;1.E-4\n";
    for (int id = 1; id <= 20; ++id)
        text += "Rod2;" + std::to_string(id) + ";1;" + std::to_string(id) + ";" + std::to_string(id + 1) + "\n";
    return text;
}

} // namespace

// ------------------------------------------------------------------------------------------------ point loads

TEST_CASE("FORCE1, FORCE2, MOMENT1 and MOMENT2 name their points by field and refuse coincident ones", "[model][cards]")
{
    const Model model = readBulk(
        "PointForceLoad1;6;21;-3.45;24;21\nPointForceLoad2;7;21;-3.45;24;21;27;19\nPointMomentLoad1;8;21;1.8;24;"
        "18\nPointMomentLoad2;9;21;2.5;24;21;27;19");
    const PointForceLoad1Card force1 = cardAt<PointForceLoad1Table>(model);
    REQUIRE(force1.id == 6);
    REQUIRE(force1.node == 21);
    REQUIRE(force1.magnitude == Approx(-3.45));
    REQUIRE((force1.node1 == 24 && force1.node2 == 21));
    const PointForceLoad2Card force2 = cardAt<PointForceLoad2Table>(model);
    REQUIRE((force2.node1 == 24 && force2.node2 == 21 && force2.node3 == 27 && force2.node4 == 19));
    REQUIRE(cardAt<PointMomentLoad1Table>(model).magnitude == Approx(1.8));
    REQUIRE(cardAt<PointMomentLoad2Table>(model).node4 == 19);
    requireRoundTrip(model);

    expectInvalid("PointForceLoad1;1;1;1.;3;3", "node2");
    expectInvalid("PointForceLoad1;1;1;1.;3", "node2");
    expectInvalid("PointForceLoad1;0;1;1.;2;3", "id");
    expectInvalid("PointForceLoad2;1;1;1.;3;3;4;5", "node2");
    expectInvalid("PointForceLoad2;1;1;1.;3;4;5;5", "node4");
    expectInvalid("PointForceLoad2;1;1;1.;3;4;5", "node4");
    expectInvalid("PointMomentLoad1;1;1;1.;3;3", "node2");
    expectInvalid("PointMomentLoad1;1;1;;3;4", "magnitude");
    expectInvalid("PointMomentLoad2;1;1;1.;3;4;5;5", "node4");
}

TEST_CASE("the points of FORCE1, FORCE2, MOMENT1 and MOMENT2 are node references that renumber and resolve",
          "[model][cards]")
{
    Model model = readBulk(grids() +
                           "PointForceLoad1;6;21;-3.45;24;21\n"
                           "PointForceLoad2;7;21;-3.45;24;21;27;19\n"
                           "PointMomentLoad1;8;21;1.8;24;18\n"
                           "PointMomentLoad2;9;21;2.5;24;21;27;19");
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor visitor;
    model.table<PointForceLoad2Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"node", "node1", "node2", "node3", "node4"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(5, ReferenceTarget::Node));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    const PointForceLoad1Card force1 = cardAt<PointForceLoad1Table>(model);
    REQUIRE((force1.node == 121 && force1.node1 == 124 && force1.node2 == 121));
    REQUIRE(cardAt<PointMomentLoad2Table>(model).node4 == 119);
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("PointForceLoad1;6;21;-3.45;24;21");
    REQUIRE(unresolvedIn(lacking) == 3);
    // The set ID renumbers with the load sets.
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(1000));
    REQUIRE(cardAt<PointForceLoad1Table>(model).id == 1006);
    REQUIRE(cardAt<PointMomentLoad2Table>(model).id == 1009);
}

TEST_CASE("PLOAD is a triangle or a quadrilateral and its points are nodes", "[model][cards]")
{
    Model model =
        readBulk(grids() +
                 "PointsPressureLoad;1;-6.;18;29;14\nPointsPressureLoad;2;9.;1;2;7;6\nPointsPressureLoad;3;9.;1;2;7;0");
    const PointsPressureLoadCard triangle = cardAt<PointsPressureLoadTable>(model, 0);
    REQUIRE(triangle.isTriangle());
    REQUIRE(triangle.pressure == Approx(-6.0));
    REQUIRE((triangle.nodes == std::array<base::EntityId, 4>{18, 29, 14, 0}));
    REQUIRE_FALSE(cardAt<PointsPressureLoadTable>(model, 1).isTriangle());
    REQUIRE(cardAt<PointsPressureLoadTable>(model, 2).isTriangle()); // G4 = 0 is a blank
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<PointsPressureLoadTable>(model, 0).nodes == std::array<base::EntityId, 4>{118, 129, 114, 0}));
    REQUIRE(cardAt<PointsPressureLoadTable>(model, 1).nodes[3] == 106);
    requireRoundTrip(model);
    // A blank G4 is not written.
    REQUIRE(
        writeModelText(readBulk("PointsPressureLoad;1;-6.;18;29;14;0")).find("PointsPressureLoad;1;-6.;18;29;14\n") !=
        std::string::npos);

    expectInvalid("PointsPressureLoad;1;1.;1;2", "node3");
    expectInvalid("PointsPressureLoad;1;1.;1;2;2", "node3");
    expectInvalid("PointsPressureLoad;1;1.;1;2;3;3", "node4");
    expectInvalid("PointsPressureLoad;1;;1;2;3", "pressure");
    expectInvalid("PointsPressureLoad;0;1.;1;2;3", "id");
}

TEST_CASE(
    "PLOAD1 has its type and scale words, a blank X2 takes X1, a blank P2 stays blank, and its element is a reference",
    "[model][cards]")
{
    Model model = readBulk(rods() +
                           "BeamDistributedLoad;25;1072;MomentY;FractionalProjectedLength;0.3;1800.;0.7;4200.\n"
                           "BeamDistributedLoad;26;10;ForceX;ActualLength;1.;5.\n"
                           "BeamDistributedLoad;27;11;ForceYAlongLength;FractionalLength;0.;-20.;1.;-20.\n"
                           "BeamDistributedLoad;28;12;MomentZ;ProjectedLength;0.5;1.;2.5");
    BeamDistributedLoadCard card = cardAt<BeamDistributedLoadTable>(model, 0);
    REQUIRE(card.kind == BarLoadType::MY);
    REQUIRE(card.scaleFactor == BarLoadScale::FRPR);
    REQUIRE((card.xCoord1 == Approx(0.3) && card.pressure1 == Approx(1800.0) && card.xCoord2 == Approx(0.7) &&
             card.pressure2 && *card.pressure2 == Approx(4200.0)));
    card = cardAt<BeamDistributedLoadTable>(model, 1);
    REQUIRE(card.isConcentrated());
    REQUIRE((card.xCoord2 == 1.0 && !card.pressure2 && card.effectiveP2() == 5.0)); // concentrated: P2 plays no part
    REQUIRE(cardAt<BeamDistributedLoadTable>(model, 2).kind == BarLoadType::FYE);
    REQUIRE(cardAt<BeamDistributedLoadTable>(model, 2).scaleFactor ==
            BarLoadScale::FR); // the words are not case sensitive
    card = cardAt<BeamDistributedLoadTable>(model, 3);
    REQUIRE((card.xCoord2 == 2.5 && !card.pressure2 && card.effectiveP2() == 0.0)); // as the solver reads it
    // X2 depends on X1: always written; a blank P2 is written blank.
    REQUIRE(writeModelText(readBulk("BeamDistributedLoad;26;10;ForceX;ActualLength;1.;5."))
                .find("BeamDistributedLoad;26;10;ForceX;ActualLength;1.;5.;1.\n") != std::string::npos);
    REQUIRE(unresolvedIn(model) == 1); // the CROD 1072 does not exist
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(cardAt<BeamDistributedLoadTable>(model, 1).elemId == 110);
    requireRoundTrip(model);

    expectInvalid("BeamDistributedLoad;1;1;FW;ActualLength;1.;1.", "kind");
    expectInvalid("BeamDistributedLoad;1;1;ForceX;XX;1.;1.", "scaleFactor");
    expectInvalid("BeamDistributedLoad;1;1;ForceX;ActualLength;1.", "pressure1");
    expectInvalid("BeamDistributedLoad;1;1;ForceX;ActualLength;2.;1.;1.", "xCoord2");
    expectInvalid("BeamDistributedLoad;1;1;ForceX;ActualLength;-1.;1.", "xCoord1");
    expectInvalid("BeamDistributedLoad;1;1;ForceX;FractionalLength;0.5;1.;1.5", "xCoord2");
}

TEST_CASE("PLOAD2 lists elements over its continuation lines, or one THRU range, and its elements are references",
          "[model][cards]")
{
    Model model =
        readBulk(rods() +
                 "ShellPressureLoad;21;-2.7;5;13;3\nShellPressureLoad;1;27.5;3;Through;8\nShellPressureLoad;3;1."
                 "5;1;2;3;4;5;6;7;8;9;10");
    REQUIRE(cardAt<ShellPressureLoadTable>(model, 0).elements == std::vector<ElementRun>{{5, 5}, {13, 13}, {3, 3}});
    REQUIRE(cardAt<ShellPressureLoadTable>(model, 1).elements == std::vector<ElementRun>{{3, 8}});
    REQUIRE(cardAt<ShellPressureLoadTable>(model, 2).elements.size() == 10);
    REQUIRE(cardAt<ShellPressureLoadTable>(model, 2).elements.back().first == 10);
    REQUIRE(unresolvedIn(model) == 0);
    // Elements that do not exist are reported (a single one), the ends of a THRU range are let be (a solver skips the
    // gaps).
    Model lacking = readBulk(rods() + "ShellPressureLoad;1;27.5;16;99\nShellPressureLoad;2;1.;15;Through;30");
    REQUIRE(unresolvedIn(lacking) == 1);
    // A range keeps its ends in a renumbering and its single elements follow it.
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(cardAt<ShellPressureLoadTable>(model, 0).elements ==
            std::vector<ElementRun>{{105, 105}, {113, 113}, {103, 103}});
    REQUIRE(cardAt<ShellPressureLoadTable>(model, 1).elements == std::vector<ElementRun>{{103, 108}});
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    // The THRU form is a list of its own: nothing else may follow it, and the writer compresses only a whole range.
    expectInvalid("ShellPressureLoad;1;1.;3;Through;8;10", "elemId");
    expectInvalid("ShellPressureLoad;1;1.;3;Through", "Through");
    expectInvalid("ShellPressureLoad;1;1.;8;Through;3", "elemId");
    expectInvalid("ShellPressureLoad;1;1.", "elemId1");
    expectInvalid("ShellPressureLoad;1;;1", "pressure");
    expectInvalid("ShellPressureLoad;1;1.;Through;5", "Through");
}

TEST_CASE("PLOADX1 has a PB that defaults to PA and an angle", "[model][cards]")
{
    Model model = readBulk(
        grids() +
        "IsoMat;1;2.1E+11;;0.3\nAxisymTractionLoad;200;35;4.5;9.5;12;28;15.\nAxisymTractionLoad;201;36;4.5;;12;28");
    AxisymTractionLoadCard card = cardAt<AxisymTractionLoadTable>(model, 0);
    REQUIRE((card.tractionA == 4.5 && card.tractionB == 9.5 && card.nodeA == 12 && card.nodeB == 28 &&
             card.tractionAngle == 15.0));
    card = cardAt<AxisymTractionLoadTable>(model, 1);
    REQUIRE((card.tractionB == 4.5 && card.tractionAngle == 0.0));
    // PB depends on PA: written; THETA is a static default: omitted.
    REQUIRE(writeModelText(readBulk("AxisymTractionLoad;201;36;4.5;;12;28"))
                .find("AxisymTractionLoad;201;36;4.5;4.5;12;28\n") != std::string::npos);
    REQUIRE(unresolvedIn(model) == 2); // the elements 35 and 36
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<AxisymTractionLoadTable>(model, 0).nodeA == 112 &&
             cardAt<AxisymTractionLoadTable>(model, 0).nodeB == 128));
    requireRoundTrip(model);
    // The test of an element by number: PLOADX1 names it as an element.
    RecordingVisitor visitor;
    model.table<AxisymTractionLoadTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"elemId", "nodeA", "nodeB"});
    REQUIRE(visitor.targets[0] == ReferenceTarget::Element);

    expectInvalid("AxisymTractionLoad;1;1;1.;;5;5", "nodeB");
    expectInvalid("AxisymTractionLoad;1;1;;;5;6", "tractionA");
    expectInvalid("AxisymTractionLoad;1;1;1.;;5", "nodeB");
}

TEST_CASE("SLOAD has one to three loads on scalar or grid points", "[model][cards]")
{
    Model model = readBulk(grids() +
                           "ScalarPoint;100\nScalarPointLoad;16;3;4.2;19;-5.1;100;-1.75\nScalarPointLoad;17;5;1."
                           "5\nScalarPointLoad;18;500;2.");
    const ScalarPointLoadCard card = cardAt<ScalarPointLoadTable>(model, 0);
    REQUIRE(card.loads == std::vector<PointLoad>{{3, 4.2}, {19, -5.1}, {100, -1.75}});
    REQUIRE(cardAt<ScalarPointLoadTable>(model, 1).loads.size() == 1);
    // A scalar point that no SPOINT defines may still be named (a scalar element defines its points that way).
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<ScalarPointLoadTable>(model, 0).loads[2].node == 1100);
    requireRoundTrip(model);

    expectInvalid("ScalarPointLoad;1;2;1.;3;2.;4;3.;5;4.", "scalarPoint3"); // four loads: at most three
    expectInvalid("ScalarPointLoad;1;2", "magnitude");
    expectInvalid("ScalarPointLoad;1", "scalarPoint1");
    expectInvalid("ScalarPointLoad;0;2;1.", "id");
    expectInvalid("ScalarPointLoad;1;0;1.", "scalarPoint1");
}

TEST_CASE("RFORCE has its fields on two lines, defaults, a METHOD of 1 or 2 and a rotation vector", "[model][cards]")
{
    Model model = readBulk(grids() +
                           "RectFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
                           "RotationalForceLoad;2;5;;-5.2;0.;0.;1.;2;1.\n"
                           "RotationalForceLoad;3;;;1.;0.;0.;1.\n"
                           "RotationalForceLoad;4;7;3;0.5;0.;1.;0.;1;0.25;-1;9\n"
                           "RotationalForceLoad;5\n");
    RotationalForceLoadCard card = cardAt<RotationalForceLoadTable>(model, 0);
    REQUIRE((card.node == 5 && card.frameId == 0 && card.magnitude == Approx(-5.2) &&
             card.axis == std::array<double, 3>{0.0, 0.0, 1.0}));
    REQUIRE((card.methodName == 2 && card.angularAcceleration == 1.0 && card.mainBulkFlag == 0 && card.regionId == 0));
    card = cardAt<RotationalForceLoadTable>(model, 1);
    REQUIRE((card.node == 0 && card.methodName == 1 && card.angularAcceleration == 0.0));
    card = cardAt<RotationalForceLoadTable>(model, 2);
    REQUIRE((card.frameId == 3 && card.methodName == 1 && card.angularAcceleration == 0.25 && card.mainBulkFlag == -1 &&
             card.regionId == 9));
    // Every field blank but the set: no rotation at all (the vector is zero, A and RACC are).
    REQUIRE(cardAt<RotationalForceLoadTable>(model, 3).magnitude == 0.0);
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<RotationalForceLoadTable>(model, 2).frameId == 13);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<RotationalForceLoadTable>(model, 0).node == 105);
    REQUIRE(cardAt<RotationalForceLoadTable>(model, 1).node == 0); // the origin stays
    requireRoundTrip(model);

    expectInvalid("RotationalForceLoad;1;;;1.;0.;0.;1.;3", "methodName");
    expectInvalid("RotationalForceLoad;1;;;1.;0.;0.;0.", "radius1");
    expectInvalid("RotationalForceLoad;1;;;;0.;0.;0.;;2.", "radius1"); // RACC without an axis
    expectInvalid("RotationalForceLoad;1;-1;;1.;0.;0.;1.", "node");
}

// ------------------------------------------------------------------------------------------------ sets of loads

TEST_CASE("LSEQ names a set of loads and a set of temperatures, and its EXCITEID is what a dynamic load names",
          "[model][cards]")
{
    const std::string cards = grids() +
                              "PointForceLoad;1300;5;;1.;0.;0.;1.\n"
                              "NodeTemperatureLoad;1301;5;100.\n"
                              "DynamicLoadScale;50;5;1;1.\n"
                              "StaticLoadSequence;100;240;1300;1301\n"
                              "StaticLoadSequence;100;241;1300\n"
                              "StaticLoadSequence;101;242;;1301\n"
                              "TransientTableLoad1;7;240;;;0;9\n"
                              "FreqResponseLoad1;8;241;;;;;9\n"
                              "FreqResponseLoad2;9;50;;;;;9\n"
                              "TableDynamic1;9;;;0.;1.;1.;2.;EndTable";
    Model model = readBulk(cards);
    const StaticLoadSequenceCard card = cardAt<StaticLoadSequenceTable>(model, 0);
    REQUIRE((card.id == 100 && card.excitationId == 240 && card.loadId == 1300 && card.tableId == 1301));
    REQUIRE(cardAt<StaticLoadSequenceTable>(model, 1).tableId == 0);
    REQUIRE(cardAt<StaticLoadSequenceTable>(model, 2).loadId == 0);
    // The EXCITEID 240 and 241 are the ones the LSEQ cards define (there is no DAREA or load set of that number); the
    // load and temperature sets resolve as load sets.
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk(grids() +
                             "StaticLoadSequence;100;240;1300;1301\nTransientTableLoad1;7;340;;;0;9\nTableDynamic1;9;;;"
                             "0.;1.;1.;2.;EndTable");
    REQUIRE(unresolvedIn(lacking) == 3); // LID, TID and the EXCITEID 340
    RecordingVisitor visitor;
    model.table<StaticLoadSequenceTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"excitationId", "loadId", "tableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ExcitationDefined,
                                                            ReferenceTarget::LoadCombinationSet,
                                                            ReferenceTarget::LoadCombinationSet});
    // Renumbering the load sets renumbers LID and TID with the loads and the temperatures; the SID is a set of its own.
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(5000));
    REQUIRE(cardAt<StaticLoadSequenceTable>(model, 0).loadId == 6300);
    REQUIRE(cardAt<StaticLoadSequenceTable>(model, 0).tableId == 6301);
    REQUIRE(cardAt<StaticLoadSequenceTable>(model, 0).id == 5100);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    // A blank LID and TID are accepted (a superelement analysis).
    REQUIRE_NOTHROW(readBulk("StaticLoadSequence;100;240"));
    expectInvalid("StaticLoadSequence;100", "excitationId");
    expectInvalid("StaticLoadSequence;0;240;1", "id");
    expectInvalid("StaticLoadSequence;1;0;1", "excitationId");
}

TEST_CASE("the Case Control commands LOADSET, DEFORM, TEMPERATURE and NONLINEAR resolve to the cards of the step",
          "[model][cards]")
{
    const std::string control =
        "[case]\nSubcaseStart;1\nLoadSequenceSelection;100\nDeformationLoadSelection;2\nTemperatureSelection;7;"
        "TemperatureLoadPart\nNonlinearLoadSelection;5\n[model]\n";
    const std::string bulk = rods() +
                             "StaticLoadSequence;100;200;1000\nPointForceLoad;1000;5;;1.;0.;0.;1."
                             "\nAxialDeformationLoad;2;1;0.1\nDefaultTemperatureLoad;7;20.\n" +
                             "NonlinearTableLoad1;5;1;;1.;2;;9\nTableDynamic1;9;;;0.;1.;1.;2.;EndTable\n";
    Model model = readModelText(control + bulk, true);
    REQUIRE(unresolvedIn(model) == 0);
    // Missing sets are reported, as warnings, because the command may name a card of a type that is not read.
    Model lacking = readModelText(control + grids(), true);
    REQUIRE(unresolvedIn(lacking) == 4);
    // The commands follow a renumbering of the sets.
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(10));
    Renumbering::apply(model, RenumberSpace::DynamicLoadSet, Renumbering::offset(10));
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(cardAt<StaticLoadSequenceTable>(model).id == 110);
    REQUIRE(cardAt<AxialDeformationLoadTable>(model).id == 12);
    REQUIRE(cardAt<NonlinearTableLoad1Table>(model).id == 15);
    REQUIRE(cardAt<DefaultTemperatureLoadTable>(model).id == 17);
}

TEST_CASE("LOADCYN, LOADCYH and LOADCYT have their fields and rules", "[model][cards]")
{
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "PointForceLoad;23;1;;1.;1.;0.;0.\n"
        "PointForceLoad;14;1;;1.;1.;0.;0.\n"
        "DynamicLoadScale;22;1;1;1.\n"
        "TableDynamic1;31;;;0.;1.;1.;2.;EndTable\n"
        "TableDynamic1;33;;;0.;1.;1.;2.;EndTable\n"
        "NodeTemperatureLoad;36;1;1.\n"
        "NodeTemperatureLoad;35;1;1.\n"
        "CyclicSegmentLoad;10;1.;1;Right;0.5;23\n"
        "CyclicSegmentLoad;11;2.;3;Left;0.5;23;0.25;22\n"
        "CyclicSegmentLoad;12;1.;2;;1.;5\n"
        "CyclicHarmonicLoad;10;1.;6;Cosine;0.5;14\n"
        "CyclicHarmonicLoad;11;1.;;GravityLoad;1.;5\n"
        "CyclicHarmonicLoad;12;2.;0;SymmetricStar;0.5;14;0.25;24\n"
        "CyclicAzimuthTableLoad;10;31;36;;33;35;1\n"
        "CyclicAzimuthTableLoad;11;31;36\n"
        "CyclicAzimuthTableLoad;12;31;36;1");
    CyclicSegmentLoadCard cyn = cardAt<CyclicSegmentLoadTable>(model, 0);
    REQUIRE((cyn.scaleFactor == 1.0 && cyn.segmentId == 1 && cyn.segmentType == SegmentType::Right &&
             cyn.scaleFactor1 == 0.5 && cyn.loadSetId1 == 23));
    REQUIRE_FALSE(cyn.hasSecond());
    cyn = cardAt<CyclicSegmentLoadTable>(model, 1);
    REQUIRE((cyn.segmentType == SegmentType::Left && cyn.scaleFactor2 == 0.25 && cyn.loadSetId2 == 22));
    REQUIRE(cardAt<CyclicSegmentLoadTable>(model, 2).segmentType == SegmentType::Both);
    CyclicHarmonicLoadCard cyh = cardAt<CyclicHarmonicLoadTable>(model, 0);
    REQUIRE((cyh.harmonicId == 6 && cyh.harmonicType == HarmonicType::C && cyh.loadSetId1 == 14));
    cyh = cardAt<CyclicHarmonicLoadTable>(model, 1);
    REQUIRE((cyh.harmonicId == CyclicHarmonicLoadCard::NO_HARMONIC && cyh.harmonicType == HarmonicType::GravityLoad));
    cyh = cardAt<CyclicHarmonicLoadTable>(model, 2);
    REQUIRE((cyh.harmonicId == 0 && cyh.harmonicType == HarmonicType::SStar &&
             cyh.loadSetId2 == 24)); // the harmonic 0 is not a blank
    CyclicAzimuthTableLoadCard cyt = cardAt<CyclicAzimuthTableLoadTable>(model, 0);
    REQUIRE((cyt.tableId1 == 31 && cyt.loadSetId1 == 36 && cyt.methodName1 == 0 && cyt.tableId2 == 33 &&
             cyt.loadSetId2 == 35 && cyt.methodName2 == 1));
    REQUIRE_FALSE(cardAt<CyclicAzimuthTableLoadTable>(model, 1).hasSecond());
    REQUIRE(cardAt<CyclicAzimuthTableLoadTable>(model, 2).methodName1 == 1);
    // Their load sets are loads or dynamic loads; the sets 5 and 24 and 22 do not exist.
    REQUIRE(unresolvedIn(model) == 3);
    requireRoundTrip(model);
    // The tables of a LOADCYT are dynamic tables, its load sets are load sets: both are renumbered with them.
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE((cardAt<CyclicAzimuthTableLoadTable>(model, 0).tableId1 == 131 &&
             cardAt<CyclicAzimuthTableLoadTable>(model, 0).tableId2 == 133));
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(1000));
    REQUIRE(cardAt<CyclicAzimuthTableLoadTable>(model, 0).loadSetId1 == 1036);
    REQUIRE(cardAt<CyclicSegmentLoadTable>(model, 0).loadSetId1 == 1023);

    expectInvalid("CyclicSegmentLoad;1;0.;1;Right;1.;5", "scaleFactor");
    expectInvalid("CyclicSegmentLoad;1;1.;1;Right;0.;5", "scaleFactor1");
    expectInvalid("CyclicSegmentLoad;1;1.;1;X;1.;5", "segmentType");
    expectInvalid("CyclicSegmentLoad;1;1.;1;Right;1.;5;2.", "loadSetId2");
    expectInvalid("CyclicSegmentLoad;1;1.;1;Right;1.", "loadSetId1");
    expectInvalid("CyclicHarmonicLoad;1;1.;1;XX;1.;5", "harmonicType");
    expectInvalid("CyclicHarmonicLoad;1;1.;;GravityLoad;1.;5;1.;6",
                  "scaleFactor2"); // GRAV and RFORCE have no second load set
    expectInvalid("CyclicHarmonicLoad;1;0.;1;Cosine;1.;5", "scaleFactor");
    expectInvalid("CyclicAzimuthTableLoad;1;31", "loadSetId1");
    expectInvalid("CyclicAzimuthTableLoad;1;31;36;2", "methodName1");
    expectInvalid("CyclicAzimuthTableLoad;1;31;36;;33", "loadSetId2");
    expectInvalid("CyclicAzimuthTableLoad;1;31;36;;;;1", "methodName2");
}

// ------------------------------------------------------------------------------------------------ temperatures

TEST_CASE("TEMPD gives one default temperature per pair, each for the set of its own", "[model][cards]")
{
    Model model = readBulk(
        "DefaultTemperatureLoad;1;208.4;2;300.;3;400.;4;500.\nDefaultTemperatureLoad;5;20.\nNodeTemperatureLoad;1;3;"
        "100.");
    REQUIRE(rowCount<DefaultTemperatureLoadTable>(model) == 5);
    REQUIRE((cardAt<DefaultTemperatureLoadTable>(model, 0).id == 1 &&
             cardAt<DefaultTemperatureLoadTable>(model, 0).temperature == 208.4));
    REQUIRE((cardAt<DefaultTemperatureLoadTable>(model, 3).id == 4 &&
             cardAt<DefaultTemperatureLoadTable>(model, 3).temperature == 500.0));
    // Written as cards of their own; the set 1 is shared with a TEMP card (a set ID is a group).
    requireRoundTrip(model);

    // A temperature set of Case Control resolves to the TEMPD that has it.
    Model selected = readModelText(
        "[case]\n"
        "SubcaseStart;1\n"
        "TemperatureSelection;4;InitialTemperature\n"
        "TemperatureSelection;6;TemperatureLoadPart\n"
        "[model]\n"
        "DefaultTemperatureLoad;1;208.4;4;500.\n",
        true);
    REQUIRE(unresolvedIn(selected) == 1); // the set 6 is not there
    Renumbering::apply(selected, RenumberSpace::LoadCombinationSet, Renumbering::offset(10));
    REQUIRE(cardAt<DefaultTemperatureLoadTable>(selected, 1).id == 14);
    expectInvalid("DefaultTemperatureLoad;1", "temperature1");
    expectInvalid("DefaultTemperatureLoad;1;20.;2", "temperature2");
    expectInvalid("DefaultTemperatureLoad;0;20.", "id");
}

TEST_CASE("TEMPP1 has its temperatures and its elements over the continuation lines", "[model][cards]")
{
    Model model = readBulk(rods() +
                           "PlateTemperatureLoad;2;27;55.;8.;51.;59.;29;23;17;30\n"
                           "PlateTemperatureLoad;3;1;5.;;;;2;Through;10;13;Through;16\n"
                           "PlateTemperatureLoad;4;5;100.");
    PlateTemperatureLoadCard card = cardAt<PlateTemperatureLoadTable>(model, 0);
    REQUIRE((card.meanTemperature == 55.0 && card.temperatureGradient == 8.0 && card.temperature1 == 51.0 &&
             card.temperature2 == 59.0));
    REQUIRE(card.elements == std::vector<ElementRun>{{27, 27}, {29, 29}, {23, 23}, {17, 17}, {30, 30}});
    card = cardAt<PlateTemperatureLoadTable>(model, 1);
    REQUIRE(card.elements == std::vector<ElementRun>{{1, 1}, {2, 10}, {13, 16}});
    REQUIRE((card.temperatureGradient == 0.0 && std::isnan(card.temperature1) &&
             std::isnan(card.temperature2))); // blank T1 and T2 are none
    REQUIRE(unresolvedIn(model) == 4);        // the elements 27, 29, 23 and 30 (the rods are 1 to 20)
    requireRoundTrip(model);
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(cardAt<PlateTemperatureLoadTable>(model, 1).elements ==
            std::vector<ElementRun>{{101, 101}, {102, 110}, {113, 116}});
    // Blank T1 and T2 are not written; a set one is.

    expectInvalid("PlateTemperatureLoad;1", "elemId1");
    expectInvalid("PlateTemperatureLoad;1;5;1.;;;;Through;9", "Through");
    expectInvalid("PlateTemperatureLoad;1;5;1.;;;;9;Through;7", "elemId");
    expectInvalid("PlateTemperatureLoad;0;5;1.", "id");
}

TEST_CASE("TEMPRB gives the point temperatures their end temperature when some are blank and none when all are",
          "[model][cards]")
{
    Model model = readBulk(rods() +
                           "LineElemTemperatureLoad;200;1;72.;26.;0.;31.;;2.5;72.;87.;42.;;53.;76.;18.;;9;10\n"
                           "LineElemTemperatureLoad;201;5;100.;100.\n"
                           "LineElemTemperatureLoad;202;1;10.;18.;;;;;;;;;;;;;2;Through;4;10;Through;14");
    LineElemTemperatureLoadCard card = cardAt<LineElemTemperatureLoadTable>(model, 0);
    REQUIRE((card.temperatureA == 72.0 && card.temperatureB == 26.0));
    REQUIRE((card.gradientY == std::array<double, 2>{0.0, 31.0} && card.gradientZ == std::array<double, 2>{0.0, 2.5}));
    // Some point temperatures given: the blank ones are TA (end A) and TB (end B), stored and written.
    REQUIRE(card.pointsA == std::array<double, 4>{72.0, 87.0, 42.0, 72.0});
    REQUIRE(card.pointsB == std::array<double, 4>{53.0, 76.0, 18.0, 26.0});
    REQUIRE(card.elements == std::vector<ElementRun>{{1, 1}, {9, 9}, {10, 10}});
    REQUIRE_FALSE(cardAt<LineElemTemperatureLoadTable>(model, 1).hasPointTemperatures());
    REQUIRE(cardAt<LineElemTemperatureLoadTable>(model, 2).elements ==
            std::vector<ElementRun>{{1, 1}, {2, 4}, {10, 14}});
    requireRoundTrip(model);

    (void)0; // no point temperature line
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(cardAt<LineElemTemperatureLoadTable>(model, 2).elements ==
            std::vector<ElementRun>{{101, 101}, {102, 104}, {110, 114}});
}

TEST_CASE("TEMPAX, DEFORM and FORCEAX have their fields and rules", "[model][cards]")
{
    Model model = readBulk(rods() +
                           "AxisymConeTemperatureLoad;4;7;30.;98.6;5;8;60.;110.\n"
                           "AxialDeformationLoad;1;5;0.05;6;-0.1\n"
                           "AxialDeformationLoad;2;1;0.1;2;0.2;3;0.3\n"
                           "AxisymRingForceLoad;1;2;3;2.;0.1;0.2;0.3\n"
                           "AxisymRingForceLoad;2;2;S0T10;1.;0.1\n"
                           "AxisymRingForceLoad;10;1013;0;1.;31.416");
    REQUIRE(rowCount<AxisymConeTemperatureLoadTable>(model) == 2);
    REQUIRE((cardAt<AxisymConeTemperatureLoadTable>(model, 1).id == 5 &&
             cardAt<AxisymConeTemperatureLoadTable>(model, 1).ringId == 8 &&
             cardAt<AxisymConeTemperatureLoadTable>(model, 1).phi == 60.0 &&
             cardAt<AxisymConeTemperatureLoadTable>(model, 1).temperature == 110.0));
    const AxialDeformationLoadCard deform = cardAt<AxialDeformationLoadTable>(model, 0);
    REQUIRE(deform.deformations == std::vector<ElementDeformation>{{5, 0.05}, {6, -0.1}});
    REQUIRE(cardAt<AxialDeformationLoadTable>(model, 1).deformations.size() == 3);
    AxisymRingForceLoadCard forceax = cardAt<AxisymRingForceLoadTable>(model, 0);
    REQUIRE((forceax.ringId == 2 && forceax.harmonicId == 3 && !forceax.isSequence() && forceax.scaleFactor == 2.0 &&
             forceax.axialForce == 0.3));
    forceax = cardAt<AxisymRingForceLoadTable>(model, 1);
    REQUIRE((forceax.isSequence() && forceax.harmonicId == 0 && forceax.harmonicEnd == 10));
    REQUIRE(cardAt<AxisymRingForceLoadTable>(model, 2).harmonicId == 0); // a harmonic 0 written as such
    // The DEFORM elements are element references; the rings of the TEMPAX and the FORCEAX are RINGAX points of the node
    // ID space (the grid points of this deck stand for them), and the ring 1013 of the last FORCEAX is none of them.
    REQUIRE(unresolvedIn(model) == 1);
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(cardAt<AxialDeformationLoadTable>(model, 0).deformations[1].element == 106);
    requireRoundTrip(model);

    expectInvalid("AxisymConeTemperatureLoad;1;2;3.", "temperature1");
    expectInvalid("AxisymConeTemperatureLoad;1;2;3.;4.;5", "ringId2");
    expectInvalid("AxialDeformationLoad;1;5", "deformation");
    expectInvalid("AxialDeformationLoad;1;5;1.;6;2.;7;3.;8;4.", "elemId3");
    expectInvalid("AxialDeformationLoad;1;0;1.", "elemId1");
    expectInvalid("AxisymRingForceLoad;1;2;S5;1.", "harmonicId");
    expectInvalid("AxisymRingForceLoad;1;2;S5T3;1.", "harmonicId");
    expectInvalid("AxisymRingForceLoad;1;2;-1;1.", "harmonicId");
    expectInvalid("AxisymRingForceLoad;1;0;1;1.", "ringId");
}

// ------------------------------------------------------------------------------------------------ dynamic loads

TEST_CASE("ACSRCE has IDs or constants in three fields and names its excitation set", "[model][cards]")
{
    Model model = readBulk(
        "DynamicLoadScale;11;5;1;1.\n"
        "Node;5;;0.;0.;0.\n"
        "TableDynamic1;12;;;0.;1.;1.;2.;EndTable\n"
        "TableDynamic1;1001;;;0.;1.;1.;2.;EndTable\n"
        "DynamicLoadDelay;20;5;;0.5\n"
        "AcousticSourceLoad;103;11;20;;;7.;12;;1.;18.\n"
        "AcousticSourceLoad;104;11;;;;;1001;;1.;1.\n"
        "AcousticSourceLoad;105;11;;0.1;;30.;;2.5;1.2;1.4E+5");
    AcousticSourceLoadCard card = cardAt<AcousticSourceLoadTable>(model, 0);
    REQUIRE(
        (card.excitationId == 11 && card.delayId == 20 && card.delay == 0.0 && card.phaseId == 0 && card.phase == 7.0));
    REQUIRE((card.powerTableId == 12 && card.power == 0.0 && card.density == 1.0 && card.bulkModulus == 18.0));
    card = cardAt<AcousticSourceLoadTable>(model, 1);
    REQUIRE((card.delayId == 0 && card.phaseId == 0 && card.powerTableId == 1001));
    card = cardAt<AcousticSourceLoadTable>(model, 2);
    REQUIRE((card.delay == 0.1 && card.phase == 30.0 && card.powerTableId == 0 && card.power == 2.5));
    REQUIRE(unresolvedIn(model) == 0); // the DELAY set 20 is there
    RecordingVisitor visitor;
    model.table<AcousticSourceLoadTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"excitationId", "delayId", "powerTableId"});
    REQUIRE(visitor.targets[0] == ReferenceTarget::ExcitationSet);
    REQUIRE(visitor.targets[1] == ReferenceTarget::DelaySet);
    REQUIRE(visitor.targets[2] == ReferenceTarget::DynamicTable);
    // The set is a dynamic load set: the DLOAD command selects it, and renumbering the dynamic loads renumbers it.
    Renumbering::apply(model, RenumberSpace::DynamicLoadSet, Renumbering::offset(1000));
    REQUIRE(cardAt<AcousticSourceLoadTable>(model, 0).id == 1103);
    REQUIRE(cardAt<AcousticSourceLoadTable>(model, 0).excitationId == 1011); // it follows the DAREA it names
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(50));
    REQUIRE(cardAt<AcousticSourceLoadTable>(model, 1).powerTableId == 1051);
    requireRoundTrip(model);

    expectInvalid("AcousticSourceLoad;1;11;;;;;;;1.;1.", "powerTableId"); // neither a table nor a power
    expectInvalid("AcousticSourceLoad;1;11;;;;;5;;0.;1.", "density");
    expectInvalid("AcousticSourceLoad;1;11;;;;;5;;1.;-1.", "bulkModulus");
    expectInvalid("AcousticSourceLoad;1;11;;;;;5;;1.", "bulkModulus");
    expectInvalid("AcousticSourceLoad;1;0;;;;;5;;1.;1.", "excitationId");
}

TEST_CASE("NOLIN1 to NOLIN4 have their points, components and last field", "[model][cards]")
{
    Model model = readBulk(grids() +
                           "ExtraPoint;60\n"
                           "TableDynamic1;6;;;0.;1.;1.;2.;EndTable\n"
                           "NonlinearTableLoad1;21;3;4;2.6;3;10;6\n"
                           "NonlinearProductLoad2;14;2;1;3.3;2;1;60;3\n"
                           "NonlinearPositivePowerLoad3;4;10;2;-4.8;2;5;-2.5\n"
                           "NonlinearNegativePowerLoad4;100;1;2;-0.6;101;10;2.");
    const NonlinearTableLoad1Card nolin1 = cardAt<NonlinearTableLoad1Table>(model);
    REQUIRE((nolin1.nodeI == 3 && nolin1.componentsI == 4 && nolin1.scaleFactor == Approx(2.6) && nolin1.nodeJ == 3 &&
             nolin1.componentsJ == 10 && nolin1.tableId == 6));
    const NonlinearProductLoad2Card nolin2 = cardAt<NonlinearProductLoad2Table>(model);
    REQUIRE((nolin2.nodeK == 60 && nolin2.componentsK == 3));
    REQUIRE(cardAt<NonlinearPositivePowerLoad3Table>(model).exponent == Approx(-2.5));
    REQUIRE(cardAt<NonlinearNegativePowerLoad4Table>(model).exponent == Approx(2.0));
    // The point 101 is not defined: a scalar point may be named without an SPOINT (a velocity of component 10).
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(10));
    REQUIRE(cardAt<NonlinearTableLoad1Table>(model).tableId == 16);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<NonlinearProductLoad2Table>(model).nodeK == 1060);
    Renumbering::apply(model, RenumberSpace::DynamicLoadSet, Renumbering::offset(5));
    REQUIRE(cardAt<NonlinearPositivePowerLoad3Table>(model).id == 9);
    requireRoundTrip(model);
    Model lacking = readBulk("NonlinearTableLoad1;21;3;4;2.6;3;10;6");
    REQUIRE(unresolvedIn(lacking) == 1); // the table 6; the points may be scalar points

    expectInvalid("NonlinearTableLoad1;1;3;7;1.;3;;6", "componentsI");
    expectInvalid("NonlinearTableLoad1;1;3;;1.;3;7;6", "componentsJ");
    expectInvalid("NonlinearTableLoad1;1;3;;1.;3;17;6", "componentsJ");
    expectInvalid("NonlinearTableLoad1;1;3;;1.;3", "tableId");
    expectInvalid("NonlinearTableLoad1;1;3;;;3;;6", "scaleFactor");
    expectInvalid("NonlinearProductLoad2;1;3;;1.;3;;4;7", "componentsK");
    expectInvalid("NonlinearProductLoad2;1;3;;1.;3", "nodeK");
    expectInvalid("NonlinearPositivePowerLoad3;1;3;;1.;3", "exponent");
    expectInvalid("NonlinearNegativePowerLoad4;1;3;;1.;0;;2.", "nodeJ");
}

// ------------------------------------------------------------------------------------------------ PLOAD4 and GRAV

TEST_CASE("every form of PLOAD4 is read with the fields the format reference gives it", "[model][cards]")
{
    Model model = readBulk(rods() +
                           "FacePressureLoad;1;1;10.\n"
                           "FacePressureLoad;2;2;10.;8.;5.\n"
                           "FacePressureLoad;3;3;10.;8.;5.;3.;1\n"
                           "FacePressureLoad;4;4;10.;;;;12;14;6;0.;1.;0.\n"
                           "FacePressureLoad;5;5;1.;;;;15\n"
                           "FacePressureLoad;6;6;10.;8.;5.;;Through;9;6;0.;1.;0.\n"
                           "FacePressureLoad;7;10;1.;;2.;;;;;;;;Line;Tangent\n"
                           "FacePressureLoad;8;11;1.;;2.;;;;6;0.;1.;0.;Line\n"
                           "FacePressureLoad;9;12;1.;;;;;;;;;;Surface;Norm\n"
                           "RectFrame2;6;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"); // the defaults written out
    REQUIRE(cardAt<FacePressureLoadTable>(model, 0).pressures == std::array<double, 4>{10.0, 10.0, 10.0, 10.0});
    REQUIRE(cardAt<FacePressureLoadTable>(model, 1).pressures == std::array<double, 4>{10.0, 8.0, 5.0, 10.0});
    REQUIRE(cardAt<FacePressureLoadTable>(model, 2).pressures == std::array<double, 4>{10.0, 8.0, 5.0, 3.0});
    FacePressureLoadCard solid = cardAt<FacePressureLoadTable>(model, 3);
    REQUIRE((solid.node1 == 12 && solid.node3Or4 == 14 && solid.frameId == 6 &&
             solid.direction == std::array<double, 3>{0.0, 1.0, 0.0}));
    REQUIRE(cardAt<FacePressureLoadTable>(model, 4).node1 == 15);
    const FacePressureLoadCard range = cardAt<FacePressureLoadTable>(model, 5);
    REQUIRE((range.isRange() && range.elemId2 == 9 && range.frameId == 6));
    const FacePressureLoadCard tangent = cardAt<FacePressureLoadTable>(model, 6);
    REQUIRE((tangent.kind == PressureKind::Line && tangent.lineDirection == LineLoadDirection::Tangent));
    REQUIRE((std::isnan(tangent.pressures[1]) && std::isnan(tangent.pressures[3]) && tangent.pressures[2] == 2.0));
    const FacePressureLoadCard vector = cardAt<FacePressureLoadTable>(model, 7);
    REQUIRE((vector.kind == PressureKind::Line && vector.lineDirection == LineLoadDirection::Normal &&
             vector.frameId == 6));
    REQUIRE(cardAt<FacePressureLoadTable>(model, 8).kind == PressureKind::Surface);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    // The same card read alone.
    const Model tagged = readModelText("[model]\nFacePressureLoad;3;2216;10.;8.;5.;;37;6;6;0.;1.;0.\n", true);
    REQUIRE(cardAt<FacePressureLoadTable>(tagged).frameId == 6);

    expectInvalid("FacePressureLoad;1;1;1.;;;;;;;;;;;XX", "lineLoadDirection");
    expectInvalid("FacePressureLoad;1;1;1.;;;;;;;;;;BAD", "surfaceOrLine");
    expectInvalid("FacePressureLoad;1;1;1.;;;;;;;0.;1.;0.;Line;XDirection",
                  "lineLoadDirection"); // a vector and LDIR together
    expectInvalid("FacePressureLoad;1;5;1.;;;;Through;3", "elemId2");
}

TEST_CASE("GRAV is read with its coordinate system, vector and MB, and its vector may not be zero", "[model][cards]")
{
    Model model = readBulk(
        "RectFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "GravityLoad;1;;12.5;0.;0.;-1.\n"
        "GravityLoad;2;3;9.81;1.;2.;3.;-1\n"
        "GravityLoad;3;0;1.;0.;0.;1.;0\n");
    GravityLoadCard card = cardAt<GravityLoadTable>(model, 0);
    REQUIRE((card.frameId == 0 && card.magnitude == Approx(12.5) &&
             card.direction == std::array<double, 3>{0.0, 0.0, -1.0} && card.mainBulkFlag == 0));
    card = cardAt<GravityLoadTable>(model, 1);
    REQUIRE((card.frameId == 3 && card.mainBulkFlag == -1 && card.direction == std::array<double, 3>{1.0, 2.0, 3.0}));
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<GravityLoadTable>(model, 1).frameId == 13);
    requireRoundTrip(model);
    // MB is written only when it is not the default.

    expectInvalid("GravityLoad;1;;1.;0.;0.;0.", "directionX");
    expectInvalid("GravityLoad;1;-1;1.;0.;0.;1.", "frameId");
    expectInvalid("GravityLoad;1;;;0.;0.;1.", "magnitude");
}
