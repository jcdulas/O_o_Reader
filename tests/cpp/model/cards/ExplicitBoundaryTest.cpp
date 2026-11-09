// Checks of the boundary and user data cards of the explicit solver: the flow boundaries of an
// Euler mesh (BARRIER, FLOW, FLOWC, FLOWDEF, FLOWT, FLOWUDS), the rigid wall and the detonation point (WALL, DETSPH),
// the hourglass suppression (HGSUPPR), the breakable join (BJOIN), the user subroutines (FORCUDS, GENUDS, UDSESV) and
// the fluid-structure switch (DYFSISW). The generic checks (read, write in every format, copy) run on their samples in
// CardRoundTripTest; this file has what is particular to each card: its fields in their places, the defaults, the
// references and the sets, and the refused values.

#include "oor/model/cards/explicit/eulerBarrier/EulerBarrierTable.hpp"
#include "oor/model/cards/explicit/eulerCyclicFlowBoundary/EulerCyclicFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/eulerDefaultFlowBoundary/EulerDefaultFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/eulerFlowBoundary/EulerFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/eulerTimeFlowBoundary/EulerTimeFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/eulerUserFlowBoundary/EulerUserFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/fluidStructureSwitch/FluidStructureSwitchTable.hpp"
#include "oor/model/cards/explicit/hourglassSuppression/HourglassSuppressionTable.hpp"
#include "oor/model/cards/explicit/rigidWall/RigidWallTable.hpp"
#include "oor/model/cards/explicit/sphericalDetonation/SphericalDetonationTable.hpp"
#include "oor/model/cards/explicit/spotweldJoinPairs/SpotweldJoinPairsTable.hpp"
#include "oor/model/cards/explicit/userEnforcedVelocity/UserEnforcedVelocityTable.hpp"
#include "oor/model/cards/explicit/userServiceData/UserServiceDataTable.hpp"
#include "oor/model/cards/explicit/userStateVariables/UserStateVariablesTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The four grid points that the BCSEG cards of these tests are made of, so that a set of BCSEG cards leaves no dangling
// reference.
constexpr const char* FACE_NODES = "Node;10;;0.;0.;0.\nNode;11;;1.;0.;0.\nNode;12;;1.;1.;0.\nNode;13;;0.;1.;0.\n";

} // namespace

TEST_CASE("a BARRIER has its fields in the order of the format reference and a box that is resolved",
          "[model][cards][explicit]")
{
    Model model = readBulk(std::string(FACE_NODES) +
                           "EulerBarrier;100;20\n"
                           "EulerBarrier;101;21;5;PositiveXDirection;0.1;0.;1.;-2.;2.;3.;4.\n"
                           "EulerBarrier;102;;;;;5.\n"
                           "EulerBarrier;103;;;NegativeZDirection\n"
                           "EulerBarrier;104;;;;;;;;;;7.\n"
                           "ContactSegment;1;20;10;11;12;13\n"
                           "ContactSegment;2;21;10;11;12;13");
    const EulerBarrierCard bare = cardAt<EulerBarrierTable>(model, 0);
    REQUIRE((bare.id == 100 && bare.contactBodyId == 20 && bare.meshId == 0 && !bare.meshFace && !bare.skinFriction));
    REQUIRE(bare.box == BoundaryBox{}); // no box given: the default box, which is not written
    const EulerBarrierCard full = cardAt<EulerBarrierTable>(model, 1);
    REQUIRE((full.contactBodyId == 21 && full.meshId == 5 && full.meshFace == BoundaryFace::PositiveX &&
             full.skinFriction == 0.1));
    REQUIRE((full.box.minX == 0.0 && full.box.maxX == 1.0 && full.box.minY == -2.0 && full.box.maxY == 2.0));
    REQUIRE((full.box.minZ == 3.0 && full.box.maxZ == 4.0));
    // A MAX that is blank while its MIN is given is the MIN: the box is a plane. The other axes keep their defaults.
    const BoundaryBox plane = cardAt<EulerBarrierTable>(model, 2).box;
    REQUIRE((plane.minX == 5.0 && plane.maxX == 5.0));
    REQUIRE((plane.minY == BoundaryBox::LOWEST && plane.maxY == BoundaryBox::HIGHEST));
    REQUIRE((plane.minZ == BoundaryBox::LOWEST && plane.maxZ == BoundaryBox::HIGHEST));
    REQUIRE(cardAt<EulerBarrierTable>(model, 3).meshFace == BoundaryFace::NegativeZ);
    const BoundaryBox half = cardAt<EulerBarrierTable>(model, 4).box; // a MAX alone leaves its MIN at the lowest value
    REQUIRE((half.minZ == BoundaryBox::LOWEST && half.maxZ == 7.0 && half.minX == BoundaryBox::LOWEST));
    requireRoundTrip(model);

    // A card with the default box has no box fields, and a box that is given is written with its six values.

    // BID is a number of its own; BCID names the set of BCSEG cards and MESH an Euler mesh.
    REQUIRE(rowCount<EulerBarrierTable>(model) == 5);
    REQUIRE(unresolvedIn(model) == 1); // the MESH 5
    RecordingVisitor visitor;
    model.table<EulerBarrierTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"contactBodyId", "meshId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::EulerMesh});
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(1000));
    REQUIRE((cardAt<EulerBarrierTable>(model, 0).contactBodyId == 1020 &&
             cardAt<EulerBarrierTable>(model, 1).contactBodyId == 1021));

    REQUIRE_THROWS_AS(readBulk("EulerBarrier;100\nEulerBarrier;100"), base::DeckError); // the BID is unique

    expectInvalid("EulerBarrier;0;20", "id");
    expectInvalid("EulerBarrier;1;-1", "contactBodyId");
    expectInvalid("EulerBarrier;1;;-2", "meshId");
    expectInvalid("EulerBarrier;1;;;UP", "meshFace");
    expectInvalid("EulerBarrier;1;;;;-0.5", "skinFriction");
    expectInvalid("EulerBarrier;1;;;;;2.;1.", "maxX"); // the MAX is below the MIN
    expectInvalid("EulerBarrier;1;;;;;x", "minX");
}

TEST_CASE(
    "a FLOW has its fields in the order of the format reference, up to five properties, and its LID is a set a TLOAD1 "
    "excites",
    "[model][cards][explicit]")
{
    Model model =
        readBulk(std::string(FACE_NODES) +
                 "EulerFlowBoundary;130;132;;;;;;;;;VelocityX;80.\n"
                 "EulerFlowBoundary;130;;5;PositiveXDirection;0.;1.;-2.;2.;3.;4.;Material;25;VelocityX;1.5;VelocityY;-"
                 "2.5;VelocityZ;0.;Pressure;1.E+5\n"
                 "EulerFlowBoundary;121;;;;;;;;;;Density;1.E+3;SpecificInternalEnergy;2.5E+5;Flow;In;Hydrostatic;3\n"
                 "EulerFlowBoundary;132;;;;;;;;;;Flow\n"
                 "EulerFlowBoundary;123;;;;;;;;;;Flow;Symmetry\n"
                 "EulerFlowBoundary;124\n"
                 "ContactSegment;1;132;10;11;12;13\n"
                 "EulerMat;25;1.E+3;1\n"
                 "GammaLawStateEquation;1\n"
                 "TransientTableLoad1;1;130;;;;;5.");
    const EulerFlowBoundaryCard simple = cardAt<EulerFlowBoundaryTable>(model, 0);
    REQUIRE((simple.setId == 130 && simple.contactBodyId == 132 && simple.meshId == 0 && !simple.meshFace));
    REQUIRE(simple.box == BoundaryBox{});
    REQUIRE(simple.properties.size() == 1);
    REQUIRE((simple.properties[0].type == FlowPropertyType::XVelocity && simple.properties[0].number == 80.0));

    const EulerFlowBoundaryCard full = cardAt<EulerFlowBoundaryTable>(model, 1);
    REQUIRE((full.contactBodyId == 0 && full.meshId == 5 && full.meshFace == BoundaryFace::PositiveX));
    REQUIRE((full.box.minX == 0.0 && full.box.maxX == 1.0 && full.box.minY == -2.0 && full.box.maxZ == 4.0));
    REQUIRE(full.properties.size() == 5);
    REQUIRE((full.properties[0].type == FlowPropertyType::Material && full.properties[0].id == 25));
    REQUIRE((full.properties[1].type == FlowPropertyType::XVelocity && full.properties[1].number == 1.5));
    REQUIRE((full.properties[2].type == FlowPropertyType::YVelocity && full.properties[2].number == -2.5));
    REQUIRE((full.properties[3].type == FlowPropertyType::ZVelocity && full.properties[3].number == 0.0));
    REQUIRE((full.properties[4].type == FlowPropertyType::Pressure && full.properties[4].number == 1.0E5));

    const EulerFlowBoundaryCard others = cardAt<EulerFlowBoundaryTable>(model, 2);
    REQUIRE(others.properties.size() == 4);
    REQUIRE((others.properties[0].type == FlowPropertyType::Density && others.properties[0].number == 1000.0));
    REQUIRE((others.properties[1].type == FlowPropertyType::SpecificEnergy && others.properties[1].number == 2.5E5));
    REQUIRE((others.properties[2].type == FlowPropertyType::Flow && others.properties[2].mode == FlowMode::In));
    REQUIRE((others.properties[3].type == FlowPropertyType::Hydrostatic && others.properties[3].id == 3));
    REQUIRE(cardAt<EulerFlowBoundaryTable>(model, 3).properties[0].mode == FlowMode::Both); // a blank value is BOTH
    REQUIRE(cardAt<EulerFlowBoundaryTable>(model, 4).properties[0].mode == FlowMode::Symmetry);
    REQUIRE(cardAt<EulerFlowBoundaryTable>(model, 5).properties.empty());
    requireRoundTrip(model);

    // Two cards of one set share the LID, which is the ID the table files them under.
    REQUIRE(rowCount<EulerFlowBoundaryTable>(model) == 6);
    REQUIRE(model.table<EulerFlowBoundaryTable>().idAtRow(0) == model.table<EulerFlowBoundaryTable>().idAtRow(1));

    // The TLOAD1 finds the set 130 (the table defines excitation IDs); the MESH 5 and the HYDSTAT 3 are cards of the
    // deck that no card defines.
    REQUIRE(unresolvedIn(model) == 2);
    Model lacking = readBulk("TransientTableLoad1;1;130;;;;;5.");
    REQUIRE(unresolvedIn(lacking) == 1);

    RecordingVisitor visitor;
    model.table<EulerFlowBoundaryTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"meshId", "propertyValue1"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::EulerMesh, ReferenceTarget::Material});
    visitor = RecordingVisitor{};
    model.table<EulerFlowBoundaryTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propertyValue4"}); // the HYDSTAT is the fourth pair
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::HydrostaticPreset});
    visitor = RecordingVisitor{};
    model.table<EulerFlowBoundaryTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"contactBodyId"}); // a velocity is a number: nothing to visit
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface});

    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE(cardAt<EulerFlowBoundaryTable>(model, 1).properties[0].id == 1025);
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(1000));
    REQUIRE(cardAt<EulerFlowBoundaryTable>(model, 0).contactBodyId == 1132);

    expectInvalid("EulerFlowBoundary;0", "setId");
    expectInvalid("EulerFlowBoundary;1;-1", "contactBodyId");
    expectInvalid("EulerFlowBoundary;1;;-1", "meshId");
    expectInvalid("EulerFlowBoundary;1;;;UP", "meshFace");
    expectInvalid("EulerFlowBoundary;1;;;;2.;1.", "maxX");
    expectInvalid("EulerFlowBoundary;1;;;;;;;;;;TEMP;1.", "propertyKind1");
    expectInvalid("EulerFlowBoundary;1;;;;;;;;;;VelocityX", "propertyValue1");
    expectInvalid("EulerFlowBoundary;1;;;;;;;;;;VelocityX;fast", "propertyValue1");
    expectInvalid("EulerFlowBoundary;1;;;;;;;;;;;5.", "propertyKind1"); // a value without its type
    expectInvalid("EulerFlowBoundary;1;;;;;;;;;;Material;0", "propertyValue1");
    expectInvalid("EulerFlowBoundary;1;;;;;;;;;;Hydrostatic;-2", "propertyValue1");
    expectInvalid("EulerFlowBoundary;1;;;;;;;;;;Flow;UP", "propertyValue1");
}

TEST_CASE("a FLOWDEF has its properties in pairs from field 9, as many as it needs", "[model][cards][explicit]")
{
    Model model = readBulk(
        "EulerDefaultFlowBoundary;25;Hydrodynamic;Density;1.E+3\n"
        "EulerDefaultFlowBoundary;26;MultiMaterialStrength;Material;25;VelocityX;1.;Flow;Out;Pressure;5.E+4;"
        "Hydrostatic;2\n"
        "EulerDefaultFlowBoundary;27\n"
        "EulerMat;25;1.E+3;1\n"
        "GammaLawStateEquation;1");
    const EulerDefaultFlowBoundaryCard density = cardAt<EulerDefaultFlowBoundaryTable>(model, 0);
    REQUIRE((density.id == 25 && density.solverType == EulerSolverType::Hydro && density.properties.size() == 1));
    REQUIRE((density.properties[0].type == FlowPropertyType::Density && density.properties[0].number == 1000.0));
    const EulerDefaultFlowBoundaryCard many = cardAt<EulerDefaultFlowBoundaryTable>(model, 1);
    REQUIRE(many.solverType == EulerSolverType::MultiMaterialStrength);
    REQUIRE(many.properties.size() == 5);
    REQUIRE((many.properties[2].type == FlowPropertyType::Flow && many.properties[2].mode == FlowMode::Out));
    REQUIRE((many.properties[4].type == FlowPropertyType::Hydrostatic && many.properties[4].id == 2));
    const EulerDefaultFlowBoundaryCard bare = cardAt<EulerDefaultFlowBoundaryTable>(model, 2);
    REQUIRE((bare.solverType == EulerSolverType::Hydro && bare.properties.empty())); // the default type
    requireRoundTrip(model);

    REQUIRE(unresolvedIn(model) == 1); // the HYDSTAT 2
    RecordingVisitor visitor;
    model.table<EulerDefaultFlowBoundaryTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propertyValue1", "propertyValue5"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::HydrostaticPreset});
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE(cardAt<EulerDefaultFlowBoundaryTable>(model, 1).properties[0].id == 125);

    expectInvalid("EulerDefaultFlowBoundary;0", "id");
    expectInvalid("EulerDefaultFlowBoundary;1;FirstOrder",
                  "solverType"); // the kinds of a PEULER that a FLOWDEF does not list
    expectInvalid("EulerDefaultFlowBoundary;1;SecondOrder", "solverType");
    expectInvalid("EulerDefaultFlowBoundary;1;FAST", "solverType");
    expectInvalid("EulerDefaultFlowBoundary;1;;Flow;Symmetry", "propertyValue1"); // SYM is a value of FLOW only
    expectInvalid("EulerDefaultFlowBoundary;1;;VelocityX", "propertyValue1");
    expectInvalid("EulerDefaultFlowBoundary;1;;;1.", "propertyKind1");
    expectInvalid("EulerDefaultFlowBoundary;1;;VelocityX;1.;TEMP;1.", "propertyKind2");
}

TEST_CASE("a FLOWC is one of a pair, with a TYPE, a FID2 and a method, and its FID is an excitation set",
          "[model][cards][explicit]")
{
    Model model = readBulk(std::string(FACE_NODES) +
                           "EulerCyclicFlowBoundary;10;30;In;;;20\n"
                           "EulerCyclicFlowBoundary;20;40;Out;;;10\n"
                           "EulerCyclicFlowBoundary;11;31;Both;5;NegativeYDirection;21;0.;1.;-1.;1.;0.;2.;2;7\n"
                           "ContactSegment;1;30;10;11;12;13\n"
                           "ContactSegment;2;40;10;11;12;13\n"
                           "ContactSegment;3;31;10;11;12;13\n"
                           "TableDynamic1;7;;;0.;1.;1.;1.;EndTable\n"
                           "TransientTableLoad1;1;10;;;;;5.");
    const EulerCyclicFlowBoundaryCard first = cardAt<EulerCyclicFlowBoundaryTable>(model, 0);
    REQUIRE((first.id == 10 && first.contactBodyId == 30 && first.type == FlowBoundaryType::In &&
             first.pairedBoundaryId == 20));
    REQUIRE((first.meshId == 0 && !first.meshFace && first.box == BoundaryBox{}));
    REQUIRE((first.method == 1 && first.tableId == 0)); // the defaults
    REQUIRE((cardAt<EulerCyclicFlowBoundaryTable>(model, 1).type == FlowBoundaryType::Out &&
             cardAt<EulerCyclicFlowBoundaryTable>(model, 1).pairedBoundaryId == 10));
    const EulerCyclicFlowBoundaryCard full = cardAt<EulerCyclicFlowBoundaryTable>(model, 2);
    REQUIRE((full.type == FlowBoundaryType::Both && full.meshId == 5 && full.meshFace == BoundaryFace::NegativeY));
    REQUIRE((full.pairedBoundaryId == 21 && full.method == 2 && full.tableId == 7));
    REQUIRE((full.box.minX == 0.0 && full.box.maxX == 1.0 && full.box.minY == -1.0 && full.box.maxZ == 2.0));
    requireRoundTrip(model);

    // FID2 names a FLOWC, which no category stands for: it is not visited and not looked up (the FLOWC 21 is not
    // there).
    REQUIRE(unresolvedIn(model) == 1); // the MESH 5
    RecordingVisitor visitor;
    model.table<EulerCyclicFlowBoundaryTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"contactBodyId", "meshId", "tableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::EulerMesh,
                                                            ReferenceTarget::DynamicTable});
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE(cardAt<EulerCyclicFlowBoundaryTable>(model, 2).tableId == 107);

    expectInvalid("EulerCyclicFlowBoundary;0;30;In;;;20", "id");
    expectInvalid("EulerCyclicFlowBoundary;1;-1;In;;;20", "contactBodyId");
    expectInvalid("EulerCyclicFlowBoundary;1;30;;;;20", "flowType");
    expectInvalid("EulerCyclicFlowBoundary;1;30;UP;;;20", "flowType");
    expectInvalid("EulerCyclicFlowBoundary;1;30;In;-1;;20", "meshId");
    expectInvalid("EulerCyclicFlowBoundary;1;30;In;;UP;20", "meshFace");
    expectInvalid("EulerCyclicFlowBoundary;1;30;In", "pairedBoundaryId");
    expectInvalid("EulerCyclicFlowBoundary;1;30;In;;;0", "pairedBoundaryId");
    expectInvalid("EulerCyclicFlowBoundary;1;30;In;;;20;2.;1.", "maxX");
    expectInvalid("EulerCyclicFlowBoundary;1;30;In;;;20;;;;;;;4", "method");
    expectInvalid("EulerCyclicFlowBoundary;1;30;In;;;20;;;;;;;0", "method");
    expectInvalid("EulerCyclicFlowBoundary;1;30;In;;;20;;;;;;;1;-3", "tableId");
}

TEST_CASE("a FLOWT has a velocity, a pressure, a material, a density and an energy that are numbers or tables",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        std::string(FACE_NODES) +
        "EulerTimeFlowBoundary;2;122;In;;;;;;;;;Table;101;Table;102;91;Table;104;Table;107\n"
        "EulerTimeFlowBoundary;3;;In\n"
        "EulerTimeFlowBoundary;4;;Out;;;;;;;;;Constant;-5.;Constant;1.E+5\n"
        "EulerTimeFlowBoundary;5;;Both;;;;;;;;;;;Constant;1.E+5\n"
        "EulerTimeFlowBoundary;6;126;In;6;NegativeXDirection;0.;1.;2.;3.;4.;5.;Constant;1.5;Table;102;92;Constant;"
        "1.E+3;Constant;2.5E+5\n"
        "ContactSegment;1;122;10;11;12;13\n"
        "ContactSegment;2;126;10;11;12;13\n"
        "EulerMat;91;1.E+3;1\n"
        "GammaLawStateEquation;1\n"
        "TableDynamic1;101;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;102;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;104;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;107;;;0.;1.;1.;1.;EndTable\n"
        "TransientTableLoad1;1;2;;;;;5.");
    const EulerTimeFlowBoundaryCard tables = cardAt<EulerTimeFlowBoundaryTable>(model, 0);
    REQUIRE(
        (tables.id == 2 && tables.contactBodyId == 122 && tables.type == FlowBoundaryType::In && tables.matId == 91));
    REQUIRE((tables.velocity.source == ValueSource::Table && tables.velocity.table == 101));
    REQUIRE((tables.pressure.source == ValueSource::Table && tables.pressure.table == 102));
    REQUIRE((tables.density.source == ValueSource::Table && tables.density.table == 104));
    REQUIRE((tables.specificEnergy.source == ValueSource::Table && tables.specificEnergy.table == 107));
    const EulerTimeFlowBoundaryCard bare = cardAt<EulerTimeFlowBoundaryTable>(model, 1);
    REQUIRE((bare.velocity.source == ValueSource::Element && bare.pressure.source == ValueSource::Element)); // blank
    REQUIRE((bare.density.source == ValueSource::Element && bare.specificEnergy.source == ValueSource::Element &&
             bare.matId == 0));
    const EulerTimeFlowBoundaryCard out = cardAt<EulerTimeFlowBoundaryTable>(model, 2);
    REQUIRE((out.type == FlowBoundaryType::Out && out.velocity.source == ValueSource::Constant));
    REQUIRE((out.velocity.constant == -5.0 && out.pressure.constant == 1.0E5)); // a velocity may be negative
    const EulerTimeFlowBoundaryCard both = cardAt<EulerTimeFlowBoundaryTable>(model, 3);
    REQUIRE((both.type == FlowBoundaryType::Both && both.velocity.source == ValueSource::Element));
    REQUIRE((both.pressure.source == ValueSource::Constant && both.pressure.constant == 1.0E5));
    const EulerTimeFlowBoundaryCard full = cardAt<EulerTimeFlowBoundaryTable>(model, 4);
    REQUIRE((full.meshId == 6 && full.meshFace == BoundaryFace::NegativeX && full.matId == 92));
    REQUIRE((full.box.minX == 0.0 && full.box.maxX == 1.0 && full.box.minY == 2.0 && full.box.maxZ == 5.0));
    REQUIRE(
        (full.velocity.constant == 1.5 && full.pressure.source == ValueSource::Table && full.pressure.table == 102));
    REQUIRE((full.density.constant == 1000.0 && full.specificEnergy.constant == 2.5E5));
    requireRoundTrip(model);

    REQUIRE(unresolvedIn(model) == 2); // the MESH 6 and the MATDEUL 92 (the FLOWT 2 is the excitation of the TLOAD1)
    RecordingVisitor visitor;
    model.table<EulerTimeFlowBoundaryTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields ==
            std::vector<std::string>{"contactBodyId", "velocity", "pressure", "matId", "density", "specificEnergy"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::DynamicTable,
                                         ReferenceTarget::DynamicTable, ReferenceTarget::Material,
                                         ReferenceTarget::DynamicTable, ReferenceTarget::DynamicTable});
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(1000));
    REQUIRE((cardAt<EulerTimeFlowBoundaryTable>(model, 0).velocity.table == 1101 &&
             cardAt<EulerTimeFlowBoundaryTable>(model, 0).specificEnergy.table == 1107));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE(cardAt<EulerTimeFlowBoundaryTable>(model, 0).matId == 1091);
}

TEST_CASE("a FLOWT refuses a quantity that is not a number or a table and the combinations its format forbids",
          "[model][cards][explicit]")
{
    expectInvalid("EulerTimeFlowBoundary;0;;In", "id");
    expectInvalid("EulerTimeFlowBoundary;1", "flowType");
    expectInvalid("EulerTimeFlowBoundary;1;;SIDEWAYS", "flowType");
    expectInvalid("EulerTimeFlowBoundary;1;-1;In", "contactBodyId");
    expectInvalid("EulerTimeFlowBoundary;1;;In;-1", "meshId");
    expectInvalid("EulerTimeFlowBoundary;1;;In;;UP", "meshFace");
    expectInvalid("EulerTimeFlowBoundary;1;;In;;;3.;2.", "maxX");
    expectInvalid("EulerTimeFlowBoundary;1;;In;;;;;;;;;POLY;1.", "velocitySource");
    expectInvalid("EulerTimeFlowBoundary;1;;In;;;;;;;;;Element;5.", "velocity"); // the element's value is not given
    expectInvalid("EulerTimeFlowBoundary;1;;In;;;;;;;;;Table", "velocity");
    expectInvalid("EulerTimeFlowBoundary;1;;In;;;;;;;;;Table;0", "velocity");
    expectInvalid("EulerTimeFlowBoundary;1;;In;;;;;;;;;;;POLY;1.", "pressureSource");
    expectInvalid("EulerTimeFlowBoundary;1;;In;;;;;;;;;;;;;-5", "matId");
    expectInvalid("EulerTimeFlowBoundary;1;;In;;;;;;;;;;;;;5;Constant;-1.;Constant;1.",
                  "density"); // a density is not negative
    // A material that flows in needs its density and its energy.
    expectInvalid("EulerTimeFlowBoundary;1;;In;;;;;;;;;;;;;5", "densitySource");
    expectInvalid("EulerTimeFlowBoundary;1;;In;;;;;;;;;;;;;5;Constant;1.", "energySource");
    // The material is not allowed on an outflow, and a boundary that is both ways gives the pressure only.
    expectInvalid("EulerTimeFlowBoundary;1;;Out;;;;;;;;;;;;;5;Constant;1.;Constant;1.", "matId");
    expectInvalid("EulerTimeFlowBoundary;1;;Both;;;;;;;;;Constant;1.", "velocitySource");
    expectInvalid("EulerTimeFlowBoundary;1;;Both;;;;;;;;;;;;;5;Constant;1.;Constant;1.", "matId");
    expectInvalid("EulerTimeFlowBoundary;1;;Both;;;;;;;;;;;;;;Constant;1.", "densitySource");
    expectInvalid("EulerTimeFlowBoundary;1;;Both;;;;;;;;;;;;;;;;Constant;1.", "energySource");
}

TEST_CASE("a FLOWUDS names a group and a user subroutine, which default to EXFLOW, and its LID is a set",
          "[model][cards][explicit]")
{
    Model model = readBulk(std::string(FACE_NODES) +
                           "EulerUserFlowBoundary;14;JETIN;ExternalFlow;310\n"
                           "EulerUserFlowBoundary;13;udsgrp\n"
                           "EulerUserFlowBoundary;13;MYFLOW2;mysub;301;5;PositiveXDirection;0.;1.;2.;3.;4.;5.\n"
                           "EulerUserFlowBoundary;15;UDSGRP;;;;;5.\n"
                           "ContactSegment;1;310;10;11;12;13\n"
                           "ContactSegment;2;301;10;11;12;13\n"
                           "TransientTableLoad1;1;14;;;;;5.");
    const EulerUserFlowBoundaryCard example = cardAt<EulerUserFlowBoundaryTable>(model, 0);
    REQUIRE((example.setId == 14 && example.groupName == "JETIN" && example.userRoutineName == "ExternalFlow" &&
             example.contactBodyId == 310));
    const EulerUserFlowBoundaryCard bare = cardAt<EulerUserFlowBoundaryTable>(model, 1);
    REQUIRE((bare.groupName == "udsgrp" &&
             bare.userRoutineName == "ExternalFlow")); // the group keeps its spelling; routineName has a default
    REQUIRE((bare.contactBodyId == 0 && bare.meshId == 0 && !bare.meshFace && bare.box == BoundaryBox{}));
    const EulerUserFlowBoundaryCard full = cardAt<EulerUserFlowBoundaryTable>(model, 2);
    REQUIRE((full.groupName == "MYFLOW2" && full.userRoutineName == "mysub" && full.contactBodyId == 301 &&
             full.meshId == 5));
    REQUIRE((full.meshFace == BoundaryFace::PositiveX && full.box.minX == 0.0 && full.box.maxZ == 5.0));
    const BoundaryBox plane = cardAt<EulerUserFlowBoundaryTable>(model, 3).box;
    REQUIRE((plane.minX == 5.0 && plane.maxX == 5.0 && plane.maxY == BoundaryBox::HIGHEST));
    requireRoundTrip(model);

    // Two cards share the LID 13, the ID of a set; the TLOAD1 finds the set 14.
    REQUIRE(model.table<EulerUserFlowBoundaryTable>().idAtRow(1) ==
            model.table<EulerUserFlowBoundaryTable>().idAtRow(2));
    REQUIRE(unresolvedIn(model) == 1); // the MESH 5
    RecordingVisitor visitor;
    model.table<EulerUserFlowBoundaryTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"contactBodyId", "meshId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::EulerMesh});

    (void)0; // the default UNAME is not written
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(1000));
    REQUIRE((cardAt<EulerUserFlowBoundaryTable>(model, 0).contactBodyId == 1310 &&
             cardAt<EulerUserFlowBoundaryTable>(model, 2).contactBodyId == 1301));

    expectInvalid("EulerUserFlowBoundary;0;GRP", "setId");
    expectInvalid("EulerUserFlowBoundary;1", "groupName"); // a group has no default
    REQUIRE_NOTHROW(readBulk("EulerUserFlowBoundary;1;TOOLONGNAME"));
    REQUIRE_NOTHROW(readBulk("EulerUserFlowBoundary;1;GRP;TOOLONGNAME"));
    expectInvalid("EulerUserFlowBoundary;1;GRP;;-1", "contactBodyId");
    expectInvalid("EulerUserFlowBoundary;1;GRP;;;-1", "meshId");
    expectInvalid("EulerUserFlowBoundary;1;GRP;;;;UP", "meshFace");
    expectInvalid("EulerUserFlowBoundary;1;GRP;;;;;2.;1.", "maxX");
}

TEST_CASE("a WALL has its plane, its BCGRID and its friction, and the BCGRID is a contact body",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "RigidWall;23;;;;;;1.;144;PenaltyMethod;0.25\n"
        "RigidWall;18;;;;;;;144\n"
        "RigidWall;19;1.;2.;3.;0.6;0.8;0.;144;Kinematic;0.3;0.25;0.1\n"
        "ContactNodeRegion;144;5;6;7;Through;12");
    const RigidWallCard example = cardAt<RigidWallTable>(model, 0);
    REQUIRE((example.id == 23 && example.normalZ == 1.0 && example.nodeSetId == 144 &&
             example.method == RigidWallMethod::Penalty));
    REQUIRE((example.pointX == 0.0 && example.normalX == 0.0 && example.normalY == 0.0)); // the defaults
    REQUIRE((example.staticFriction == 0.25 && example.kineticFriction == 0.0 && example.frictionDecay == 0.0));
    const RigidWallCard bare = cardAt<RigidWallTable>(model, 1);
    REQUIRE((bare.id == 18 && bare.normalZ == 0.0 && bare.staticFriction == 0.0 &&
             bare.method == RigidWallMethod::Penalty));
    const RigidWallCard full = cardAt<RigidWallTable>(model, 2);
    REQUIRE((full.pointX == 1.0 && full.pointY == 2.0 && full.pointZ == 3.0 && full.normalX == 0.6 &&
             full.normalY == 0.8 && full.normalZ == 0.0));
    REQUIRE((full.method == RigidWallMethod::Kinematic && full.staticFriction == 0.3 && full.kineticFriction == 0.25 &&
             full.frictionDecay == 0.1));
    requireRoundTrip(model);

    // The BCGRID 144 exists, so the walls add no dangling reference to those that the BCGRID itself has (its grid
    // points).
    Model onlyGrid = readBulk("ContactNodeRegion;144;5;6;7;Through;12");
    REQUIRE(unresolvedIn(model) == unresolvedIn(onlyGrid));
    RecordingVisitor visitor;
    model.table<RigidWallTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodeSetId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactBody});
    Renumbering::apply(model, RenumberSpace::ContactBody, Renumbering::offset(100));
    REQUIRE((cardAt<RigidWallTable>(model, 0).nodeSetId == 244 && cardAt<RigidWallTable>(model, 2).nodeSetId == 244));
    Model dangling = readBulk("RigidWall;1;;;;;;;9");
    REQUIRE(unresolvedIn(dangling) == 1);

    REQUIRE_THROWS_AS(readBulk("RigidWall;1;;;;;;;9\nRigidWall;1;;;;;;;9"), base::DeckError); // the ID is unique

    expectInvalid("RigidWall;0;;;;;;;9", "id");
    expectInvalid("RigidWall;1", "nodeSetId");
    expectInvalid("RigidWall;1;;;;;;;0", "nodeSetId");
    expectInvalid("RigidWall;1;x;;;;;;9", "pointX");
    expectInvalid("RigidWall;1;;;;;;;9;SLIDE", "method");
    expectInvalid("RigidWall;1;;;;;;;9;;-0.1", "staticFriction");
    expectInvalid("RigidWall;1;;;;;;;9;;;-0.1", "kineticFriction");
    expectInvalid("RigidWall;1;;;;;;;9;;;;-0.1", "frictionDecay");
}

TEST_CASE("a DETSPH has its point, speed and time, which are zero when blank", "[model][cards][explicit]")
{
    Model model = readBulk(
        "SphericalDetonation;100;10;48.2;121.9;22.6;3105.;2.4E-6\nSphericalDetonation;101;11\nEulerMat;10;1.E+3;"
        "1\nGammaLawStateEquation;1");
    const SphericalDetonationCard example = cardAt<SphericalDetonationTable>(model, 0);
    REQUIRE((example.id == 100 && example.matId == 10 && example.pointX == 48.2 && example.pointY == 121.9 &&
             example.pointZ == 22.6));
    REQUIRE((example.detonationSpeed == 3105.0 && example.detonationTime == 2.4E-6));
    const SphericalDetonationCard bare = cardAt<SphericalDetonationTable>(model, 1);
    REQUIRE((bare.pointX == 0.0 && bare.pointY == 0.0 && bare.pointZ == 0.0 && bare.detonationSpeed == 0.0 &&
             bare.detonationTime == 0.0));
    requireRoundTrip(model);

    REQUIRE(unresolvedIn(model) == 1); // the MATDEUL 11
    RecordingVisitor visitor;
    model.table<SphericalDetonationTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE(cardAt<SphericalDetonationTable>(model, 0).matId == 1010);

    expectInvalid("SphericalDetonation;0;10", "id");
    expectInvalid("SphericalDetonation;1", "matId");
    expectInvalid("SphericalDetonation;1;0", "matId");
    expectInvalid("SphericalDetonation;1;10;x", "pointX");
    expectInvalid("SphericalDetonation;1;10;;;;-1.", "detonationSpeed");
    expectInvalid("SphericalDetonation;1;10;;;;;-1.E-6", "detonationTime");
}

TEST_CASE("a HGSUPPR resolves the method its class of elements defaults to, and keeps the other classes blank",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "HourglassSuppression;1;Shell;100;FlanaganBelytschkoViscous;0.1;0.1;0.1;;Yes;8000\n"
        "HourglassSuppression;2;Shell;101\n"
        "HourglassSuppression;3;Solid;102\n"
        "HourglassSuppression;4;Bar;103\n"
        "HourglassSuppression;5;Composite;104;ExternalExplicitProgram;0.05;0.06;0.07;0.08;Yes;5000\n"
        "HourglassSuppression;6;Composite;105\n"
        "ShellProp;100;1;0.1\n"
        "IsoMat;1;2.1E+11;;0.3");
    const HourglassSuppressionCard example = cardAt<HourglassSuppressionTable>(model, 0);
    REQUIRE((example.id == 1 && example.propKind == HourglassSuppressionProperty::Shell && example.propId == 100));
    REQUIRE(example.method == HourglassMethod::FlanaganBelytschkoViscous);
    REQUIRE((example.membraneCoefficient == 0.1 && example.solidCoefficient == 0.1 &&
             example.rotationCorrection == RotationCorrection::On));
    REQUIRE(example.stepCount == 8000);
    // A blank HGTYPE is stored as the default of the class: FBV for shells, FBS for solids, nothing for the others.
    REQUIRE(cardAt<HourglassSuppressionTable>(model, 1).method == HourglassMethod::FlanaganBelytschkoViscous);
    REQUIRE(cardAt<HourglassSuppressionTable>(model, 2).method == HourglassMethod::FlanaganBelytschkoStiffness);
    REQUIRE(!cardAt<HourglassSuppressionTable>(model, 3).method.has_value());
    REQUIRE(cardAt<HourglassSuppressionTable>(model, 5).method == HourglassMethod::FlanaganBelytschkoViscous);
    const HourglassSuppressionCard bare = cardAt<HourglassSuppressionTable>(model, 1);
    REQUIRE((bare.membraneCoefficient == 0.1 && bare.warpCoefficient == 0.1 && bare.twistCoefficient == 0.1 &&
             bare.solidCoefficient == 0.1));
    REQUIRE((bare.rotationCorrection == RotationCorrection::Off && bare.stepCount == 0));
    const HourglassSuppressionCard full = cardAt<HourglassSuppressionTable>(model, 4);
    REQUIRE(
        (full.propKind == HourglassSuppressionProperty::Composite && full.method == HourglassMethod::StandardViscous));
    REQUIRE((full.membraneCoefficient == 0.05 && full.warpCoefficient == 0.06 && full.twistCoefficient == 0.07 &&
             full.solidCoefficient == 0.08));
    REQUIRE((full.rotationCorrection == RotationCorrection::On && full.stepCount == 5000));
    requireRoundTrip(model);

    // The resolved method is written, so the card does not change meaning when its class is edited.

    // The PID is a property, whatever the class named by PROP.
    REQUIRE(unresolvedIn(model) == 5); // the properties 101 to 105: the PSHELL 100 is there
    RecordingVisitor visitor;
    model.table<HourglassSuppressionTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property});
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(cardAt<HourglassSuppressionTable>(model, 0).propId == 1100);

    expectInvalid("HourglassSuppression;0;Shell;1", "id");
    expectInvalid("HourglassSuppression;1", "propKind");
    expectInvalid("HourglassSuppression;1;TANK;1", "propKind");
    expectInvalid("HourglassSuppression;1;Shell", "propId");
    expectInvalid("HourglassSuppression;1;Shell;0", "propId");
    expectInvalid("HourglassSuppression;1;Shell;1;FBX", "method");
    expectInvalid("HourglassSuppression;1;Shell;1;;0.2", "membraneCoefficient");
    expectInvalid("HourglassSuppression;1;Shell;1;;-0.1", "membraneCoefficient");
    expectInvalid("HourglassSuppression;1;Shell;1;;;0.16", "warpCoefficient");
    expectInvalid("HourglassSuppression;1;Shell;1;;;;0.2", "twistCoefficient");
    expectInvalid("HourglassSuppression;1;Shell;1;;;;;0.151", "solidCoefficient");
    expectInvalid("HourglassSuppression;1;Shell;1;;;;;;MAYBE", "rotationCorrection");
    expectInvalid("HourglassSuppression;1;Shell;1;;;;;;;-5", "stepCount");
}

TEST_CASE("a BJOIN has its gaps in the layout, a tolerance and failure forces, and its SID is a contact body",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "SpotweldJoinPairs;1;2;;SpotWeld;2500.;1500.\n"
        "SpotweldJoinPairs;2;2\n"
        "SpotweldJoinPairs;3;2;0.001;SpotWeld;2.E+3;1.E+3;No;0.5\n"
        "ContactNodeRegion;2;5;6;7;Through;12");
    const SpotweldJoinPairsCard example = cardAt<SpotweldJoinPairsTable>(model, 0);
    REQUIRE((example.id == 1 && example.nodeSetId == 2 && example.type == SpotweldJoinPairsFailure::Spotweld));
    REQUIRE((example.tensileFailureForce == 2500.0 && example.shearFailureForce == 1500.0));
    REQUIRE((example.tolerance == 1.0E-4 && example.multiplePairs == SpotweldJoinPairsPairs::Given &&
             example.failureTime == 1.0E20)); // the defaults
    const SpotweldJoinPairsCard bare = cardAt<SpotweldJoinPairsTable>(model, 1);
    REQUIRE(
        (!bare.tensileFailureForce.has_value() && !bare.shearFailureForce.has_value())); // a blank force is no failure
    const SpotweldJoinPairsCard full = cardAt<SpotweldJoinPairsTable>(model, 2);
    REQUIRE((full.tolerance == 1.0E-3 && full.tensileFailureForce == 2000.0 && full.shearFailureForce == 1000.0));
    REQUIRE((full.multiplePairs == SpotweldJoinPairsPairs::Matched && full.failureTime == 0.5));
    requireRoundTrip(model);

    // The BCGRID 2 exists, so the joins add no dangling reference to those that the BCGRID itself has (its grid
    // points).
    Model onlyGrid = readBulk("ContactNodeRegion;2;5;6;7;Through;12");
    REQUIRE(unresolvedIn(model) == unresolvedIn(onlyGrid));
    RecordingVisitor visitor;
    model.table<SpotweldJoinPairsTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodeSetId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactBody});
    Renumbering::apply(model, RenumberSpace::ContactBody, Renumbering::offset(100));
    REQUIRE(cardAt<SpotweldJoinPairsTable>(model, 2).nodeSetId == 102);
    Model dangling = readBulk("SpotweldJoinPairs;1;9");
    REQUIRE(unresolvedIn(dangling) == 1);

    expectInvalid("SpotweldJoinPairs;0;2", "id");
    expectInvalid("SpotweldJoinPairs;1", "nodeSetId");
    expectInvalid("SpotweldJoinPairs;1;0", "nodeSetId");
    expectInvalid("SpotweldJoinPairs;1;2;0.", "tolerance");
    expectInvalid("SpotweldJoinPairs;1;2;-1.E-4", "tolerance");
    expectInvalid("SpotweldJoinPairs;1;2;;WELD", "failureKind");
    expectInvalid("SpotweldJoinPairs;1;2;;;0.", "tensileFailureForce");
    expectInvalid("SpotweldJoinPairs;1;2;;;;-1.", "shearFailureForce");
    expectInvalid("SpotweldJoinPairs;1;2;;;;;MAYBE", "multiplePairs");
    expectInvalid("SpotweldJoinPairs;1;2;;;;;;x", "failureTime");
}

TEST_CASE("a FORCUDS lists grid points with THRU and BY, and its LID is a set a TLOAD1 excites",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "UserEnforcedVelocity;1;PISTON;;1;2;3;5\n"
        "UserEnforcedVelocity;2;mygroup;myvel;10;Through;20;By;2;30;;;40;Through;50\n"
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "Node;3;;2.;0.;0.\n"
        "Node;5;;3.;0.;0.\n"
        "Node;30;;4.;0.;0.\n"
        "TransientTableLoad1;1;2;;;;;5.");
    const UserEnforcedVelocityCard example = cardAt<UserEnforcedVelocityTable>(model, 0);
    REQUIRE((example.setId == 1 && example.groupName == "PISTON" &&
             example.userRoutineName == "ExternalVelocity")); // UNAME defaults to EXTVEL
    REQUIRE(example.grids == std::vector<ContactRun>{{1, 1, 1}, {2, 2, 1}, {3, 3, 1}, {5, 5, 1}});
    const UserEnforcedVelocityCard runs = cardAt<UserEnforcedVelocityTable>(model, 1);
    REQUIRE((runs.groupName == "mygroup" && runs.userRoutineName == "myvel")); // names keep their exact spelling
    REQUIRE(runs.grids == std::vector<ContactRun>{{10, 20, 2}, {30, 30, 1}, {40, 50, 1}});
    requireRoundTrip(model);

    // A THRU range stays on one line when it is written, and a point inside a range may be missing from the model.

    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<UserEnforcedVelocityTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"node", "node", "node", "node", "node"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::NodeInRange, ReferenceTarget::NodeInRange,
                                                            ReferenceTarget::Node, ReferenceTarget::NodeInRange,
                                                            ReferenceTarget::NodeInRange});
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<UserEnforcedVelocityTable>(model, 0).grids.front() == ContactRun{101, 101, 1});
    REQUIRE(cardAt<UserEnforcedVelocityTable>(model, 1).grids ==
            std::vector<ContactRun>{{110, 120, 2}, {130, 130, 1}, {140, 150, 1}});
    Model dangling = readBulk("UserEnforcedVelocity;1;GRP;;7;Through;9");
    REQUIRE(unresolvedIn(dangling) == 0); // the ends of a range may be missing
    Model missing = readBulk("UserEnforcedVelocity;1;GRP;;7");
    REQUIRE(unresolvedIn(missing) == 1);

    expectInvalid("UserEnforcedVelocity;0;GRP;;1", "setId");
    expectInvalid("UserEnforcedVelocity;1;;;1", "groupName");
    REQUIRE_NOTHROW(readBulk("UserEnforcedVelocity;1;TOOLONGNAME;;1"));
    REQUIRE_NOTHROW(readBulk("UserEnforcedVelocity;1;GRP;TOOLONGNAME;1"));
    expectInvalid("UserEnforcedVelocity;1;GRP", "node"); // the points are required
    expectInvalid("UserEnforcedVelocity;1;GRP;;0", "node");
    expectInvalid("UserEnforcedVelocity;1;GRP;;-4", "node");
    expectInvalid("UserEnforcedVelocity;1;GRP;;5;Through;1", "node"); // a range goes upwards
    expectInvalid("UserEnforcedVelocity;1;GRP;;Through;5", "Through");
    expectInvalid("UserEnforcedVelocity;1;GRP;;1;Through", "Through");
    expectInvalid("UserEnforcedVelocity;1;GRP;;1;Through;5;By;0", "By");
    expectInvalid("UserEnforcedVelocity;1;GRP;;1;By;2", "By");
}

TEST_CASE("a GENUDS has the blocks INT, REAL and CHAR after its service name, and no ID", "[model][cards][explicit]")
{
    Model model = readBulk(
        "UserServiceData;PRB_SRV;IntegerBlock;3;4;7;250\n"
        "UserServiceData;srv\n"
        "UserServiceData;SRV2;IntegerBlock;9;1;2;3;4;5;6;7;8;9;RealBlock;2;1.5;2.5;TextBlock;2;abc;DEF\n"
        "UserServiceData;SRV3;TextBlock;1;ON;IntegerBlock;1;4\n");
    const UserServiceDataCard example = cardAt<UserServiceDataTable>(model, 0);
    REQUIRE((example.serviceId == "PRB_SRV" && example.integers == std::vector<std::int32_t>{4, 7, 250}));
    REQUIRE((example.reals.empty() && example.words.empty()));
    const UserServiceDataCard bare = cardAt<UserServiceDataTable>(model, 1);
    REQUIRE((bare.serviceId == "srv" && bare.integers.empty())); // the name keeps its exact spelling
    const UserServiceDataCard full = cardAt<UserServiceDataTable>(model, 2);
    REQUIRE(full.integers == std::vector<std::int32_t>{1, 2, 3, 4, 5, 6, 7, 8, 9}); // two lines of the INT block
    REQUIRE(full.reals == std::vector<double>{1.5, 2.5});
    REQUIRE(full.words == std::vector<std::string>{"abc", "DEF"});
    // The blocks come in any order and are written as INT, REAL and CHAR.
    const UserServiceDataCard reordered = cardAt<UserServiceDataTable>(model, 3);
    REQUIRE((reordered.integers == std::vector<std::int32_t>{4} && reordered.words == std::vector<std::string>{"ON"}));
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<UserServiceDataTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty()); // a GENUDS refers to no card
    REQUIRE(model.table<UserServiceDataTable>().idAtRow(0) == 0);

    expectInvalid("UserServiceData", "serviceId");
    REQUIRE_NOTHROW(readBulk("UserServiceData;TOOLONGSRV"));
    expectInvalid("UserServiceData;S;IntegerBlock;1;1;IntegerBlock;1;2", ""); // a block at most once
    expectInvalid("UserServiceData;S;IntegerBlock;3;1;;3", "");               // a gap inside a line of data
    expectInvalid("UserServiceData;S;IntegerBlock;1;x", "integerData");
    expectInvalid("UserServiceData;S;RealBlock;1;x", "realData");
    expectInvalid("UserServiceData;S;IntegerBlock", "integerData"); // a block has data
    expectInvalid("UserServiceData;S;TextBlock", "textData");
    expectInvalid("UserServiceData;S;TextBlock;1;IntegerBlock",
                  "textData"); // a word is never one of the three keywords
    expectInvalid("UserServiceData;S;TextBlock;1;RealBlock", "textData");
    REQUIRE_NOTHROW(readBulk("UserServiceData;S;TextBlock;1;TOOLONGWORD"));
}

TEST_CASE("a UDSESV gives the number of state variables and names some of them", "[model][cards][explicit]")
{
    Model model = readBulk(
        "UserStateVariables;3;SV2;VAR2;SV3;VAR3\n"
        "UserStateVariables;1\n"
        "UserStateVariables;8;SV2;temp2;SV3;;SV4;VAR4;SV5;VAR5;SV6;VAR6;SV7;VAR7\n");
    const UserStateVariablesCard example = cardAt<UserStateVariablesTable>(model, 0);
    REQUIRE(example.variableCount == 3);
    REQUIRE(example.names == std::vector<UserStateVariablesName>{{2, "VAR2"}, {3, "VAR3"}});
    REQUIRE((cardAt<UserStateVariablesTable>(model, 1).variableCount == 1 &&
             cardAt<UserStateVariablesTable>(model, 1).names.empty()));
    const UserStateVariablesCard full = cardAt<UserStateVariablesTable>(model, 2);
    REQUIRE(full.variableCount == 8);
    // A blank name uses its default SVi; other names keep their exact spelling.
    REQUIRE(full.names == std::vector<UserStateVariablesName>{
                              {2, "temp2"}, {3, "SV3"}, {4, "VAR4"}, {5, "VAR5"}, {6, "VAR6"}, {7, "VAR7"}});
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<UserStateVariablesTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty()); // a UDSESV refers to no card

    expectInvalid("UserStateVariables", "variableCount");
    expectInvalid("UserStateVariables;0", "variableCount");
    expectInvalid("UserStateVariables;101", "variableCount"); // at most 100 state variables, the temperature included
    expectInvalid("UserStateVariables;3;SV4;X", "variableLabel"); // there are three state variables
    expectInvalid("UserStateVariables;3;SV0;X", "variableLabel");
    expectInvalid("UserStateVariables;3;SV2;A;SV2;B", "variableLabel"); // a variable is named once
    expectInvalid("UserStateVariables;3;TEMP;A", "variableLabel");
    expectInvalid("UserStateVariables;3;SV;A", "variableLabel");
    expectInvalid("UserStateVariables;3;;A", "variableLabel");
    REQUIRE_NOTHROW(readBulk("UserStateVariables;3;SV2;TOOLONGNAME"));
}

TEST_CASE("a DYFSISW switches the fluid-structure interaction with a table", "[model][cards][explicit]")
{
    Model model = readBulk(
        "FluidStructureSwitch;14;10\nFluidStructureSwitch;15\nFluidStructureSwitch;16;11;Yes\nTableDynamic1;10;;;0.;1.;"
        "1.;-1.;EndTable");
    const FluidStructureSwitchCard example = cardAt<FluidStructureSwitchTable>(model, 0);
    REQUIRE(
        (example.id == 14 && example.tableId == 10 && example.eulerInitialization == EulerInitialization::AtCycleZero));
    const FluidStructureSwitchCard bare = cardAt<FluidStructureSwitchTable>(model, 1);
    REQUIRE((bare.tableId == 0 && bare.eulerInitialization == EulerInitialization::AtCycleZero)); // NO is the default
    REQUIRE(cardAt<FluidStructureSwitchTable>(model, 2).eulerInitialization == EulerInitialization::WhenActive);
    requireRoundTrip(model);

    REQUIRE(unresolvedIn(model) == 1); // the TABLED1 11
    RecordingVisitor visitor;
    model.table<FluidStructureSwitchTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"tableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::DynamicTable});
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE(cardAt<FluidStructureSwitchTable>(model, 0).tableId == 110);

    REQUIRE_THROWS_AS(readBulk("FluidStructureSwitch;14\nFluidStructureSwitch;14"),
                      base::DeckError); // the SWID is unique

    expectInvalid("FluidStructureSwitch;0;10", "id");
    expectInvalid("FluidStructureSwitch;1;-10", "tableId");
    expectInvalid("FluidStructureSwitch;1;10;MAYBE", "eulerInitialization");
}
