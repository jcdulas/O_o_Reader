// Checks of the cards of the Euler domain of the explicit solver: MESH, BIAS, PEULER1, PMARKER,
// CMARKB2, CMARKN1, SURFINI, CYLINDR, SPHERE, EULFOR, EULFOR1, EULFREG and HYDSTAT. Per card: the fields in their
// format reference positions, the defaults, the references the card lists, the renumbering of its ID space, the refused
// values and the round trip through a model file.

#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/explicit/cylinderShape/CylinderShapeTable.hpp"
#include "oor/model/cards/explicit/eulerBodyForce/EulerBodyForceTable.hpp"
#include "oor/model/cards/explicit/eulerBodyForceSet/EulerBodyForceSetTable.hpp"
#include "oor/model/cards/explicit/eulerForceRegion/EulerForceRegionTable.hpp"
#include "oor/model/cards/explicit/eulerHydrostaticPreset/EulerHydrostaticPresetTable.hpp"
#include "oor/model/cards/explicit/eulerMeshBias/EulerMeshBiasTable.hpp"
#include "oor/model/cards/explicit/eulerMeshGenerator/EulerMeshGeneratorTable.hpp"
#include "oor/model/cards/explicit/eulerProp/EulerPropTable.hpp"
#include "oor/model/cards/explicit/eulerRegionProp/EulerRegionPropTable.hpp"
#include "oor/model/cards/explicit/initialSurfaceVolume/InitialSurfaceVolumeTable.hpp"
#include "oor/model/cards/explicit/markerNode/MarkerNodeTable.hpp"
#include "oor/model/cards/explicit/markerBody/MarkerBodyTable.hpp"
#include "oor/model/cards/explicit/markerProp/MarkerPropTable.hpp"
#include "oor/model/cards/explicit/sphereShape/SphereShapeTable.hpp"
#include "support/ExplicitTestSupport.hpp"
#include "support/SlotRecord.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The record of a card from the text of each of the slots its card reader reads (slot 1 first).
std::string freeCard(const std::string& name, const std::vector<std::string>& fields)
{
    return slotRecord(name, fields);
}

// The fields of a valid card with one of them replaced (the card grows when the field is beyond its end).
std::vector<std::string> fieldsWith(std::vector<std::string> fields, int field, const std::string& value)
{
    if (fields.size() < static_cast<std::size_t>(field))
        fields.resize(static_cast<std::size_t>(field));
    fields[static_cast<std::size_t>(field - 1)] = value;
    return fields;
}

// The text of a valid card with one of its fields replaced.
std::string withField(const std::string& name, const std::vector<std::string>& fields, int field,
                      const std::string& value)
{
    return freeCard(name, fieldsWith(fields, field, value));
}

// The fields of a MESH from (field number, text) pairs: the card has eight lines of eight fields.
std::vector<std::string> meshFields(const std::vector<std::pair<int, std::string>>& given)
{
    std::vector<std::string> fields(64);
    for (const auto& [field, text] : given)
        fields[static_cast<std::size_t>(field - 1)] = text;
    return fields;
}

// A box with element sizes and numbers of elements, the least a BOX mesh needs.
const std::vector<std::string> BOX_FIELDS = meshFields(
    {{1, "1"}, {2, "Box"}, {3, "0.1"}, {4, "0.1"}, {5, "0.1"}, {17, "10"}, {18, "10"}, {19, "10"}, {24, "5"}});

// An adaptive mesh with element sizes, the least an ADAPT mesh needs.
const std::vector<std::string> ADAPT_FIELDS =
    meshFields({{1, "2"}, {2, "Adaptive"}, {3, "0.1"}, {4, "0.1"}, {5, "0.1"}, {24, "5"}});

// A box with every field of the layout given (a BOX may carry the fields of the adaptive mesh: the card keeps them).
const std::vector<std::string> BOX_ALL_FIELDS =
    meshFields({{1, "4"},    {2, "Box"},   {3, "0.1"},  {4, "0.2"},   {5, "0.3"},   {6, "-0.5"},      {7, "-0.5"},
                {8, "-0.5"}, {9, "-1."},   {10, "-2."}, {11, "-3."},  {12, "4."},   {13, "4."},       {14, "3.9"},
                {17, "40"},  {18, "20"},   {19, "13"},  {20, "21"},   {21, "1000"}, {22, "2000"},     {23, "Euler"},
                {24, "6"},   {25, "None"}, {26, "101"}, {27, "102"},  {28, "103"},  {29, "Material"}, {33, "Center"},
                {34, "3."},  {35, "2.5"},  {36, "2."},  {37, "100"},  {38, "101"},  {39, "102"},      {41, "0."},
                {42, "0."},  {43, "0."},   {44, "8."},  {45, "8."},   {46, "8."},   {49, "12"},       {57, "1000"},
                {58, "8"},   {59, "4"},    {60, "4"},   {61, "User"}, {62, "4"},    {63, "2"},        {64, "2"}});

// The sub mesh of the full box: the least a mesh needs, numbered 21.
const std::vector<std::string> SUB_MESH_FIELDS = meshFields(
    {{1, "21"}, {2, "Box"}, {3, "0.05"}, {4, "0.05"}, {5, "0.05"}, {17, "4"}, {18, "4"}, {19, "4"}, {24, "6"}});

// A TABLED1 of two points.
std::string tableText(int id)
{
    return "TableDynamic1;" + std::to_string(id) + ";;;0.;1.;1.;1.;EndTable\n";
}

// One element, which the BSURF cards of the decks name (a BSURF is a list of elements).
const std::string ELEMENT_TEXT =
    "Node;1\nNode;2;;1.\nNode;3;;1.;1.\nNode;4;;0.;1.\nShellQuad4;1;1;1;2;3;4\nShellProp;1;1;0.1\nIsoMat;1;2.1E+11;;0."
    "3\n";

} // namespace

TEST_CASE("a PEULER1 is a property: its SID names a TICEUL1 and its SID2 an EULFOR1", "[model][cards][explicit]")
{
    Model model = readBulk(
        "EulerRegionProp;100;;300\nEulerRegionProp;101;MultiMaterialHydrodynamic;301;7\nEulerRegionProp;102;FirstOrder;"
        "302\n"
        "EulerBodyForceSet;7;1\nEulerForceRegion;1;1;Sphere;21;25;31;32;33\nSphereShape;21;0.;0.;0.;1.\n"
        "EulerMat;25;1000.;1\nGammaLawStateEquation;1;1.4\nEulerInitialRegions;300;200\nEulerInitialRegions;301;200\n"
        "EulerInitialRegions;302;200\nInitialConditionRegion;1;200;Sphere;21;25\n" +
        tableText(31) + tableText(32) + tableText(33));
    const EulerRegionPropCard plain = cardAt<EulerRegionPropTable>(model, 0);
    REQUIRE((plain.id == 100 && plain.type == EulerSolverType::Hydro && plain.initialRegionsId == 300 &&
             plain.bodyForceSetId == 0));
    const EulerRegionPropCard full = cardAt<EulerRegionPropTable>(model, 1);
    REQUIRE(
        (full.type == EulerSolverType::MultiMaterialHydro && full.initialRegionsId == 301 && full.bodyForceSetId == 7));
    REQUIRE(cardAt<EulerRegionPropTable>(model, 2).type == EulerSolverType::FirstOrder);
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<EulerRegionPropTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"initialRegionsId", "bodyForceSetId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::EulerInitialization, ReferenceTarget::EulerBodyForce});
    visitor = RecordingVisitor{};
    model.table<EulerRegionPropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"initialRegionsId"}); // a blank SID2 names nothing

    // The property space is shared by every property type, PEULER included.
    REQUIRE_THROWS_AS(readBulk("EulerRegionProp;100;;300\nShellProp;100;1;0.1"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("EulerRegionProp;100;;300\nEulerProp;100"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<EulerRegionPropTable>(model, 0).id == 1100 && cardAt<EulerRegionPropTable>(model, 2).id == 1102));
    Renumbering::apply(model, RenumberSpace::EulerBodyForce, Renumbering::offset(10));
    REQUIRE((cardAt<EulerRegionPropTable>(model, 1).bodyForceSetId == 17 &&
             cardAt<EulerBodyForceSetTable>(model, 0).id == 17));
    REQUIRE(cardAt<EulerRegionPropTable>(model, 1).initialRegionsId ==
            301); // the TICEUL1 is not in the space renumbered here
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("EulerRegionProp;0;;300", "id");
    expectInvalid("EulerRegionProp;100;;0", "initialRegionsId");
    expectInvalid("EulerRegionProp;100", "initialRegionsId"); // required
    expectInvalid("EulerRegionProp;100;PLASTIC;300", "solverType");
    expectInvalid("EulerRegionProp;100;;300;-1", "bodyForceSetId");
}

TEST_CASE("a PMARKER is a property and the marker elements are elements, so each shares its ID space",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "MarkerProp;7\n"
        "MarkerProp;8;Moving\n"
        "MarkerBody;50;7;9;10\n"
        "MarkerNode;51;8;9\n"
        "Node;9;;0.;0.;0.\n"
        "Node;10;;1.;0.;0.");
    REQUIRE(
        (cardAt<MarkerPropTable>(model, 0).id == 7 && cardAt<MarkerPropTable>(model, 0).type == MarkerPropType::Fixed));
    REQUIRE(cardAt<MarkerPropTable>(model, 1).type == MarkerPropType::Moving);
    const MarkerBodyCard line = cardAt<MarkerBodyTable>(model, 0);
    REQUIRE((line.id == 50 && line.propId == 7 && line.node1 == 9 && line.node2 == 10));
    REQUIRE((model.findTable<MarkerBodyTable>()->nodes(0) == std::array<base::EntityId, 2>{9, 10}));
    REQUIRE(model.findTable<MarkerBodyTable>()->propId(0) == 7);
    const MarkerNodeCard point = cardAt<MarkerNodeTable>(model, 0);
    REQUIRE((point.id == 51 && point.propId == 8 && point.node1 == 9));
    REQUIRE((model.findTable<MarkerNodeTable>()->nodes(0) == std::array<base::EntityId, 1>{9}));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<MarkerBodyTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "node1", "node2"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node, ReferenceTarget::Node});
    visitor = RecordingVisitor{};
    model.table<MarkerNodeTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "node1"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node});
    visitor = RecordingVisitor{};
    model.table<MarkerPropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());

    // A marker element may not have the ID of a structural element, nor a marker property that of another property.
    REQUIRE_THROWS_AS(readBulk("MarkerBody;50;7;9;10\nRod2;50;1;9;10"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("MarkerBody;50;7;9;10\nMarkerNode;50;7;9"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("MarkerProp;7\nShellProp;7;1;0.1"), base::DeckError);

    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(100));
    REQUIRE((cardAt<MarkerPropTable>(model, 0).id == 107 && cardAt<MarkerBodyTable>(model, 0).propId == 107));
    REQUIRE((cardAt<MarkerPropTable>(model, 1).id == 108 && cardAt<MarkerNodeTable>(model, 0).propId == 108));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(5));
    REQUIRE((cardAt<MarkerBodyTable>(model, 0).node1 == 14 && cardAt<MarkerBodyTable>(model, 0).node2 == 15));
    REQUIRE(cardAt<MarkerNodeTable>(model, 0).node1 == 14);
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE((cardAt<MarkerBodyTable>(model, 0).id == 1050 && cardAt<MarkerNodeTable>(model, 0).id == 1051));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("MarkerProp;0", "id");
    expectInvalid("MarkerProp;7;STATIC", "markerKind");
    expectInvalid("MarkerBody;0;7;9;10", "id");
    expectInvalid("MarkerBody;50;0;9;10", "propId");
    expectInvalid("MarkerBody;50;7;0;10", "node1");
    expectInvalid("MarkerBody;50;7;9;0", "node2");
    expectInvalid("MarkerBody;50;7;9;9", "node2"); // a line between a point and itself
    expectInvalid("MarkerBody;50;7;9", "node2");
    expectInvalid("MarkerNode;0;7;9", "id");
    expectInvalid("MarkerNode;51;0;9", "propId");
    expectInvalid("MarkerNode;51;7;0", "node1");
    expectInvalid("MarkerNode;51;7", "node1");
}

TEST_CASE("a SURFINI, a SPHERE and a CYLINDR are the shapes of an Euler region, in one category",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "InitialSurfaceVolume;100;37\n"
        "InitialSurfaceVolume;101;38;Outside;Off;Off\n"
        "InitialSurfaceVolume;102;39;Inside;On;Off\n"
        "ContactElemSurface;37;1\n"
        "ContactElemSurface;38;1\n"
        "ContactElemSurface;39;1\n"
        "SphereShape;100;;;;0.75\n"
        "SphereShape;101;1.;-2.;3.;0.5\n"
        "CylinderShape;100;0.;0.;0.;0.;0.;2.5;0.5\n"
        "CylinderShape;101;-1.;2.;3.;4.5;-2.;1.;0.25\n" +
        ELEMENT_TEXT);
    const InitialSurfaceVolumeCard surface = cardAt<InitialSurfaceVolumeTable>(model, 0);
    REQUIRE((surface.id == 100 && surface.surfaceId == 37 && surface.cover == CoverSide::Inside));
    REQUIRE((surface.reverse == Switch::On && surface.check == Switch::On)); // the defaults
    const InitialSurfaceVolumeCard outside = cardAt<InitialSurfaceVolumeTable>(model, 1);
    REQUIRE((outside.cover == CoverSide::Outside && outside.reverse == Switch::Off && outside.check == Switch::Off));
    // REVERSE = ON with CHECK = OFF is kept as written: the solver sets CHECK itself.
    REQUIRE((cardAt<InitialSurfaceVolumeTable>(model, 2).reverse == Switch::On &&
             cardAt<InitialSurfaceVolumeTable>(model, 2).check == Switch::Off));
    const SphereShapeCard centred = cardAt<SphereShapeTable>(model, 0);
    REQUIRE((centred.id == 100 && centred.centerX == 0.0 && centred.centerY == 0.0 && centred.centerZ == 0.0 &&
             centred.radius == 0.75));
    const SphereShapeCard moved = cardAt<SphereShapeTable>(model, 1);
    REQUIRE((moved.centerX == 1.0 && moved.centerY == -2.0 && moved.centerZ == 3.0 && moved.radius == 0.5));
    const CylinderShapeCard axis = cardAt<CylinderShapeTable>(model, 0);
    REQUIRE(
        (axis.id == 100 && axis.centerX1 == 0.0 && axis.centerZ1 == 0.0 && axis.centerZ2 == 2.5 && axis.radius == 0.5));
    const CylinderShapeCard oblique = cardAt<CylinderShapeTable>(model, 1);
    REQUIRE((oblique.centerX1 == -1.0 && oblique.centerY1 == 2.0 && oblique.centerZ1 == 3.0));
    REQUIRE((oblique.centerX2 == 4.5 && oblique.centerY2 == -2.0 && oblique.centerZ2 == 1.0 && oblique.radius == 0.25));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<InitialSurfaceVolumeTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"surfaceId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface});
    visitor = RecordingVisitor{};
    model.table<SphereShapeTable>().visitReferences(0, visitor);
    model.table<CylinderShapeTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty()); // the two geometric shapes name no card

    // The VID is unique per card type: a SURFINI, a SPHERE and a CYLINDR may share a number (a check of the whole
    // model looks at the category), but two of one type may not.
    REQUIRE_NOTHROW(
        readBulk("InitialSurfaceVolume;20;1\nSphereShape;20;0.;0.;0.;1.\nCylinderShape;20;0.;0.;0.;0.;0.;1.;0.5"));
    REQUIRE_THROWS_AS(readBulk("SphereShape;20;0.;0.;0.;1.\nSphereShape;20;1.;1.;1.;1."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("InitialSurfaceVolume;20;1\nInitialSurfaceVolume;20;2"), base::DeckError);

    Renumbering::apply(model, RenumberSpace::EulerRegionShape, Renumbering::offset(500));
    REQUIRE((cardAt<InitialSurfaceVolumeTable>(model, 1).id == 601 && cardAt<SphereShapeTable>(model, 0).id == 600));
    REQUIRE(cardAt<CylinderShapeTable>(model, 1).id == 601);
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(1000));
    REQUIRE((cardAt<InitialSurfaceVolumeTable>(model, 0).surfaceId == 1037 &&
             cardAt<InitialSurfaceVolumeTable>(model, 2).surfaceId == 1039));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("InitialSurfaceVolume;0;37", "id");
    expectInvalid("InitialSurfaceVolume;100;0", "surfaceId");
    expectInvalid("InitialSurfaceVolume;100", "surfaceId");     // required
    expectInvalid("InitialSurfaceVolume;100;37;None", "cover"); // a word of the COVER of a COUPLE, not of a SURFINI
    expectInvalid("InitialSurfaceVolume;100;37;AROUND", "cover");
    expectInvalid("InitialSurfaceVolume;100;37;;MAYBE", "reverseNormals");
    expectInvalid("InitialSurfaceVolume;100;37;;;MAYBE", "checkNormals");

    expectInvalid("SphereShape;0;;;;1.", "id");
    expectInvalid("SphereShape;100;;;;0.", "radius");
    expectInvalid("SphereShape;100;;;;-1.", "radius");
    expectInvalid("SphereShape;100", "radius"); // required
    expectInvalid("SphereShape;100;X;;;1.", "centerX");

    expectInvalid("CylinderShape;0;0.;0.;0.;1.;1.;1.;0.5", "id");
    expectInvalid("CylinderShape;4;0.;0.;0.;1.;1.;1.;0.", "radius");
    expectInvalid("CylinderShape;4;0.;0.;0.;1.;1.;1.;-0.5", "radius");
    expectInvalid("CylinderShape;4;0.;0.;0.;1.;1.", "centerZ2"); // every coordinate is required
    expectInvalid("CylinderShape;4;0.;0.;0.;1.;1.;1.", "radius");
    expectInvalid("CylinderShape;4;1.;2.;3.;1.;2.;3.;0.5", "centerX2"); // no axis
}

TEST_CASE("an EULFOR is a direction scaled by a number or a table, in the category of the body forces",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "EulerBodyForce;100;5;;13;1.;0.;0.\nEulerBodyForce;101;;9.81;;;;-1.\nEulerBodyForce;102;;2.;;0.1;0.2;0.3\n"
        "RectFrame2;5\n" +
        tableText(13));
    const EulerBodyForceCard table = cardAt<EulerBodyForceTable>(model, 0);
    REQUIRE((table.id == 100 && table.frameId == 5 && table.scaleFactor == 0.0 && table.tableId == 13));
    REQUIRE((table.directionX == 1.0 && table.directionY == 0.0 && table.directionZ == 0.0));
    const EulerBodyForceCard scaled = cardAt<EulerBodyForceTable>(model, 1);
    REQUIRE((scaled.frameId == 0 && scaled.scaleFactor == 9.81 && scaled.tableId == 0 && scaled.directionZ == -1.0));
    const EulerBodyForceCard every = cardAt<EulerBodyForceTable>(model, 2);
    REQUIRE(
        (every.scaleFactor == 2.0 && every.directionX == 0.1 && every.directionY == 0.2 && every.directionZ == 0.3));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<EulerBodyForceTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"frameId", "tableId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Coordinate, ReferenceTarget::DynamicTable});
    visitor = RecordingVisitor{};
    model.table<EulerBodyForceTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields.empty()); // the basic system and no table

    REQUIRE_THROWS_AS(readBulk("EulerBodyForce;100;;1.;;1.\nEulerBodyForce;100;;1.;;1."), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<EulerBodyForceTable>(model, 0).frameId == 15);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE(cardAt<EulerBodyForceTable>(model, 0).tableId == 113);
    Renumbering::apply(model, RenumberSpace::EulerBodyForce, Renumbering::offset(1000));
    REQUIRE((cardAt<EulerBodyForceTable>(model, 0).id == 1100 && cardAt<EulerBodyForceTable>(model, 2).id == 1102));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("EulerBodyForce;0;;1.;;1.", "id");
    expectInvalid("EulerBodyForce;100;-1;1.;;1.", "frameId");
    expectInvalid("EulerBodyForce;100;;-1.;;1.", "scaleFactor");
    expectInvalid("EulerBodyForce;100;;ONE;;1.", "scaleFactor");
    expectInvalid("EulerBodyForce;100;;1.;-3;1.", "tableId");
    expectInvalid("EulerBodyForce;100;;;;1.", "scaleFactor"); // neither the number nor the table
    expectInvalid("EulerBodyForce;100;;1.", "directionX");    // no direction
    expectInvalid("EulerBodyForce;100;;1.;;0.;0.;0.", "directionX");
}

TEST_CASE("an EULFOR1 names the set of regions of an EULFREG and an EULFREG names a shape of the type it says",
          "[model][cards][explicit]")
{
    const std::string tables = tableText(31) + tableText(32) + tableText(33);
    Model model = readBulk(
        "EulerRegionProp;100;;300;7\n"
        "EulerBodyForceSet;7;1\n"
        "EulerBodyForce;7;;1.;;1.\n"
        "EulerForceRegion;10;1;Surface;20;25;31;32;33\n"
        "EulerForceRegion;11;1;Sphere;21;25;31;32;33;2.5\n"
        "EulerForceRegion;12;1;Cylinder;22;25;31;32;33;-1.\n"
        "EulerForceRegion;13;1;Box;23;25;31;32;33\n"
        "InitialSurfaceVolume;20;60\n"
        "SphereShape;21;0.;0.;0.;1.\n"
        "CylinderShape;22;0.;0.;0.;0.;0.;1.;0.5\n"
        "ContactElemSurface;23;1\n"
        "ContactElemSurface;60;1\n"
        "EulerMat;25;1.E+3;1\n"
        "GammaLawStateEquation;1;1.4\n"
        "EulerInitialRegions;300;200\n"
        "InitialConditionRegion;1;200;Sphere;21;25\n" +
        ELEMENT_TEXT + tables);
    const EulerBodyForceSetCard force = cardAt<EulerBodyForceSetTable>(model, 0);
    REQUIRE((force.id == 7 && force.regionSetId == 1));
    // One body force number is shared by the two cards (the category EulerBodyForce): the cards of one type are unique.
    REQUIRE(model.table<EulerBodyForceSetTable>().idAtRow(0) == model.table<EulerBodyForceTable>().idAtRow(0));

    const EulerForceRegionCard surface = cardAt<EulerForceRegionTable>(model, 0);
    REQUIRE(
        (surface.id == 10 && surface.setId == 1 && surface.type == EulerRegionType::Surface && surface.shapeId == 20));
    REQUIRE((surface.matId == 25 && surface.accelX == 31 && surface.accelY == 32 && surface.accelZ == 33));
    REQUIRE(surface.level == 0.0); // the default
    REQUIRE((cardAt<EulerForceRegionTable>(model, 1).type == EulerRegionType::Sphere &&
             cardAt<EulerForceRegionTable>(model, 1).level == 2.5));
    REQUIRE((cardAt<EulerForceRegionTable>(model, 2).type == EulerRegionType::Cylinder &&
             cardAt<EulerForceRegionTable>(model, 2).level == -1.0));
    REQUIRE(cardAt<EulerForceRegionTable>(model, 3).type == EulerRegionType::Box); // the word in any case
    // The cards of a set share the ESID, which is the ID of the table.
    REQUIRE(model.table<EulerForceRegionTable>().idAtRow(0) == model.table<EulerForceRegionTable>().idAtRow(3));
    REQUIRE(unresolvedIn(model) == 0);

    // What the VID names depends on the TYPE of the card.
    RecordingVisitor visitor;
    model.table<EulerForceRegionTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"shapeId", "matId", "accelX", "accelY", "accelZ"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::EulerRegionShape,
                                                            ReferenceTarget::Material, ReferenceTarget::DynamicTable,
                                                            ReferenceTarget::DynamicTable,
                                                            ReferenceTarget::DynamicTable});
    visitor = RecordingVisitor{};
    model.table<EulerForceRegionTable>().visitReferences(3, visitor);
    REQUIRE(visitor.targets.front() == ReferenceTarget::ContactSurface); // a BCBOX
    Model elements = readBulk("EulerForceRegion;14;2;Elements;24;25;31;32;33");
    visitor = RecordingVisitor{};
    elements.table<EulerForceRegionTable>().visitReferences(0, visitor);
    REQUIRE(visitor.targets.front() == ReferenceTarget::Set); // a SET1

    RecordingVisitor forceVisitor;
    model.table<EulerBodyForceSetTable>().visitReferences(0, forceVisitor);
    REQUIRE(forceVisitor.fields == std::vector<std::string>{"regionSetId"});
    REQUIRE(forceVisitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::EulerRegionSet});

    // The shapes of the category are renumbered with the VID of the regions that name them, and only those.
    Renumbering::apply(model, RenumberSpace::EulerRegionShape, Renumbering::offset(100));
    REQUIRE((cardAt<InitialSurfaceVolumeTable>(model, 0).id == 120 && cardAt<SphereShapeTable>(model, 0).id == 121));
    REQUIRE(cardAt<CylinderShapeTable>(model, 0).id == 122);
    REQUIRE((cardAt<EulerForceRegionTable>(model, 0).shapeId == 120 &&
             cardAt<EulerForceRegionTable>(model, 1).shapeId == 121));
    REQUIRE((cardAt<EulerForceRegionTable>(model, 2).shapeId == 122 &&
             cardAt<EulerForceRegionTable>(model, 3).shapeId == 23));
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(1000));
    REQUIRE((cardAt<EulerForceRegionTable>(model, 3).shapeId == 1023 &&
             cardAt<InitialSurfaceVolumeTable>(model, 0).surfaceId == 1060));
    // The set of the regions is renumbered with the EULFOR1 that names it.
    Renumbering::apply(model, RenumberSpace::EulerRegionSet, Renumbering::offset(50));
    REQUIRE(
        (cardAt<EulerForceRegionTable>(model, 0).setId == 51 && cardAt<EulerForceRegionTable>(model, 3).setId == 51));
    REQUIRE(cardAt<EulerBodyForceSetTable>(model, 0).regionSetId == 51);
    Renumbering::apply(model, RenumberSpace::EulerBodyForce, Renumbering::offset(10));
    REQUIRE((cardAt<EulerBodyForceSetTable>(model, 0).id == 17 && cardAt<EulerBodyForceTable>(model, 0).id == 17));
    REQUIRE(cardAt<EulerRegionPropTable>(model, 0).bodyForceSetId == 17);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE((cardAt<EulerForceRegionTable>(model, 0).accelX == 131 &&
             cardAt<EulerForceRegionTable>(model, 0).accelZ == 133));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(5));
    REQUIRE(cardAt<EulerForceRegionTable>(model, 2).matId == 30);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("EulerBodyForceSet;0;1", "id");
    expectInvalid("EulerBodyForceSet;7;0", "regionSetId");
    expectInvalid("EulerBodyForceSet;7", "regionSetId");

    expectInvalid("EulerForceRegion;0;1;Box;4;5;6;7;8", "id");
    expectInvalid("EulerForceRegion;1;0;Box;4;5;6;7;8", "setId");
    expectInvalid("EulerForceRegion;1;1;;4;5;6;7;8", "shapeType"); // required
    expectInvalid("EulerForceRegion;1;1;PLANE;4;5;6;7;8", "shapeType");
    expectInvalid("EulerForceRegion;1;1;Box;0;5;6;7;8", "shapeId");
    expectInvalid("EulerForceRegion;1;1;Box;4;0;6;7;8", "matId");
    expectInvalid("EulerForceRegion;1;1;Box;4;5;0;7;8", "accelX");
    expectInvalid("EulerForceRegion;1;1;Box;4;5;6;0;8", "accelY");
    expectInvalid("EulerForceRegion;1;1;Box;4;5;6;7;0", "accelZ");
    expectInvalid("EulerForceRegion;1;1;Box;4;5;6;7", "accelZ"); // the three tables are required
    expectInvalid("EulerForceRegion;1;1;Box;4;5;6;7;8;ABC", "level");
    // A second region after the first is not in the documented layout.
}

TEST_CASE("a HYDSTAT places the free surface by a grid point or by coordinates", "[model][cards][explicit]")
{
    Model model = readBulk(
        "EulerHydrostaticPreset;101;4;;;0.;0.;2.5;101325.\n"
        "EulerHydrostaticPreset;102;4;35;;;;;101325.\n"
        "EulerHydrostaticPreset;103;5;;3;1.;2.;3.;0.\n"
        "EulerHydrostaticPreset;104;5;;;;;1.5;1.E+5\n"
        "EulerMat;4;1.E+3;1\n"
        "EulerMat;5;1.E+3;1\n"
        "GammaLawStateEquation;1;1.4\n"
        "Node;35;;0.;0.;3.\n"
        "RectFrame2;3");
    const EulerHydrostaticPresetCard coordinates = cardAt<EulerHydrostaticPresetTable>(model, 0);
    REQUIRE((coordinates.id == 101 && coordinates.matId == 4 && coordinates.nodeId == 0 && coordinates.frameId == 0));
    REQUIRE((coordinates.surfacePointX == 0.0 && coordinates.surfacePointY == 0.0 && coordinates.surfacePointZ == 2.5 &&
             coordinates.surfacePressure == 101325.0));
    const EulerHydrostaticPresetCard grid = cardAt<EulerHydrostaticPresetTable>(model, 1);
    REQUIRE((grid.nodeId == 35 && !grid.surfacePointX && !grid.surfacePointY && !grid.surfacePointZ &&
             grid.surfacePressure == 101325.0));
    const EulerHydrostaticPresetCard system = cardAt<EulerHydrostaticPresetTable>(model, 2);
    REQUIRE((system.frameId == 3 && system.surfacePointX == 1.0 && system.surfacePointY == 2.0 &&
             system.surfacePointZ == 3.0 && system.surfacePressure == 0.0));
    // With gravity along z only the z coordinate counts: the others may stay blank, and stay blank.
    const EulerHydrostaticPresetCard vertical = cardAt<EulerHydrostaticPresetTable>(model, 3);
    REQUIRE((!vertical.surfacePointX && !vertical.surfacePointY && vertical.surfacePointZ == 1.5 &&
             vertical.surfacePressure == 1.0E5));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<EulerHydrostaticPresetTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId", "nodeId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::Node});
    visitor = RecordingVisitor{};
    model.table<EulerHydrostaticPresetTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId", "frameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::Coordinate});

    Renumbering::apply(model, RenumberSpace::HydrostaticPreset, Renumbering::offset(100));
    REQUIRE((cardAt<EulerHydrostaticPresetTable>(model, 0).id == 201 &&
             cardAt<EulerHydrostaticPresetTable>(model, 3).id == 204));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE(cardAt<EulerHydrostaticPresetTable>(model, 1).nodeId == 45);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(1));
    REQUIRE(cardAt<EulerHydrostaticPresetTable>(model, 2).frameId == 4);
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(50));
    REQUIRE((cardAt<EulerHydrostaticPresetTable>(model, 0).matId == 54 &&
             cardAt<EulerHydrostaticPresetTable>(model, 2).matId == 55));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("EulerHydrostaticPreset;0;4;;;0.;0.;0.;1.", "id");
    expectInvalid("EulerHydrostaticPreset;101;0;;;0.;0.;0.;1.", "matId");
    expectInvalid("EulerHydrostaticPreset;101;4;-1;;0.;0.;0.;1.", "nodeId");
    expectInvalid("EulerHydrostaticPreset;101;4;;-1;0.;0.;0.;1.", "frameId");
    expectInvalid("EulerHydrostaticPreset;101;4;;;X;0.;0.;1.", "surfacePointX");
    expectInvalid("EulerHydrostaticPreset;101;4;;;0.;0.;0.", "surfacePressure"); // required
    // The two ways of placing the free surface exclude each other, and one is needed.
    expectInvalid("EulerHydrostaticPreset;101;4;35;;0.;0.;0.;1.", "surfacePointX");
    expectInvalid("EulerHydrostaticPreset;101;4;35;;;2.;;1.", "surfacePointY");
    expectInvalid("EulerHydrostaticPreset;101;4;35;;;;3.;1.", "surfacePointZ");
    expectInvalid("EulerHydrostaticPreset;101;4;35;3;;;;1.", "frameId");
    expectInvalid("EulerHydrostaticPreset;101;4;;;;;;1.", "nodeId");
    expectInvalid("EulerHydrostaticPreset;101;4;;3;;;;1.", "nodeId");
}

TEST_CASE("a BIAS is a list of intervals, one per line, each with one of the seven ways to give the spacing",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "EulerMeshBias;10;-2.;0.5;12;;;0.;1.;8;;;1.5;;;0.05;0.3\n"
        "EulerMeshBias;11;0.;0.5;12;;;1.;;;0.1;0.4;2.;;8;0.1;;3.;;8;;0.3;4.;2.;;0.1;;5.;2.;;;0.3;6.;;9\n"
        "EulerMeshBias;12;;;10\n"
        "EulerMeshBias;13;-2.;0.5;12;;;;;;;;0.;1.;8");
    const EulerMeshBiasCard three = cardAt<EulerMeshBiasTable>(model, 0);
    REQUIRE((three.id == 10 && three.intervals.size() == 3));
    REQUIRE((three.intervals[0] == EulerMeshBiasInterval{-2.0, 0.5, 12, std::nullopt, std::nullopt}));
    REQUIRE((three.intervals[1] == EulerMeshBiasInterval{0.0, 1.0, 8, std::nullopt, std::nullopt}));
    REQUIRE((three.intervals[2] == EulerMeshBiasInterval{1.5, std::nullopt, std::nullopt, 0.05, 0.3}));

    // The seven ways, in the order of the format reference.
    const EulerMeshBiasCard methods = cardAt<EulerMeshBiasTable>(model, 1);
    REQUIRE(methods.intervals.size() == 7);
    REQUIRE((methods.intervals[0] == EulerMeshBiasInterval{0.0, 0.5, 12, std::nullopt, std::nullopt}));  // GROWTH and N
    REQUIRE((methods.intervals[1] == EulerMeshBiasInterval{1.0, std::nullopt, std::nullopt, 0.1, 0.4})); // DXS and DXE
    REQUIRE((methods.intervals[2] == EulerMeshBiasInterval{2.0, std::nullopt, 8, 0.1, std::nullopt}));   // DXS and N
    REQUIRE((methods.intervals[3] == EulerMeshBiasInterval{3.0, std::nullopt, 8, std::nullopt, 0.3}));   // DXE and N
    REQUIRE(
        (methods.intervals[4] == EulerMeshBiasInterval{4.0, 2.0, std::nullopt, 0.1, std::nullopt})); // GROWTH and DXS
    REQUIRE(
        (methods.intervals[5] == EulerMeshBiasInterval{5.0, 2.0, std::nullopt, std::nullopt, 0.3})); // GROWTH and DXE
    REQUIRE(
        (methods.intervals[6] == EulerMeshBiasInterval{6.0, std::nullopt, 9, std::nullopt, std::nullopt})); // N alone

    // The begin coordinate of an interval may be blank (the first one is the origin of the mesh).
    const EulerMeshBiasCard blank = cardAt<EulerMeshBiasTable>(model, 2);
    REQUIRE((blank.intervals.size() == 1 && !blank.intervals[0].begin && blank.intervals[0].count == 10));
    // A line with nothing on it holds no interval.
    REQUIRE(cardAt<EulerMeshBiasTable>(model, 3).intervals.size() == 2);

    RecordingVisitor visitor;
    model.table<EulerMeshBiasTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());

    REQUIRE_THROWS_AS(readBulk("EulerMeshBias;10;0.;;5\nEulerMeshBias;10;0.;;5"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::MeshBias, Renumbering::offset(100));
    REQUIRE((cardAt<EulerMeshBiasTable>(model, 0).id == 110 && cardAt<EulerMeshBiasTable>(model, 2).id == 112));
    requireRoundTrip(model);

    expectInvalid("EulerMeshBias;0;0.;1.;5", "id");
    expectInvalid("EulerMeshBias;10", "intervalStart"); // a bias with no interval defines nothing
    expectInvalid("EulerMeshBias;10;X;1.;5", "intervalStart");
    expectInvalid("EulerMeshBias;10;0.;1.", "intervalElemCount"); // GROWTH alone is not a way
    expectInvalid("EulerMeshBias;10;0.;;;0.1", "intervalElemCount");
    expectInvalid("EulerMeshBias;10;0.;;;;0.3", "intervalElemCount");
    expectInvalid("EulerMeshBias;10;0.;1.;5;0.1", "intervalStartSize"); // three of the four
    expectInvalid("EulerMeshBias;10;0.;1.;5;0.1;0.2", "intervalStartSize");
    expectInvalid("EulerMeshBias;10;0.;-1.;5", "intervalGrowth");
    expectInvalid("EulerMeshBias;10;0.;0.;5", "intervalGrowth");
    expectInvalid("EulerMeshBias;10;0.;;0", "intervalElemCount");
    expectInvalid("EulerMeshBias;10;0.;;5.5", "intervalElemCount");
    expectInvalid("EulerMeshBias;10;0.;;;-0.1;0.2", "intervalStartSize");
    expectInvalid("EulerMeshBias;10;0.;;;0.1;0.", "intervalEndSize");
    expectInvalid("EulerMeshBias;10;1.;;5;;;0.;;5", "intervalStart"); // the begin coordinates increase
    expectInvalid("EulerMeshBias;10;1.;;5;;;1.;;5", "intervalStart");
}

TEST_CASE("a MESH has its fields over eight lines in the order of the format reference", "[model][cards][explicit]")
{
    Model model = readBulk(freeCard("EulerMeshGenerator", BOX_ALL_FIELDS) + "\n" +
                           freeCard("EulerMeshGenerator", SUB_MESH_FIELDS));
    const EulerMeshGeneratorCard card = cardAt<EulerMeshGeneratorTable>(model, 0);
    REQUIRE((card.id == 4 && card.type == EulerMeshGeneratorType::Box));
    REQUIRE((card.elemSizeX == 0.1 && card.elemSizeY == 0.2 && card.elemSizeZ == 0.3));
    REQUIRE((card.xReference == -0.5 && card.yReference == -0.5 && card.zReference == -0.5));
    REQUIRE((card.originX == -1.0 && card.originY == -2.0 && card.originZ == -3.0 && card.widthX == 4.0 &&
             card.widthY == 4.0 && card.widthZ == 3.9));
    REQUIRE((card.elemCountX == 40 && card.elemCountY == 20 && card.elemCountZ == 13));
    REQUIRE((card.subMeshId == 21 && card.firstNodeId == 1000 && card.firstElemId == 2000));
    REQUIRE((card.propKind == EulerMeshGeneratorProperty::Euler && card.propId == 6));
    REQUIRE((card.resize == EulerMeshGeneratorResize::None && card.resizeTableIdX == 101 &&
             card.resizeTableIdY == 102 && card.resizeTableIdZ == 103));
    REQUIRE(card.method == EulerMeshGeneratorRemesh::Material);
    REQUIRE((card.bias == EulerMeshGeneratorBiasStart::Center && card.growthX == 3.0 && card.growthY == 2.5 &&
             card.growthZ == 2.0));
    REQUIRE((card.biasIdX == 100 && card.biasIdY == 101 && card.biasIdZ == 102));
    REQUIRE((card.boxOriginX == 0.0 && card.boxOriginY == 0.0 && card.boxOriginZ == 0.0 && card.boxWidthX == 8.0 &&
             card.boxWidthY == 8.0));
    REQUIRE((card.boxWidthZ == 8.0 && card.frameId == 12));
    REQUIRE((card.elemsPerCube == 1000 && card.cubeCountX == 8 && card.cubeCountY == 4 && card.cubeCountZ == 4));
    REQUIRE((card.partitionDirection == EulerMeshGeneratorPartition::User && card.partitionCountX == 4 &&
             card.partitionCountY == 2 && card.partitionCountZ == 2));

    // The defaults of a card that gives the least: the reference point is -1.0e-6 and is the origin of a BOX.
    Model least = readBulk(freeCard("EulerMeshGenerator", BOX_FIELDS));
    const EulerMeshGeneratorCard box = cardAt<EulerMeshGeneratorTable>(least);
    REQUIRE((box.xReference == EULER_MESH_DEFAULT_REFERENCE && box.yReference == EULER_MESH_DEFAULT_REFERENCE &&
             box.zReference == EULER_MESH_DEFAULT_REFERENCE));
    REQUIRE((box.originX == EULER_MESH_DEFAULT_REFERENCE && box.originY == EULER_MESH_DEFAULT_REFERENCE &&
             box.originZ == EULER_MESH_DEFAULT_REFERENCE));
    REQUIRE((!box.widthX && !box.widthY && !box.widthZ && box.subMeshId == 0 && box.firstNodeId == 0 &&
             box.firstElemId == 0));
    REQUIRE((box.propKind == EulerMeshGeneratorProperty::Euler && box.resize == EulerMeshGeneratorResize::None &&
             box.method == EulerMeshGeneratorRemesh::All));
    REQUIRE(
        (box.resizeTableIdX == 0 && box.resizeTableIdY == 0 && box.resizeTableIdZ == 0 && !box.bias && !box.growthX));
    REQUIRE((box.biasIdX == 0 && !box.boxOriginX && box.frameId == 0 && box.elemsPerCube == 1));
    REQUIRE((!box.cubeCountX && !box.cubeCountY && !box.cubeCountZ &&
             box.partitionDirection == EulerMeshGeneratorPartition::X && !box.partitionCountX && !box.partitionCountY &&
             !box.partitionCountZ));
    requireRoundTrip(model);
    requireRoundTrip(least);
}

TEST_CASE("the fields of a MESH whose default is another field are stored resolved", "[model][cards][explicit]")
{
    // The origin of a BOX follows the reference point that was given, field by field.
    const Model moved = readBulk(withField("EulerMeshGenerator", BOX_FIELDS, 6, "2.5"));
    REQUIRE((cardAt<EulerMeshGeneratorTable>(moved).originX == 2.5 &&
             cardAt<EulerMeshGeneratorTable>(moved).originY == EULER_MESH_DEFAULT_REFERENCE));
    // An adaptive mesh does not use X0, Y0 and Z0: they stay blank.
    const EulerMeshGeneratorCard adapt =
        cardAt<EulerMeshGeneratorTable>(readBulk(freeCard("EulerMeshGenerator", ADAPT_FIELDS)));
    REQUIRE((!adapt.originX && !adapt.originY && !adapt.originZ));
    // TID-Y and TID-Z follow TID-X when they are blank, and keep their own value when given.
    const std::vector<std::string> resized = fieldsWith(fieldsWith(ADAPT_FIELDS, 25, "Scale"), 26, "101");
    const Model tables = readBulk(freeCard("EulerMeshGenerator", resized));
    REQUIRE((cardAt<EulerMeshGeneratorTable>(tables).resize == EulerMeshGeneratorResize::Scale &&
             cardAt<EulerMeshGeneratorTable>(tables).resizeTableIdX == 101));
    REQUIRE((cardAt<EulerMeshGeneratorTable>(tables).resizeTableIdY == 101 &&
             cardAt<EulerMeshGeneratorTable>(tables).resizeTableIdZ == 101));
    const EulerMeshGeneratorCard own =
        cardAt<EulerMeshGeneratorTable>(readBulk(withField("EulerMeshGenerator", resized, 27, "102")));
    REQUIRE((own.resizeTableIdY == 102 && own.resizeTableIdZ == 101));
    // A table of the second direction alone is not resolved from anything.
    REQUIRE(cardAt<EulerMeshGeneratorTable>(readBulk(withField("EulerMeshGenerator", ADAPT_FIELDS, 27, "102")))
                .resizeTableIdX == 0);
    // NBY and NBZ follow NBX, NPY and NPZ follow NPX.
    const Model cubes = readBulk(withField("EulerMeshGenerator", BOX_FIELDS, 58, "4"));
    REQUIRE((cardAt<EulerMeshGeneratorTable>(cubes).cubeCountX == 4 &&
             cardAt<EulerMeshGeneratorTable>(cubes).cubeCountY == 4 &&
             cardAt<EulerMeshGeneratorTable>(cubes).cubeCountZ == 4));
    REQUIRE(cardAt<EulerMeshGeneratorTable>(
                readBulk(withField("EulerMeshGenerator", fieldsWith(BOX_FIELDS, 58, "4"), 59, "2")))
                .cubeCountZ == 4);
    const std::vector<std::string> partitioned =
        fieldsWith(fieldsWith(fieldsWith(BOX_FIELDS, 58, "8"), 61, "User"), 62, "4");
    const EulerMeshGeneratorCard user =
        cardAt<EulerMeshGeneratorTable>(readBulk(freeCard("EulerMeshGenerator", partitioned)));
    REQUIRE((user.partitionDirection == EulerMeshGeneratorPartition::User && user.partitionCountX == 4 &&
             user.partitionCountY == 4 && user.partitionCountZ == 4));
    REQUIRE((user.cubeCountX == 8 && user.cubeCountY == 8 && user.cubeCountZ == 8));
    // Without NBX the cubes are the business of NELCUBE: NBY and NBZ stay blank too.
    REQUIRE((!cardAt<EulerMeshGeneratorTable>(readBulk(withField("EulerMeshGenerator", BOX_FIELDS, 57, "2000")))
                  .cubeCountX));
    requireRoundTrip(tables);
    requireRoundTrip(cubes);
    requireRoundTrip(readBulk(freeCard("EulerMeshGenerator", partitioned)));
}

TEST_CASE("a MESH is a box or an adaptive mesh, each with its way to give the sizes", "[model][cards][explicit]")
{
    // A BOX with widths and numbers of elements, an ADAPT with element sizes, with widths and counts, resized by a
    // table.
    const std::vector<std::string> boxWidths = meshFields(
        {{1, "1"}, {2, "Box"}, {12, "2."}, {13, "1."}, {14, "1."}, {17, "20"}, {18, "10"}, {19, "10"}, {24, "5"}});
    const std::vector<std::string> adaptWidths = meshFields(
        {{1, "2"}, {2, "Adaptive"}, {12, "1."}, {13, "1."}, {14, "1."}, {17, "20"}, {18, "20"}, {19, "20"}, {24, "5"}});
    const std::vector<std::string> outputBox = meshFields({{1, "3"},
                                                           {2, "Adaptive"},
                                                           {3, "0.1"},
                                                           {4, "0.1"},
                                                           {5, "0.1"},
                                                           {24, "5"},
                                                           {41, "-1."},
                                                           {42, "-1."},
                                                           {43, "-1."},
                                                           {44, "10."},
                                                           {45, "10."},
                                                           {46, "10."}});
    const Model model =
        readBulk(freeCard("EulerMeshGenerator", boxWidths) + "\n" + freeCard("EulerMeshGenerator", adaptWidths) + "\n" +
                 freeCard("EulerMeshGenerator", outputBox));
    const EulerMeshGeneratorCard box = cardAt<EulerMeshGeneratorTable>(model, 0);
    REQUIRE((box.type == EulerMeshGeneratorType::Box && !box.elemSizeX && box.widthX == 2.0 && box.widthY == 1.0 &&
             box.elemCountX == 20 && box.elemCountZ == 10));
    const EulerMeshGeneratorCard adapt = cardAt<EulerMeshGeneratorTable>(model, 1);
    REQUIRE((adapt.type == EulerMeshGeneratorType::Adapt && !adapt.elemSizeX && adapt.widthZ == 1.0 &&
             adapt.elemCountZ == 20));
    const EulerMeshGeneratorCard output = cardAt<EulerMeshGeneratorTable>(model, 2);
    REQUIRE((output.boxOriginX == -1.0 && output.boxOriginZ == -1.0 && output.boxWidthX == 10.0 &&
             output.boxWidthZ == 10.0));
    requireRoundTrip(model);

    // Sizes, widths and counts may be given together: the card keeps them as written and computes nothing.
    const EulerMeshGeneratorCard both =
        cardAt<EulerMeshGeneratorTable>(readBulk(freeCard("EulerMeshGenerator", BOX_ALL_FIELDS)));
    REQUIRE((both.elemSizeX == 0.1 && both.widthX == 4.0 && both.elemCountX == 40));
    REQUIRE_THROWS_AS(
        readBulk(freeCard("EulerMeshGenerator", BOX_FIELDS) + "\n" + freeCard("EulerMeshGenerator", BOX_FIELDS)),
        base::DeckError);
}

TEST_CASE("a MESH names a property, a sub mesh, tables, bias cards and a coordinate system", "[model][cards][explicit]")
{
    Model model = readBulk(freeCard("EulerMeshGenerator", BOX_ALL_FIELDS) + "\n" +
                           freeCard("EulerMeshGenerator", SUB_MESH_FIELDS) +
                           "EulerProp;6\nEulerMeshBias;100;0.;;5\nEulerMeshBias;101;0.;;5\nEulerMeshBias;102;0.;;5\n" +
                           tableText(101) + tableText(102) + tableText(103) + "RectFrame2;12");
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<EulerMeshGeneratorTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"subMeshId", "propId", "resizeTableIdX", "resizeTableIdY",
                                                       "resizeTableIdZ", "biasIdX", "biasIdY", "biasIdZ", "frameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{
                                   ReferenceTarget::EulerMesh, ReferenceTarget::Property, ReferenceTarget::DynamicTable,
                                   ReferenceTarget::DynamicTable, ReferenceTarget::DynamicTable,
                                   ReferenceTarget::MeshBias, ReferenceTarget::MeshBias, ReferenceTarget::MeshBias,
                                   ReferenceTarget::Coordinate});
    visitor = RecordingVisitor{};
    model.table<EulerMeshGeneratorTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId"}); // everything else is blank

    Renumbering::apply(model, RenumberSpace::EulerMesh, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<EulerMeshGeneratorTable>(model, 0).id == 1004 && cardAt<EulerMeshGeneratorTable>(model, 1).id == 1021));
    REQUIRE(cardAt<EulerMeshGeneratorTable>(model, 0).subMeshId == 1021);
    Renumbering::apply(model, RenumberSpace::MeshBias, Renumbering::offset(5));
    REQUIRE((cardAt<EulerMeshGeneratorTable>(model, 0).biasIdX == 105 &&
             cardAt<EulerMeshGeneratorTable>(model, 0).biasIdZ == 107));
    REQUIRE(cardAt<EulerMeshBiasTable>(model, 0).id == 105);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(10));
    REQUIRE((cardAt<EulerMeshGeneratorTable>(model, 0).propId == 16 &&
             cardAt<EulerMeshGeneratorTable>(model, 1).propId == 16));
    REQUIRE(cardAt<EulerPropTable>(model).id == 16);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(1));
    REQUIRE((cardAt<EulerMeshGeneratorTable>(model, 0).resizeTableIdX == 102 &&
             cardAt<EulerMeshGeneratorTable>(model, 0).resizeTableIdZ == 104));
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(1));
    REQUIRE(cardAt<EulerMeshGeneratorTable>(model, 0).frameId == 13);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    Model dangling = readBulk(freeCard("EulerMeshGenerator", BOX_ALL_FIELDS));
    REQUIRE(unresolvedIn(dangling) == 9); // the sub mesh, the property, three tables, three bias cards, the system
}

TEST_CASE("a MESH refuses what the format forbids", "[model][cards][explicit]")
{
    const auto fields = [](std::initializer_list<std::pair<int, std::string>> given)
    { return meshFields(std::vector<std::pair<int, std::string>>(given)); };

    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 1, "0"), "id");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 2, ""), "meshType"); // required
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 2, "CUBE"), "meshType");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 3, "-0.1"), "elemSizeX");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 4, "0."), "elemSizeY");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 6, "ABC"), "xReference");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 12, "-1."), "widthX");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 17, "0"), "elemCountX");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 17, "2.5"), "elemCountX");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 20, "1"), "subMeshId"); // the card is mesh 1
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 21, "-1"), "firstNodeId");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 22, "-1"), "firstElemId");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 23, "PLATE"), "propKind");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 24, ""), "propId"); // required
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 24, "0"), "propId");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 29, "None"), "remeshMethod");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 37, "-1"), "biasIdX");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 49, "-1"), "frameId");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 57, "0"), "elemsPerCube");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 58, "-1"), "cubeCountX");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 62, "0"), "partitionCountX");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 61, "W"), "partitionDirection");

    // The sizes, widths and numbers a mesh of each type must give.
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 19, ""), "elemCountZ");
    expectInvalid(
        freeCard("EulerMeshGenerator", fields({{1, "1"}, {2, "Box"}, {17, "10"}, {18, "10"}, {19, "10"}, {24, "5"}})),
        "elemSizeX");
    expectInvalid(freeCard("EulerMeshGenerator",
                           fields({{1, "1"}, {2, "Box"}, {12, "1."}, {17, "10"}, {18, "10"}, {19, "10"}, {24, "5"}})),
                  "widthY");
    expectInvalid(freeCard("EulerMeshGenerator", fields({{1, "2"}, {2, "Adaptive"}, {24, "5"}})), "elemSizeX");
    expectInvalid(freeCard("EulerMeshGenerator", fields({{1, "2"}, {2, "Adaptive"}, {3, "0.1"}, {24, "5"}})),
                  "elemSizeY");
    expectInvalid(freeCard("EulerMeshGenerator",
                           fields({{1, "2"}, {2, "Adaptive"}, {12, "1."}, {13, "1."}, {14, "1."}, {24, "5"}})),
                  "elemCountX");

    // RESIZE is for an adaptive mesh and needs a table; a box placed by a system cannot adapt.
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 25, "Scale"), "resize");
    expectInvalid(withField("EulerMeshGenerator", ADAPT_FIELDS, 25, "SHRINK"), "resize");
    expectInvalid(withField("EulerMeshGenerator", ADAPT_FIELDS, 25, "Scale"), "resizeTableIdX");
    expectInvalid(withField("EulerMeshGenerator", ADAPT_FIELDS, 25, "Length"), "resizeTableIdX");
    expectInvalid(withField("EulerMeshGenerator", ADAPT_FIELDS, 49, "3"), "frameId");

    // A biased mesh needs the grow factor of each direction unless a BIAS card of the direction gives the sizes.
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 33, "EDGE"), "biasStart");
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 33, "Center"), "growthX");
    expectInvalid(withField("EulerMeshGenerator", fieldsWith(BOX_FIELDS, 33, "Center"), 34, "2."), "growthY");
    expectInvalid(
        withField("EulerMeshGenerator", fieldsWith(fieldsWith(BOX_FIELDS, 33, "Reference"), 34, "2."), 35, "2."),
        "growthZ");
    expectInvalid(
        withField("EulerMeshGenerator", fieldsWith(fieldsWith(BOX_FIELDS, 33, "Reference"), 34, "2."), 35, "0."),
        "growthY");

    // The auxiliary box is a start point and three widths, all or nothing.
    expectInvalid(withField("EulerMeshGenerator", ADAPT_FIELDS, 41, "0."), "boxOriginY");
    expectInvalid(freeCard("EulerMeshGenerator", fields({{1, "2"},
                                                         {2, "Adaptive"},
                                                         {3, "0.1"},
                                                         {4, "0.1"},
                                                         {5, "0.1"},
                                                         {24, "5"},
                                                         {41, "0."},
                                                         {42, "0."},
                                                         {43, "0."},
                                                         {44, "-1."},
                                                         {45, "1."},
                                                         {46, "1."}})),
                  "boxWidthX");

    // The user partition needs NPX and a number of cubes that is a multiple of it in each direction.
    expectInvalid(withField("EulerMeshGenerator", BOX_FIELDS, 61, "User"), "partitionCountX");
    expectInvalid(
        freeCard("EulerMeshGenerator", fieldsWith(fieldsWith(fieldsWith(BOX_FIELDS, 58, "5"), 61, "User"), 62, "2")),
        "cubeCountX");
}
