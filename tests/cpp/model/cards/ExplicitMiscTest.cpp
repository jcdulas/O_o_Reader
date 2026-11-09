// Checks of the explicit solver cards that have no group of their own: the ATB cards of SOL 700 (ATBACC, ATBJNT,
// ATBSEG), the rigid joint RBJOINT, the external ellipsoid RELEX, the contact cards of several modules (MDBCATP,
// MDBCNCT, MDBCPAR, MDBCTB1), the acceleration loads ACCEL and ACCEL1, and the legacy contact cards (BCTSET, BCTPARM,
// BCTPARA) with the BCTADD that joins them: field positions, defaults, references, ID spaces, renumbering and
// refused values.

#include "oor/model/cards/explicit/occupantAccelField/OccupantAccelFieldTable.hpp"
#include "oor/model/cards/explicit/occupantJoint/OccupantJointTable.hpp"
#include "oor/model/cards/explicit/occupantSegment/OccupantSegmentTable.hpp"
#include "oor/model/cards/explicit/rigidBodyJoint/RigidBodyJointTable.hpp"
#include "oor/model/cards/explicit/rigidEllipsoidExternal/RigidEllipsoidExternalTable.hpp"
#include "oor/model/cards/loads/accelFieldLoad/AccelFieldLoadTable.hpp"
#include "oor/model/cards/loads/accelNodeLoad/AccelNodeLoadTable.hpp"
#include "oor/model/cards/nodes/moduleContactCandidates/ModuleContactCandidatesTable.hpp"
#include "oor/model/cards/nodes/moduleContactDetectParams/ModuleContactDetectParamsTable.hpp"
#include "oor/model/cards/nodes/moduleContactPair/ModuleContactPairTable.hpp"
#include "oor/model/cards/nodes/moduleContactTable/ModuleContactTableTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactControlParams/ContactControlParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSet/ContactSetTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSetUnion/ContactSetUnionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSurfaceParams/ContactSurfaceParamsTable.hpp"
#include "support/ExplicitTestSupport.hpp"
#include "support/ModelRecord.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

using Grids4 = std::array<base::EntityId, 4>;
using Grids8 = std::array<base::EntityId, 8>;
using Elements3 = std::array<base::EntityId, 3>;
using Elements6 = std::array<base::EntityId, 6>;

// Node records for the numbers (all at the origin: only the IDs matter to these tests).
std::string gridCards(std::initializer_list<int> ids)
{
    std::string text;
    for (const int id : ids)
        text += oor::testing::modelRecord("Node", id, "", 0.0, 0.0, 0.0);
    return text;
}

// The fields of the card at `row` of a table whose reference points at nothing.
template <typename TableT>
std::vector<std::string> unresolvedFields(Model& model, std::uint32_t row = 0)
{
    std::vector<std::string> fields;
    for (const UnresolvedReference& reference : unresolvedAt(model, CardLocation{typeIndexOf<TableT>(), row}))
        fields.push_back(reference.field);
    return fields;
}

// The references a card of a table lists.
template <typename TableT>
RecordingVisitor referencesOf(Model& model, std::uint32_t row = 0)
{
    RecordingVisitor visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return visitor;
}

} // namespace

// ------------------------------------------------------------------------------------------------------- ATB cards

TEST_CASE("an ATBJNT has eight grid points, which it refers to, and six numbers of elements to generate",
          "[model][cards][explicit]")
{
    const std::string joints =
        "OccupantJoint;1;NK;1410;1411;1412;1413;1024;1025;1026;2410;2411;2412;2413;2034;2035;2036\n"
        "OccupantJoint;2;knee;1410;1411;1412;1413;;;;2410;2411;2412;2413\n";
    Model model = readBulk(gridCards({1410, 1411, 1412, 1413, 2410, 2411, 2412, 2413}) + joints);
    const OccupantJointCard full = cardAt<OccupantJointTable>(model, 0);
    REQUIRE((full.id == 1 && full.name == "NK"));
    REQUIRE(full.grids == Grids8{1410, 1411, 1412, 1413, 2410, 2411, 2412, 2413});
    REQUIRE(full.elemIds == Elements6{1024, 1025, 1026, 2034, 2035, 2036});
    const OccupantJointCard bare = cardAt<OccupantJointTable>(model, 1);
    REQUIRE(bare.name == "knee");
    REQUIRE(bare.elemIds == Elements6{});

    REQUIRE(unresolvedIn(model) == 0);
    const RecordingVisitor visitor = referencesOf<OccupantJointTable>(model);
    REQUIRE(visitor.fields ==
            std::vector<std::string>{"orientationNode", "node1", "node2", "node3", "node4", "node5", "node6", "node7"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(8, ReferenceTarget::Node));
    Model lacking = readBulk(joints);
    REQUIRE(unresolvedIn(lacking) == 16); // the element numbers name nothing
    requireRoundTrip(model);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<OccupantJointTable>(model, 0).grids == Grids8{1510, 1511, 1512, 1513, 2510, 2511, 2512, 2513});
    REQUIRE(cardAt<OccupantJointTable>(model, 0).elemIds.front() == 1024); // an element number is not a reference
    REQUIRE(unresolvedIn(model) == 0);

    REQUIRE_THROWS_AS(readBulk("OccupantJoint;1;A;1;2;3;4;;;;5;6;7;8\nOccupantJoint;1;B;1;2;3;4;;;;5;6;7;8"),
                      base::DeckError);
    expectInvalid("OccupantJoint;0;NK;1;2;3;4;;;;5;6;7;8", "id");
    expectInvalid("OccupantJoint;1;;1;2;3;4;;;;5;6;7;8", "jointName");
    expectInvalid("OccupantJoint;1;NK;1;2;3;4;;;;5;6;7", "node7");
    expectInvalid("OccupantJoint;1;NK;1;2;3;4;-5;;;5;6;7;8", "elemId1");
}

TEST_CASE("an ATBSEG has a keyword, numbers to generate and four grid points, which it refers to",
          "[model][cards][explicit]")
{
    const std::string segments =
        "OccupantSegment;1;HEAD;Yes;80;1200;1300;1400;;1610;1611;1612;1613;1301;1302;1303;1305\n"
        "OccupantSegment;2;chest;;;;;;;11;12;13;14\n"
        "OccupantSegment;3;PELVIS;No;50;2000;3000;4000;5000;11;12;13;14;21;22;23;6000\n";
    Model model = readBulk(gridCards({1610, 1611, 1612, 1613, 11, 12, 13, 14}) + segments);
    const OccupantSegmentCard example = cardAt<OccupantSegmentTable>(model, 0);
    REQUIRE((example.id == 1 && example.name == "HEAD" && example.cover == OccupantSegmentCover::Yes));
    REQUIRE((example.elemCount == 80 && example.firstNodeId == 1200 && example.firstElemId == 1300 &&
             example.matId == 1400));
    REQUIRE((example.coverPropId == 0 && example.centerOfGravityPropId == 1305)); // the example leaves PIDCOV blank
    REQUIRE(example.grids == Grids4{1610, 1611, 1612, 1613});
    REQUIRE(example.elemIds == Elements3{1301, 1302, 1303});
    const OccupantSegmentCard bare = cardAt<OccupantSegmentTable>(model, 1);
    REQUIRE((bare.name == "chest" && !bare.cover.has_value() && bare.elemCount == 0 && bare.matId == 0 &&
             bare.centerOfGravityPropId == 0));
    REQUIRE(bare.elemIds == Elements3{});
    const OccupantSegmentCard full = cardAt<OccupantSegmentTable>(model, 2);
    REQUIRE((full.cover == OccupantSegmentCover::No && full.elemCount == 50 && full.firstNodeId == 2000 &&
             full.firstElemId == 3000));
    REQUIRE((full.matId == 4000 && full.coverPropId == 5000 && full.centerOfGravityPropId == 6000));

    // Only the grid points are references: the material and the properties are given to cards SOL 700 generates.
    REQUIRE(unresolvedIn(model) == 0);
    const RecordingVisitor visitor = referencesOf<OccupantSegmentTable>(model, 2);
    REQUIRE(visitor.fields == std::vector<std::string>{"orientationNode", "node1", "node2", "node3"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(4, ReferenceTarget::Node));
    Model lacking = readBulk(segments);
    REQUIRE(unresolvedIn(lacking) == 12);
    requireRoundTrip(model);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<OccupantSegmentTable>(model, 0).grids == Grids4{1710, 1711, 1712, 1713});
    REQUIRE((cardAt<OccupantSegmentTable>(model, 0).matId == 1400 &&
             cardAt<OccupantSegmentTable>(model, 0).firstNodeId == 1200));

    REQUIRE_THROWS_AS(readBulk("OccupantSegment;1;A;;;;;;;1;2;3;4\nOccupantSegment;1;B;;;;;;;1;2;3;4"),
                      base::DeckError);
    expectInvalid("OccupantSegment;0;HEAD;;;;;;;1;2;3;4", "id");
    expectInvalid("OccupantSegment;1;;;;;;;;1;2;3;4", "segmentName");
    expectInvalid("OccupantSegment;1;HEAD;MAYBE;;;;;;1;2;3;4", "cover");
    expectInvalid("OccupantSegment;1;HEAD;;-5;;;;;1;2;3;4", "elemCount");
    expectInvalid("OccupantSegment;1;HEAD;;;;;;;1;2;3", "node3");
    expectInvalid("OccupantSegment;1;HEAD;;;;;;;1;2;3;4;-7", "elemId1");
}

TEST_CASE("an RBJOINT has the nodes its type needs and a relative penalty stiffness in field 10",
          "[model][cards][explicit]")
{
    Model model = readBulk(gridCards({105, 207, 109, 212, 1, 2, 3, 4, 5, 6}) +
                           "RigidBodyJoint;1;CylindricalJoint;105;207;109;212\n"
                           "RigidBodyJoint;2;SphericalJoint;105;207\n"
                           "RigidBodyJoint;3;RevoluteJoint;1;2;3;4\n"
                           "RigidBodyJoint;4;PlanarJoint;1;2;3;4\n"
                           "RigidBodyJoint;5;UniversalJoint;1;2;3;4\n"
                           "RigidBodyJoint;6;TranslationalJoint;1;2;3;4;5;6;2.5\n");
    const std::vector<RigidBodyJointType> types = {RigidBodyJointType::Cylindrical, RigidBodyJointType::Spherical,
                                                   RigidBodyJointType::Revolute,    RigidBodyJointType::Planar,
                                                   RigidBodyJointType::Universal,   RigidBodyJointType::Translational};
    const std::vector<std::size_t> counts = {4, 2, 4, 4, 4, 6};
    for (std::uint32_t row = 0; row < types.size(); ++row)
    {
        const RigidBodyJointCard joint = cardAt<RigidBodyJointTable>(model, row);
        INFO(row);
        REQUIRE(joint.id == static_cast<base::EntityId>(row + 1));
        REQUIRE(joint.type == types[row]);
        REQUIRE(joint.nodes.size() == counts[row]);
        REQUIRE(joint.penaltyScale == (row == 5 ? 2.5 : 1.0)); // RPS defaults to 1.0
    }
    REQUIRE(cardAt<RigidBodyJointTable>(model, 0).nodes == std::vector<base::EntityId>{105, 207, 109, 212});

    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(referencesOf<RigidBodyJointTable>(model, 1).fields == std::vector<std::string>{"node1", "node2"});
    const RecordingVisitor all = referencesOf<RigidBodyJointTable>(model, 5);
    REQUIRE(all.fields == std::vector<std::string>{"node1", "node2", "node3", "node4", "node5", "node6"});
    REQUIRE(all.targets == std::vector<ReferenceTarget>(6, ReferenceTarget::Node));
    Model lacking = readBulk("RigidBodyJoint;1;CylindricalJoint;105;207;109;212");
    REQUIRE(unresolvedIn(lacking) == 4);
    requireRoundTrip(model);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<RigidBodyJointTable>(model, 0).nodes == std::vector<base::EntityId>{1105, 1207, 1109, 1212});
    REQUIRE(unresolvedIn(model) == 0);

    REQUIRE_THROWS_AS(readBulk("RigidBodyJoint;1;SphericalJoint;1;2\nRigidBodyJoint;1;SphericalJoint;3;4"),
                      base::DeckError);
    expectInvalid("RigidBodyJoint;0;SphericalJoint;1;2", "id");
    expectInvalid("RigidBodyJoint;1;BALL;1;2", "jointType");
    expectInvalid("RigidBodyJoint;1;;1;2", "jointType");
    expectInvalid("RigidBodyJoint;1;RevoluteJoint;1;2", "node3");          // a revolute joint has four nodes
    expectInvalid("RigidBodyJoint;1;TranslationalJoint;1;2;3;4", "node5"); // a translational one has six
    expectInvalid("RigidBodyJoint;1;SphericalJoint;1;2;3", "");            // a spherical joint has two
    expectInvalid("RigidBodyJoint;1;RevoluteJoint;1;2;3;4;5", "");         // N5 is for TRANSL only
    expectInvalid("RigidBodyJoint;1;SphericalJoint;1;-2", "node2");
    expectInvalid("RigidBodyJoint;1;SphericalJoint;1;2;;;;;-1.", "penaltyScale");
}

TEST_CASE("a RELEX is found by its name and gives the program that defines the ellipsoid", "[model][cards][explicit]")
{
    const Model model = readBulk("RigidEllipsoidExternal;head;atb\nRigidEllipsoidExternal;CHEST;ATB");
    REQUIRE((cardAt<RigidEllipsoidExternalTable>(model, 0).name == "head" &&
             cardAt<RigidEllipsoidExternalTable>(model, 0).programName == "atb"));
    REQUIRE(cardAt<RigidEllipsoidExternalTable>(model, 1).name == "CHEST");
    REQUIRE(model.findTable<RigidEllipsoidExternalTable>()->idAtRow(0) == 0); // no number
    requireRoundTrip(model);

    // A name names one ellipsoid.
    REQUIRE_THROWS_AS(readBulk("RigidEllipsoidExternal;HEAD;ATB\nRigidEllipsoidExternal;HEAD;ATB"), base::DeckError);
    const Model tolerant =
        readModelText("[model]\nRigidEllipsoidExternal;HEAD;ATB\nRigidEllipsoidExternal;HEAD;ATB\n", false);
    REQUIRE(tolerant.diagnostics().firstError()->code == "deck.duplicate-rigid-ellipsoid-external");

    expectInvalid("RigidEllipsoidExternal;;ATB", "ellipsoidName");
    expectInvalid("RigidEllipsoidExternal;HEAD", "programName");
    oor::testing::requireModelFileRoundTrip(readModelText("RigidEllipsoidExternal;TOOLONGNAME;ATB", true));
}

// -------------------------------------------------------------------------------- contact cards across modules

TEST_CASE("an MDBCATP is a parameter list from field 3 that takes the names of its format and of three other cards",
          "[model][cards][explicit]")
{
    const Model model = readBulk("ModuleContactDetectParams;DetectionDistance;0.45;DeleteSolids;Yes");
    const ModuleContactDetectParamsCard example = cardAt<ModuleContactDetectParamsTable>(model);
    REQUIRE(example.params.size() == 2);
    REQUIRE((example.params[0].name == "DetectionDistance" && std::get<double>(example.params[0].value) == 0.45));
    REQUIRE((example.params[1].name == "DeleteSolids" && std::get<std::string>(example.params[1].value) == "Yes"));
    requireRoundTrip(model);

    // The words of the format, a table ID in FRIC, FTYPE as a word or a number, and the names of BCPARA and BCONPRG.
    Model rich = readBulk(
        "ModuleContactDetectParams;ContactKind;PermanentGlue;DetectionDistance;0.5;BeamContact;Yes;EdgeContact;No;"
        "FrictionKind;"
        "BilinearCoulomb;FrictionCoefficient;7;IgnoreThickness;No;InitialStiffness;Yes;SelfContact;No;"
        "ContactDistanceTolerance;"
        "0.01;ContactSearchOrder;1");
    const ModuleContactDetectParamsCard card = cardAt<ModuleContactDetectParamsTable>(rich);
    REQUIRE(card.params.size() == 11);
    // FRIC as a table ID
    REQUIRE((card.params[5].name == "FrictionCoefficient" && std::get<std::int64_t>(card.params[5].value) == 7));
    REQUIRE(card.params[8].name == "SelfContact"); // a keyword in any case gives the neutral name
    REQUIRE(card.params[10].name == "ContactSearchOrder");
    REQUIRE(referencesOf<ModuleContactDetectParamsTable>(rich).fields.empty());

    // One card in a deck.
    REQUIRE_THROWS_AS(readBulk("ModuleContactDetectParams\nModuleContactDetectParams"), base::DeckError);
    const Model tolerant = readModelText("[model]\nModuleContactDetectParams\nModuleContactDetectParams\n", false);
    REQUIRE(tolerant.diagnostics().firstError()->code == "deck.duplicate-module-contact-detect-params");

    expectInvalid("ModuleContactDetectParams;BOGUS;1", "parameterName");
    expectInvalid("ModuleContactDetectParams;DetectionDistance;ABC", "parameterValue");
    expectInvalid("ModuleContactDetectParams;ContactKind;5", "parameterValue");
    expectInvalid("ModuleContactDetectParams;;5", "parameterName");
}

TEST_CASE("an MDBCPAR has SET3ID in field 2 and pairs of modules, which it reads in order", "[model][cards][explicit]")
{
    Model model = readBulk("ModuleContactCandidates;5;1;6;8;9\nTypedListSet3;5;Module;1;6;8;9");
    const ModuleContactCandidatesCard example = cardAt<ModuleContactCandidatesTable>(model);
    REQUIRE(example.moduleSetId == 5);
    using Pairs = std::vector<std::pair<base::EntityId, base::EntityId>>;
    REQUIRE(example.pairs == Pairs{{1, 6}, {8, 9}});
    REQUIRE(unresolvedIn(model) == 0);
    const RecordingVisitor visitor = referencesOf<ModuleContactCandidatesTable>(model);
    REQUIRE(visitor.fields == std::vector<std::string>{"moduleSetId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Set});
    Model lacking = readBulk("ModuleContactCandidates;5");
    REQUIRE(unresolvedIn(lacking) == 1);
    requireRoundTrip(model);
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(10));
    REQUIRE(cardAt<ModuleContactCandidatesTable>(model).moduleSetId == 15);
    REQUIRE(unresolvedIn(model) == 0);

    // -1 (the default) and 0 name no set.
    Model open = readBulk("ModuleContactCandidates");
    REQUIRE((cardAt<ModuleContactCandidatesTable>(open).moduleSetId == -1 &&
             cardAt<ModuleContactCandidatesTable>(open).pairs.empty()));
    REQUIRE(referencesOf<ModuleContactCandidatesTable>(open).fields.empty());
    const Model none = readBulk("ModuleContactCandidates;0");
    REQUIRE(cardAt<ModuleContactCandidatesTable>(none).moduleSetId == 0);
    const Model packed = readBulk("ModuleContactCandidates;-1;1;2;3;4;5;6");
    REQUIRE(cardAt<ModuleContactCandidatesTable>(packed).pairs == Pairs{{1, 2}, {3, 4}, {5, 6}});
    requireRoundTrip(packed);

    REQUIRE_THROWS_AS(readBulk("ModuleContactCandidates\nModuleContactCandidates;0"), base::DeckError);
    expectInvalid("ModuleContactCandidates;-2", "moduleSetId");
}

TEST_CASE("an MDBCTB1 is a contact table of pairs of module and contact pair, refers to the ones of the main Bulk Data",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "ModuleContactTable;0;1003;41;1004;67\n"
        "ModuleContactTable;470;;462;;461\n"
        "ModuleContactTable;471;102;174;202;83;302;96;402;502;602;147;702;38\n"
        "ContactPair;461;;;110;111\n"
        "ModuleContactPair;462;;;;5;;6");
    const ModuleContactTableCard example = cardAt<ModuleContactTableTable>(model, 0);
    REQUIRE(example.id == 0); // the table of load case 0
    REQUIRE(example.pairs == std::vector<ModuleId>{{1003, 41}, {1004, 67}});
    REQUIRE(cardAt<ModuleContactTableTable>(model, 1).pairs ==
            std::vector<ModuleId>{{0, 462}, {0, 461}}); // a blank module is 0
    const ModuleContactTableCard pairs = cardAt<ModuleContactTableTable>(model, 2);
    REQUIRE(pairs.pairs.size() == 6);
    REQUIRE(
        (pairs.pairs[3] == ModuleId{402, 502} && pairs.pairs[5] == ModuleId{702, 38})); // MOD4 and ID4 are on two lines

    // The pairs of the main Bulk Data are contact pairs (a BCONECT or an MDBCNCT); the others are in another module.
    REQUIRE(unresolvedFields<ModuleContactTableTable>(model, 0).empty());
    REQUIRE(unresolvedFields<ModuleContactTableTable>(model, 1).empty());
    const RecordingVisitor visitor = referencesOf<ModuleContactTableTable>(model, 1);
    REQUIRE(visitor.fields == std::vector<std::string>{"contactPairId", "contactPairId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::ContactPair));
    REQUIRE(referencesOf<ModuleContactTableTable>(model, 0).fields.empty());
    Model lacking = readBulk("ModuleContactTable;470;;462;;461");
    REQUIRE(unresolvedIn(lacking) == 2);
    requireRoundTrip(model);

    Renumbering::apply(model, RenumberSpace::ContactPair, Renumbering::offset(1000));
    REQUIRE(cardAt<ModuleContactTableTable>(model, 1).pairs == std::vector<ModuleId>{{0, 1462}, {0, 1461}});
    REQUIRE(cardAt<ModuleContactTableTable>(model, 0).pairs.front().id == 41); // in module 1003: left alone
    REQUIRE(unresolvedFields<ModuleContactTableTable>(model, 1).empty());

    // The BCID is a contact table number: renumbered with the other contact tables.
    Model tables = readBulk("ModuleContactTable;470;1;2\nContactTable1;340;330");
    Renumbering::apply(tables, RenumberSpace::ContactTable, Renumbering::offset(100));
    REQUIRE(cardAt<ModuleContactTableTable>(tables).id == 570);

    REQUIRE_THROWS_AS(readBulk("ModuleContactTable;5;1;2\nModuleContactTable;5;1;3"), base::DeckError);
    expectInvalid("ModuleContactTable;-1;1;2", "id");
    expectInvalid("ModuleContactTable;5", "contactPairId");
    expectInvalid("ModuleContactTable;5;1", "contactPairId");         // a module without its ID
    expectInvalid("ModuleContactTable;5;1;2;;;3;4", "contactPairId"); // blank fields between the pairs
    expectInvalid("ModuleContactTable;5;1;-2", "contactPairId");
    expectInvalid("ModuleContactTable;5;-1;2", "module");
}

TEST_CASE("an MDBCNCT has a short form and SECNDRY and PRIMARY lists of modules and bodies", "[model][cards][explicit]")
{
    Model model = readBulk(
        "ModuleContactPair;64;318;;103;4;205;1007\n"
        "ModuleContactPair;9;;112;;;;;Secondary;2;103;35;205;22;Primary;3;103;276;205;141;307;513\n"
        "ModuleContactPair;462;310;320;;20;;30\n"
        "ContactPairGeomParams;310;ContactSearchOrder;1\n"
        "ContactPairPhysicalParams;320;FrictionCoefficient;0.2\n"
        "ContactBody1;20;;ThreeDimensional;Deformable;200\n"
        "ContactBody1;30;;ThreeDimensional;Deformable;200\n"
        "ContactElemSurface;200;1;2");
    const ModuleContactPairCard example = cardAt<ModuleContactPairTable>(model, 0);
    REQUIRE((example.id == 64 && example.geometricParamsId == 318 && example.physicalParamsId == 0));
    REQUIRE((example.secondaryModule == 103 && example.secondaryBodyId == 4 && example.primaryModule == 205 &&
             example.primaryBodyId == 1007));
    REQUIRE((example.secondary.empty() && example.primary.empty()));
    const ModuleContactPairCard lists = cardAt<ModuleContactPairTable>(model, 1);
    REQUIRE((lists.id == 9 && lists.physicalParamsId == 112 && lists.secondaryBodyId == 0 && lists.primaryBodyId == 0));
    REQUIRE(lists.secondary == std::vector<ModuleId>{{103, 35}, {205, 22}});
    REQUIRE(lists.primary == std::vector<ModuleId>{{103, 276}, {205, 141}, {307, 513}});
    const ModuleContactPairCard main = cardAt<ModuleContactPairTable>(model, 2);
    REQUIRE((main.secondaryModule == 0 && main.secondaryBodyId == 20 && main.primaryModule == 0 &&
             main.primaryBodyId == 30)); // blank modules: module 0

    // The parameter sets are always looked up; a body only when its module is the main Bulk Data.
    REQUIRE(unresolvedFields<ModuleContactPairTable>(model, 0) == std::vector<std::string>{"geometricParamsId"});
    REQUIRE(unresolvedFields<ModuleContactPairTable>(model, 1) == std::vector<std::string>{"physicalParamsId"});
    REQUIRE(unresolvedFields<ModuleContactPairTable>(model, 2).empty());
    const RecordingVisitor visitor = referencesOf<ModuleContactPairTable>(model, 2);
    REQUIRE(visitor.fields ==
            std::vector<std::string>{"geometricParamsId", "physicalParamsId", "secondaryBodyId", "primaryBodyId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactGeometricParameters,
                                                            ReferenceTarget::ContactPhysicalParameters,
                                                            ReferenceTarget::ContactBody,
                                                            ReferenceTarget::ContactBody});
    REQUIRE(referencesOf<ModuleContactPairTable>(model, 0).fields == std::vector<std::string>{"geometricParamsId"});
    requireRoundTrip(model);

    Renumbering::apply(model, RenumberSpace::ContactBody, Renumbering::offset(100));
    REQUIRE((cardAt<ModuleContactPairTable>(model, 2).secondaryBodyId == 120 &&
             cardAt<ModuleContactPairTable>(model, 2).primaryBodyId == 130));
    REQUIRE(cardAt<ModuleContactPairTable>(model, 0).secondaryBodyId == 4); // body 4 of module 103
    Renumbering::apply(model, RenumberSpace::ContactPair, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<ModuleContactPairTable>(model, 0).id == 1064 && cardAt<ModuleContactPairTable>(model, 2).id == 1462));

    // Every field, the lists on several lines, a blank module and the two forms together.
    const Model full = readBulk(
        "ModuleContactPair;461;300;310;1;20;2;30;Secondary;4;3;40;;41;4;42;5;43;Primary;4;6;50;7;51;8;52;9;53");
    const ModuleContactPairCard every = cardAt<ModuleContactPairTable>(full);
    REQUIRE(every.secondary == std::vector<ModuleId>{{3, 40}, {0, 41}, {4, 42}, {5, 43}});
    REQUIRE(every.primary == std::vector<ModuleId>{{6, 50}, {7, 51}, {8, 52}, {9, 53}});
    requireRoundTrip(full);

    REQUIRE_THROWS_AS(readBulk("ModuleContactPair;5;;;;1;;2\nModuleContactPair;5;;;;3;;4"), base::DeckError);
    expectInvalid("ModuleContactPair;-1;;;;2;;3", "id");
    expectInvalid("ModuleContactPair;57", "secondaryBodyId");         // no touching body
    expectInvalid("ModuleContactPair;57;;;;2", "primaryBodyId");      // no touched body
    expectInvalid("ModuleContactPair;57;;;3;;;4", "secondaryBodyId"); // a module without its body
    expectInvalid("ModuleContactPair;57;;;;2;;3;Secondary;1;1", "secondaryBodyIds");
    expectInvalid("ModuleContactPair;57;;;;2;;3;Secondary;1;1;-2", "secondaryBodyIds");
}

// ----------------------------------------------------------------------------------------------- acceleration loads

TEST_CASE("an ACCEL has a vector, an axis and a table of at least two pairs from field 9", "[model][cards][explicit]")
{
    Model model = readBulk(
        "AccelFieldLoad;100;2;0.;1.5;2.5;XDirection;0.;1.;800.;2.5\n"
        "AccelFieldLoad;101;;1.;;;ZDirection;0.;1.;5.;2.\n"
        "AccelFieldLoad;102;3;0.5;-1.5;2.5;YDirection;-1.;0.5;0.;1.;1.;1.5;2.;2.;3.;0.1\n"
        "RectFrame2;2\n"
        "RectFrame2;3");
    const AccelFieldLoadCard example = cardAt<AccelFieldLoadTable>(model, 0);
    REQUIRE((example.id == 100 && example.frameId == 2 && example.directionX == 0.0 && example.directionY == 1.5 &&
             example.directionZ == 2.5));
    REQUIRE(example.direction == AccelFieldLoadDirection::X);
    using Points = std::vector<std::pair<double, double>>;
    REQUIRE(example.points == Points{{0.0, 1.0}, {800.0, 2.5}});
    const AccelFieldLoadCard basic = cardAt<AccelFieldLoadTable>(model, 1);
    REQUIRE((basic.frameId == 0 && basic.directionX == 1.0 &&
             basic.direction == AccelFieldLoadDirection::Z)); // a blank CID is the basic system
    const AccelFieldLoadCard longTable = cardAt<AccelFieldLoadTable>(model, 2);
    REQUIRE(longTable.direction == AccelFieldLoadDirection::Y);
    REQUIRE(longTable.points == Points{{-1.0, 0.5}, {0.0, 1.0}, {1.0, 1.5}, {2.0, 2.0}, {3.0, 0.1}});

    // CID is the only reference; the basic system is not visited.
    REQUIRE(unresolvedFields<AccelFieldLoadTable>(model, 0).empty());
    REQUIRE(referencesOf<AccelFieldLoadTable>(model, 0).fields == std::vector<std::string>{"frameId"});
    REQUIRE(referencesOf<AccelFieldLoadTable>(model, 0).targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Coordinate});
    REQUIRE(referencesOf<AccelFieldLoadTable>(model, 1).fields.empty());
    Model lacking = readBulk("AccelFieldLoad;100;2;0.;1.5;2.5;XDirection;0.;1.;800.;2.5");
    REQUIRE(unresolvedFields<AccelFieldLoadTable>(lacking) == std::vector<std::string>{"frameId"});
    requireRoundTrip(model);

    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(
        (cardAt<AccelFieldLoadTable>(model, 0).frameId == 12 && cardAt<AccelFieldLoadTable>(model, 2).frameId == 13));
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(1000));
    REQUIRE(cardAt<AccelFieldLoadTable>(model, 0).id == 1100);

    // The ID is a load set shared by the cards of the set.
    REQUIRE(rowCount<AccelFieldLoadTable>(readBulk(
                "AccelFieldLoad;100;;1.;;;XDirection;0.;1.;1.;2.\nAccelFieldLoad;100;;;1.;;YDirection;0.;1.;1.;2.")) ==
            2);

    expectInvalid("AccelFieldLoad;0;;1.;;;XDirection;0.;1.;1.;2.", "id");
    expectInvalid("AccelFieldLoad;1;-1;1.;;;XDirection;0.;1.;1.;2.", "frameId");
    expectInvalid("AccelFieldLoad;1;;;;;XDirection;0.;1.;1.;2.", "directionX"); // the vector is zero
    expectInvalid("AccelFieldLoad;1;;1.;;;W;0.;1.;1.;2.", "direction");
    expectInvalid("AccelFieldLoad;1;;1.;;;;0.;1.;1.;2.", "direction");
    expectInvalid("AccelFieldLoad;1;;1.;;;XDirection;0.;1.", "location"); // one pair
    expectInvalid("AccelFieldLoad;1;;1.;;;XDirection;0.;1.;1.", "acceleration");
}

TEST_CASE("an ACCEL1 lists grid points with THRU and BY and refers to them", "[model][cards][explicit]")
{
    Model model = readBulk(
        "AccelNodeLoad;100;2;12.;1.;3.;0.;1;2;3;5;Through;11;By;2;24;25;Through;34;41;57;63;66;78;85;96\n"
        "AccelNodeLoad;101;;9.81;;;-1.;1;2;3\n"
        "AccelNodeLoad;102;3;2.5;1.;1.;1.;10;Through;20");
    const AccelNodeLoadCard example = cardAt<AccelNodeLoadTable>(model, 0);
    REQUIRE((example.id == 100 && example.frameId == 2 && example.magnitude == 12.0));
    REQUIRE((example.directionX == 1.0 && example.directionY == 3.0 && example.directionZ == 0.0));
    REQUIRE(example.grids.size() == 13);
    REQUIRE(example.grids[3] == ContactRun{5, 11, 2}); // the run keeps its BY
    REQUIRE(example.grids[5] == ContactRun{25, 34, 1});
    const std::vector<base::EntityId> expanded = expandContactRuns(example.grids);
    REQUIRE(expanded.size() == 3 + 4 + 1 + 10 + 4 + 3);
    REQUIRE((expanded[3] == 5 && expanded[4] == 7 && expanded[6] == 11 && expanded.back() == 96));
    const AccelNodeLoadCard plain = cardAt<AccelNodeLoadTable>(model, 1);
    REQUIRE((plain.frameId == 0 && plain.magnitude == 9.81 && plain.directionX == 0.0 && plain.directionZ == -1.0));

    // A single grid point must exist; the ends of a THRU range may name nothing (the points between them are skipped).
    Model lacking = readBulk("AccelNodeLoad;1;;9.81;;;-1.;1;5;Through;9");
    REQUIRE(unresolvedFields<AccelNodeLoadTable>(lacking) == std::vector<std::string>{"nodeId"});
    const RecordingVisitor visitor = referencesOf<AccelNodeLoadTable>(lacking);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodeId", "nodeId", "nodeId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::NodeInRange,
                                                            ReferenceTarget::NodeInRange});
    requireRoundTrip(model);

    Model grids =
        readBulk("Node;1;;0.;0.;0.\nNode;5;;0.;0.;0.\nNode;9;;0.;0.;0.\nAccelNodeLoad;1;;9.81;;;-1.;1;5;Through;9");
    REQUIRE(unresolvedIn(grids) == 0);
    Renumbering::apply(grids, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<AccelNodeLoadTable>(grids).grids == std::vector<ContactRun>{{101, 101, 1}, {105, 109, 1}});

    expectInvalid("AccelNodeLoad;0;;9.81;1.;;;1;2", "id");
    expectInvalid("AccelNodeLoad;1;-1;9.81;1.;;;1;2", "frameId");
    expectInvalid("AccelNodeLoad;1;;;1.;;;1;2", "magnitude");
    expectInvalid("AccelNodeLoad;1;;9.81;;;;1;2", "directionX");       // the vector is zero
    expectInvalid("AccelNodeLoad;1;;9.81;1.", "nodeId");               // no grid point
    expectInvalid("AccelNodeLoad;1;;9.81;1.;;;5;Through;1", "nodeId"); // a range goes up
    expectInvalid("AccelNodeLoad;1;;9.81;1.;;;1;By;2", "By");
}

// ---------------------------------------------------------------------------------- legacy contact cards of the decks

TEST_CASE("a BCTSET is a contact set of pairs of regions, joined by a BCTADD and given parameters by a BCTPARA",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "ContactSet;101;1;2;0.;0.;2.\n"
        "ContactSet;103;2;1;;;0.25;3;4;;;0.25\n"
        "ContactSet;104;5;6;0.2;-0.1;3.5;7;8;0.4;0.05;1.5\n"
        "ContactSetUnion;100;101;103\n"
        "ContactSurfaceParams;101;TiedContact;1\n"
        "ContactControlParams;103;PENTYP;1;INIPENE;0");
    const ContactSetCard single = cardAt<ContactSetTable>(model, 0);
    REQUIRE(single.id == 101);
    REQUIRE(single.pairs == std::vector<ContactSetPair>{{1, 2, 0.0, 0.0, 2.0}});
    // Two pairs, the second on the next line: the pair starts at field 10, and the blank FRIC and MIND are 0.0.
    REQUIRE(cardAt<ContactSetTable>(model, 1).pairs ==
            std::vector<ContactSetPair>{{2, 1, 0.0, 0.0, 0.25}, {3, 4, 0.0, 0.0, 0.25}});
    REQUIRE(cardAt<ContactSetTable>(model, 2).pairs ==
            std::vector<ContactSetPair>{{5, 6, 0.2, -0.1, 3.5}, {7, 8, 0.4, 0.05, 1.5}});

    // The regions may be BSURFS cards, which are not typed: they are not looked up. The set is a contact table, which a
    // BCTADD, a BCTPARA and a BCTPARM name.
    REQUIRE(referencesOf<ContactSetTable>(model).fields.empty());
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(referencesOf<ContactSetUnionTable>(model).targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ContactTable, ReferenceTarget::ContactTable});
    REQUIRE(referencesOf<ContactSurfaceParamsTable>(model).targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ContactTable});
    Model lacking = readBulk("ContactSetUnion;100;101;999\nContactSet;101;1;2\nContactSurfaceParams;555;TiedContact;1");
    REQUIRE(unresolvedFields<ContactSetUnionTable>(lacking) == std::vector<std::string>{"memberSetId"});
    REQUIRE(unresolvedFields<ContactSurfaceParamsTable>(lacking) == std::vector<std::string>{"contactSetId"});
    requireRoundTrip(model);

    // A renumbering of the contact tables moves the sets, the BCTADD that names them and the parameters of a set.
    Renumbering::apply(model, RenumberSpace::ContactTable, Renumbering::offset(1000));
    REQUIRE((cardAt<ContactSetTable>(model, 0).id == 1101 && cardAt<ContactSetTable>(model, 1).id == 1103));
    REQUIRE(cardAt<ContactSetUnionTable>(model).id == 1100);
    REQUIRE(cardAt<ContactSetUnionTable>(model).sets == std::vector<base::EntityId>{1101, 1103});
    REQUIRE(cardAt<ContactSurfaceParamsTable>(model).contactSetId == 1101);
    REQUIRE(model.table<ContactSurfaceParamsTable>().idAtRow(0) == 1101); // the ID column follows
    REQUIRE(cardAt<ContactControlParamsTable>(model).contactSetId == 1103);
    REQUIRE(unresolvedIn(model) == 0);

    REQUIRE_THROWS_AS(readBulk("ContactSet;101;1;2\nContactSet;101;3;4"), base::DeckError);
    expectInvalid("ContactSet;0;1;2", "id");
    expectInvalid("ContactSet;101", "sourceRegionId"); // no pair
    expectInvalid("ContactSet;101;1", "targetRegionId");
    expectInvalid("ContactSet;101;0;2", "sourceRegionId");
    expectInvalid("ContactSet;101;1;2;-0.5", "friction");
}

TEST_CASE("a BCTPARM takes the known names with their kinds and any other name as a number", "[model][cards][explicit]")
{
    const Model model = readBulk(
        "ContactControlParams;101;PENTYP;1;INIPENE;0\nContactControlParams;102;NormalPenalty;10.;TangentPenalty;0.5;"
        "ContactTolerance;0.001;ShellThicknessOption;1");
    const ContactControlParamsCard twoNames = cardAt<ContactControlParamsTable>(model, 0);
    REQUIRE(twoNames.contactSetId == 101);
    REQUIRE(twoNames.params.size() == 2);
    REQUIRE((twoNames.params[0].name == "PENTYP" && std::get<std::int64_t>(twoNames.params[0].value) == 1));
    REQUIRE((twoNames.params[1].name == "INIPENE" && std::get<std::int64_t>(twoNames.params[1].value) == 0));
    const ContactControlParamsCard known = cardAt<ContactControlParamsTable>(model, 1);
    REQUIRE(known.params.size() == 4);
    REQUIRE((known.params[0].name == "NormalPenalty" && std::get<double>(known.params[0].value) == 10.0));
    // SHLTHK on line 2
    REQUIRE((known.params[3].name == "ShellThicknessOption" && std::get<std::int64_t>(known.params[3].value) == 1));
    requireRoundTrip(model);

    REQUIRE_THROWS_AS(readBulk("ContactControlParams;101;NormalPenalty;1.\nContactControlParams;101;TangentPenalty;1."),
                      base::DeckError); // one card per set
    expectInvalid("ContactControlParams;0;NormalPenalty;1.", "contactSetId");
    expectInvalid("ContactControlParams;101;ShellThicknessOption;1.5", "parameterValue"); // an integer
    expectInvalid("ContactControlParams;101;NormalPenalty", "parameterValue");            // every value is required
    expectInvalid("ContactControlParams;101;PENTYP", "parameterValue");
    expectInvalid("ContactControlParams;101;;1.", "parameterName");
    oor::testing::requireModelFileRoundTrip(readModelText("ContactControlParams;101;TOOLONGNAME;1.", true));
    expectInvalid("ContactControlParams;101;1ABC;1.", "parameterName");
    expectInvalid("ContactControlParams;101;FRICMOD;ABC", "parameterValue"); // an unknown name holds a number
}

TEST_CASE("a BCTPARA checks the values of the known names and keeps the blank ones", "[model][cards][explicit]")
{
    const Model model = readBulk(
        "ContactSurfaceParams;101;TiedContact;1\n"
        "ContactSurfaceParams;102;ContactKind;0;SideCount;2;SegmentNormal;-1;ContactStiffness;1;OffsetDistance;0.025\n"
        "ContactSurfaceParams;103;InitialPenetration;2;PenetrationDepth;0.1;BirthTime;0.;DeathTime;5.;OffsetKind;1;"
        "ZeroPenetrationTime;0.2;TiedTolerance;0.01;ExtensionFactor;0.005;FRICMOD;1;EPSN;100.\n"
        "ContactSurfaceParams;104;TiedContact");
    REQUIRE(cardAt<ContactSurfaceParamsTable>(model, 0).params.size() == 1);
    const ContactSurfaceParamsCard example = cardAt<ContactSurfaceParamsTable>(model, 1);
    REQUIRE(example.params.size() == 5);
    REQUIRE((example.params[2].name == "SegmentNormal" && std::get<std::int64_t>(example.params[2].value) == -1));
    REQUIRE((example.params[4].name == "OffsetDistance" && std::get<double>(example.params[4].value) == 0.025));
    const ContactSurfaceParamsCard every = cardAt<ContactSurfaceParamsTable>(model, 2);
    REQUIRE(every.params.size() == 10);
    REQUIRE(
        (every.params[8].name == "FRICMOD" && std::get<std::int64_t>(every.params[8].value) == 1)); // not a known name
    REQUIRE((every.params[9].name == "EPSN" && std::get<double>(every.params[9].value) == 100.0));
    // A known name may have a blank value, which stays blank.
    const ContactSurfaceParamsCard blank = cardAt<ContactSurfaceParamsTable>(model, 3);
    REQUIRE((blank.params.size() == 1 && std::holds_alternative<std::monostate>(blank.params[0].value)));
    requireRoundTrip(model);

    expectInvalid("ContactSurfaceParams;101;ContactKind;3", "params");
    expectInvalid("ContactSurfaceParams;101;SideCount;0", "params");
    expectInvalid("ContactSurfaceParams;101;InitialPenetration;4", "params");
    expectInvalid("ContactSurfaceParams;101;SegmentNormal;2", "params");
    expectInvalid("ContactSurfaceParams;101;ContactStiffness;2", "params");
    expectInvalid("ContactSurfaceParams;101;TiedContact;2", "params");
    expectInvalid("ContactSurfaceParams;101;ExtensionFactor;0.5", "params");
    expectInvalid("ContactSurfaceParams;101;ExtensionFactor;1.E-9", "params");
    expectInvalid("ContactSurfaceParams;101;ContactKind;1.5", "parameterValue"); // an integer
    expectInvalid("ContactSurfaceParams;101;FRICMOD", "parameterValue");         // an unknown name needs a value
    expectInvalid("ContactSurfaceParams;0;TiedContact;1", "contactSetId");
}
