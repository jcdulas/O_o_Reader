// Checks of the axisymmetric and SOL 400 cards: PAXISYM, PAXSYMH, PBSH2DT and PSPRMAT (properties), RBAX3D (a rigid
// element), LBSH3DG and NLBSH3D (the rotor bushing loads of SOL 400) and PLTSURF (a plot surface). Per card: the fields
// in their format reference positions, the defaults and the variants, the references it lists and what each one points
// to, the renumbering of the spaces it owns or names, the refused values and the round trip through a model file.
// The generic tests of CardRoundTripTest.cpp read, write and copy every sample; this file covers what each card means.

#include "oor/model/cards/elements/plotSurface/PlotSurfaceTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearBushForceTables/NonlinearBushForceTablesTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearBushLoad3D/NonlinearBushLoad3DTable.hpp"
#include "oor/model/cards/properties/axisymHarmonicProp/AxisymHarmonicPropTable.hpp"
#include "oor/model/cards/properties/axisymShellProp/AxisymShellPropTable.hpp"
#include "oor/model/cards/properties/bushProp2DFreq/BushProp2DFreqTable.hpp"
#include "oor/model/cards/properties/springOffsetProp/SpringOffsetPropTable.hpp"
#include "oor/model/cards/rigid_elements/rigidAxisymConnection/RigidAxisymConnectionTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The IDs that the references of a card name, as a visitor sees them.
struct Visited
{
    std::vector<std::string> fields;
    std::vector<ReferenceTarget> targets;
};

template <typename TableT>
Visited visitedBy(Model& model, std::uint32_t row)
{
    RecordingVisitor visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return Visited{visitor.fields, visitor.targets};
}

bool contains(const std::string& text, const std::string& part)
{
    return text.find(part) != std::string::npos;
}

// A TABLED1 of two points, which is all a card needs of the table it names.
std::string tabled1(int id)
{
    return "TableDynamic1;" + std::to_string(id) + ";;;0.;0.;1.;1.;EndTable\n";
}

// A TABLED5, the table of an LBSH3DG term: one TABLED1 for each of two rotor speeds.
std::string tabled5(int id, int first, int second)
{
    return "TableDynamic5;" + std::to_string(id) + ";0.;" + std::to_string(first) + ";10.;" + std::to_string(second) +
           ";EndTable\n";
}

// The ID that the tests give to the term of row `row` and column `column` (both from 1): 511 for P11, 566 for P66.
int termId(int row, int column)
{
    return 500 + 10 * row + column;
}

// The record of an LBSH3DG of the form ASYM with all its 36 terms, row after row of the matrix.
std::string asymmetricText(int lid)
{
    std::string text = "NonlinearBushForceTables;" + std::to_string(lid) + ";AsymmetricForm;10.;7000.";
    for (int row = 1; row <= 6; ++row)
    {
        for (int column = 1; column <= 6; ++column)
            text += ";" + std::to_string(termId(row, column));
    }
    return text + "\n";
}

// The record of an LBSH3DG of the form SYM (the form is left blank) with the 21 terms of the lower triangle (the
// terms above the diagonal are blank).
std::string symmetricText(int lid)
{
    std::string text = "NonlinearBushForceTables;" + std::to_string(lid) + ";;100.;6000.";
    for (int row = 1; row <= 6; ++row)
    {
        for (int column = 1; column <= 6; ++column)
            text += ";" + (column <= row ? std::to_string(termId(row, column)) : std::string());
    }
    return text + "\n";
}

// The TABLED1 and TABLED5 cards that give the 21 terms of the lower triangle their tables, so that no reference is left
// unresolved.
std::string triangleTables()
{
    std::string text = tabled1(101);
    for (int row = 1; row <= 6; ++row)
    {
        for (int column = 1; column <= row; ++column)
            text += tabled5(termId(row, column), 101, 101);
    }
    return text;
}

} // namespace

// --- PAXISYM --------------------------------------------------------------------------------------------------------

TEST_CASE("a PAXISYM is a primary property with a material, two thicknesses and an analysis type",
          "[model][cards][paxisym]")
{
    REQUIRE((AxisymShellPropTable::INFO.category == CardCategory::Property &&
             AxisymShellPropTable::INFO.idSpace == base::IdSpace::Property));
    Model model = readBulk(
        "IsoMat;2017;2.1E+11;;0.3\n"
        "HeatIsoMat;2005;1.\n"
        "AxisymShellProp;2098;2017;0.125\n"
        "AxisymShellProp;2099;2017;0.125;0.25;Structural\n"
        "AxisymShellProp;2100;2005;0.125;;Heat\n"
        "AxisymShellProp;2101;2017;0.125;0.;StructuralAndHeat\n");

    // T1 alone: the third field is a blank cell and T1 is in the fourth.
    const AxisymShellPropCard example = cardAt<AxisymShellPropTable>(model, 0);
    REQUIRE((example.id == 2098 && example.matId == 2017 && example.thickness1 == 0.125));
    REQUIRE(
        (example.thickness2 == 0.0 && example.analysisType == AxisymShellPropAnalysis::StructuralHeat)); // the defaults
    const AxisymShellPropCard structural = cardAt<AxisymShellPropTable>(model, 1);
    REQUIRE((structural.thickness2 == 0.25 && structural.analysisType == AxisymShellPropAnalysis::Structural));
    const AxisymShellPropCard heat = cardAt<AxisymShellPropTable>(model, 2);
    REQUIRE((heat.matId == 2005 &&
             heat.analysisType == AxisymShellPropAnalysis::Heat)); // any case; a MAT4 for heat transfer
    REQUIRE(cardAt<AxisymShellPropTable>(model, 3).analysisType == AxisymShellPropAnalysis::StructuralHeat);

    // The defaults are blank in the text, and written when asked for.
    const Model plain = readBulk("AxisymShellProp;2101;2017;0.125;0.;StructuralAndHeat\n");
    REQUIRE_FALSE(contains(writeModelText(plain), "StructuralAndHeat"));
    REQUIRE(contains(writeModelText(plain, false), "StructuralAndHeat"));
    requireRoundTrip(model);

    // The material is a MAT1 or, for heat transfer, a MAT4: both exist here, and the PID is one of the properties.
    REQUIRE(unresolvedIn(model) == 0);
    const Visited visited = visitedBy<AxisymShellPropTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"matId"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    Model missing = readBulk("AxisymShellProp;2098;2017;0.125\n");
    REQUIRE(unresolvedIn(missing) == 1);

    // The property space is the one of every property: a PSHELL and a PAXISYM cannot share an ID.
    REQUIRE_THROWS_AS(readBulk("AxisymShellProp;2098;2017;0.125\nShellProp;2098;2017;0.125"), base::DeckError);

    expectInvalid("AxisymShellProp;0;2017;0.125", "id");
    expectInvalid("AxisymShellProp;2098", "matId"); // required
    expectInvalid("AxisymShellProp;2098;0;0.125", "matId");
    expectInvalid("AxisymShellProp;2098;2017", "thickness1"); // required
    expectInvalid("AxisymShellProp;2098;2017;0.", "thickness1");
    expectInvalid("AxisymShellProp;2098;2017;-0.125", "thickness1");
    expectInvalid("AxisymShellProp;2098;2017;abc", "thickness1");
    expectInvalid("AxisymShellProp;2098;2017;0.125;-0.25", "thickness2");
    expectInvalid("AxisymShellProp;2098;2017;0.125;0.25;XX", "analysisType");
}

TEST_CASE("a PAXISYM is named by a CAXISYM and follows the renumbering of the materials and of the properties",
          "[model][cards][paxisym]")
{
    Model model = readBulk(
        "IsoMat;2017;2.1E+11;;0.3\n"
        "Node;1;;1.;0.;0.\n"
        "Node;2;;2.;0.;0.\n"
        "AxisymShellProp;2098;2017;0.125\n"
        "AxisymShellProp;2099;2017;0.125;0.25;Structural\n"
        "ShellAxisym;5;2098;1;2\n");
    REQUIRE(unresolvedIn(model) == 0); // the CAXISYM finds its property now that the card is typed

    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<AxisymShellPropTable>(model, 0).matId == 3017 && cardAt<AxisymShellPropTable>(model, 1).matId == 3017));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<AxisymShellPropTable>(model, 0).id == 3098 && cardAt<AxisymShellPropTable>(model, 1).id == 3099));
    REQUIRE(model.locate(base::IdSpace::Property, 3098).has_value());
    REQUIRE_FALSE(model.locate(base::IdSpace::Property, 2098).has_value());
    REQUIRE(unresolvedIn(model) == 0); // the PID of the CAXISYM followed
    requireRoundTrip(model);

    // What the Python layer does when it edits a card: the values are checked, then stored in the row.
    AxisymShellPropTable& table = model.table<AxisymShellPropTable>();
    AxisymShellPropCard edited = table.card(1);
    edited.thickness2 = 0.3;
    edited.analysisType = AxisymShellPropAnalysis::Heat;
    REQUIRE_FALSE(AxisymShellPropFormat::check(edited).has_value());
    table.store(1, edited);
    REQUIRE((cardAt<AxisymShellPropTable>(model, 1).thickness2 == 0.3 &&
             cardAt<AxisymShellPropTable>(model, 1).analysisType == AxisymShellPropAnalysis::Heat));
    edited.thickness1 = -1.0;
    REQUIRE(AxisymShellPropFormat::check(edited).has_value());

    // A deleted card leaves its ID free.
    const std::size_t before = model.cardCount();
    model.remove(*model.locate(base::IdSpace::Property, 3099));
    REQUIRE(model.cardCount() == before - 1);
    REQUIRE_FALSE(model.locate(base::IdSpace::Property, 3099).has_value());
}

// --- PAXSYMH --------------------------------------------------------------------------------------------------------

TEST_CASE("a PAXSYMH has the format reference defaults and is the property that a CQUADX and a CTRIAX name",
          "[model][cards][paxsymh]")
{
    REQUIRE((AxisymHarmonicPropTable::INFO.category == CardCategory::Property &&
             AxisymHarmonicPropTable::INFO.idSpace == base::IdSpace::Property));
    Model model = readBulk(
        "IsoMat;3010;2.1E+11;;0.3\n"
        "RectFrame2;3005\n"
        "Node;1;;1.;0.;0.\n"
        "Node;2;;2.;0.;0.\n"
        "Node;3;;2.;1.;0.\n"
        "Node;4;;1.;1.;0.\n"
        "AxisymHarmonicProp;3100;3010;3005\n"
        "AxisymHarmonicProp;3101;3010\n"
        "AxisymHarmonicProp;3102;3010;3005;2;3\n"
        "AxisymHarmonicProp;3103;3010;;0;1\n"
        "AxisymQuad;7;3100;1;2;3;4\n"
        "AxisymTria;8;3101;1;2;3\n");

    const AxisymHarmonicPropCard example = cardAt<AxisymHarmonicPropTable>(model, 0);
    REQUIRE((example.id == 3100 && example.matId == 3010 && example.frameId == 3005)); // NHARM, INT blank
    REQUIRE((example.harmonicIndex == 1 && example.integration == 0));                 // the defaults
    const AxisymHarmonicPropCard minimal = cardAt<AxisymHarmonicPropTable>(model, 1);
    REQUIRE((minimal.frameId == 0 && minimal.harmonicIndex == 1 && minimal.integration == 0));
    const AxisymHarmonicPropCard full = cardAt<AxisymHarmonicPropTable>(model, 2);
    REQUIRE((full.frameId == 3005 && full.harmonicIndex == 2 && full.integration == 3));
    // NHARM may be 0 (the image of the format says 0 or more) and INT 1 is the experimental scheme.
    const AxisymHarmonicPropCard zero = cardAt<AxisymHarmonicPropTable>(model, 3);
    REQUIRE((zero.frameId == 0 && zero.harmonicIndex == 0 && zero.integration == 1));
    requireRoundTrip(model);

    REQUIRE(unresolvedIn(model) ==
            0); // the CQUADX and the CTRIAX find their property, the material and the system exist
    const Visited withSystem = visitedBy<AxisymHarmonicPropTable>(model, 0);
    REQUIRE(withSystem.fields == std::vector<std::string>{"matId", "frameId"});
    REQUIRE(withSystem.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::Coordinate});
    // A blank CID is the basic system: nothing to look up.
    REQUIRE(visitedBy<AxisymHarmonicPropTable>(model, 1).fields == std::vector<std::string>{"matId"});

    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE((cardAt<AxisymHarmonicPropTable>(model, 0).frameId == 3015 &&
             cardAt<AxisymHarmonicPropTable>(model, 1).frameId == 0));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE(cardAt<AxisymHarmonicPropTable>(model, 2).matId == 4010);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(500));
    REQUIRE(
        (cardAt<AxisymHarmonicPropTable>(model, 0).id == 3600 && cardAt<AxisymHarmonicPropTable>(model, 3).id == 3603));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    Model missing = readBulk("AxisymHarmonicProp;3100;3010;3005\n");
    REQUIRE(unresolvedIn(missing) == 2); // no material and no coordinate system

    // The property space is shared with the PAXISYM.
    REQUIRE_THROWS_AS(readBulk("AxisymHarmonicProp;3100;3010\nAxisymShellProp;3100;3010;0.1"), base::DeckError);

    expectInvalid("AxisymHarmonicProp;0;3010", "id");
    expectInvalid("AxisymHarmonicProp;3100", "matId"); // required
    expectInvalid("AxisymHarmonicProp;3100;0", "matId");
    expectInvalid("AxisymHarmonicProp;3100;3010;-1", "frameId");
    expectInvalid("AxisymHarmonicProp;3100;3010;1.5", "frameId");
    expectInvalid("AxisymHarmonicProp;3100;3010;;-1", "harmonicIndex");
    expectInvalid("AxisymHarmonicProp;3100;3010;;;4", "integration");
    expectInvalid("AxisymHarmonicProp;3100;3010;;;-1", "integration");
    expectInvalid("AxisymHarmonicProp;3100;3010;;;x", "integration");
}

// --- PBSH2DT --------------------------------------------------------------------------------------------------------

TEST_CASE("a PBSH2DT has three groups of four tables, each led by a flag, in any order", "[model][cards][pbsh2dt]")
{
    REQUIRE((BushProp2DFreqTable::INFO.category == CardCategory::Property &&
             BushProp2DFreqTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "BushProp2D;20;1.;1.\n"
        "BushProp2D;21;1.;1.\n"
        "BushProp2D;22;1.;1.\n"
        "BushProp2D;23;1.;1.\n"
        "BushProp2DFreq;20;Stiffness;101;102;103;104;Damping;201;202;;;Mass;;;301\n"
        "BushProp2DFreq;21\n"
        "BushProp2DFreq;22;Mass;301;302;;;Stiffness;101\n"
        "BushProp2DFreq;23;Damping;;;;204\n");

    const BushProp2DFreqCard full = cardAt<BushProp2DFreqTable>(model, 0);
    REQUIRE(full.id == 20);
    REQUIRE(full.stiffnessTableIds == std::array<base::EntityId, 4>{101, 102, 103, 104}); // K11, K22, K12, K21
    REQUIRE(full.dampingTableIds == std::array<base::EntityId, 4>{201, 202, 0, 0});
    REQUIRE(full.massTableIds == std::array<base::EntityId, 4>{0, 0, 301, 0}); // M12 alone
    const BushProp2DFreqCard bare = cardAt<BushProp2DFreqTable>(model, 1);
    REQUIRE((bare.stiffnessTableIds == std::array<base::EntityId, 4>{} &&
             bare.dampingTableIds == bare.stiffnessTableIds && bare.massTableIds == bare.stiffnessTableIds));
    // The groups are read in any order: M before K.
    const BushProp2DFreqCard reversed = cardAt<BushProp2DFreqTable>(model, 2);
    REQUIRE(reversed.stiffnessTableIds == std::array<base::EntityId, 4>{101, 0, 0, 0});
    REQUIRE(reversed.massTableIds == std::array<base::EntityId, 4>{301, 302, 0, 0});
    // A group on the line of the PID only: the flag is in the second field.
    REQUIRE(cardAt<BushProp2DFreqTable>(model, 3).dampingTableIds == std::array<base::EntityId, 4>{0, 0, 0, 204});
    requireRoundTrip(model);

    // The card is written with the groups in the order K, B, M.
    const Model order = readBulk("BushProp2DFreq;22;Mass;301;302;;;Stiffness;101\n");

    oor::testing::requireModelFileRoundTrip(order);
}

TEST_CASE("a PBSH2DT extends the PBUSH2D of its ID and names its tables as TABLED1 entries", "[model][cards][pbsh2dt]")
{
    Model model = readBulk(
        "BushProp2D;20;1.;1.\nBushProp2DFreq;20;Stiffness;101;102;103;104;Damping;201;202;;;Mass;;;301\n" +
        tabled1(101) + tabled1(102) + tabled1(103) + tabled1(104) + tabled1(201) + tabled1(202) + tabled1(301));
    REQUIRE(unresolvedIn(model) == 0);

    // The ID, then the tables of K, B and M in the order of the format, the blank ones skipped.
    const Visited visited = visitedBy<BushProp2DFreqTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"id", "stiffnessTableId11", "stiffnessTableId22",
                                                       "stiffnessTableId12", "stiffnessTableId21", "dampingTableId11",
                                                       "dampingTableId22", "massTableId12"});
    REQUIRE(visited.targets[0] == ReferenceTarget::Property);
    for (std::size_t index = 1; index < visited.targets.size(); ++index)
        REQUIRE(visited.targets[index] == ReferenceTarget::DynamicTable);

    // A PBUSH2D that is missing and a table that is missing are two unresolved references.
    Model missing = readBulk("BushProp2DFreq;20;Stiffness;101\n");
    REQUIRE(unresolvedIn(missing) == 2);

    // The ID of the card is the one of the property: it follows, and is in a space of its own (a PBUSH2D and its
    // PBSH2DT have the same ID, two PBSH2DT may not).
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(100));
    REQUIRE(cardAt<BushProp2DFreqTable>(model).id == 120);
    REQUIRE(model.locateInType(typeIndexOf<BushProp2DFreqTable>(), 120).has_value());
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(1000));
    const BushProp2DFreqCard moved = cardAt<BushProp2DFreqTable>(model);
    REQUIRE(moved.stiffnessTableIds == std::array<base::EntityId, 4>{1101, 1102, 1103, 1104});
    REQUIRE(moved.dampingTableIds == std::array<base::EntityId, 4>{1201, 1202, 0, 0});
    REQUIRE(moved.massTableIds == std::array<base::EntityId, 4>{0, 0, 1301, 0});
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE_THROWS_AS(readBulk("BushProp2DFreq;20;Stiffness;101\nBushProp2DFreq;20;Damping;201"), base::DeckError);

    // Editing, as the Python layer does: a table of another group, checked and stored.
    BushProp2DFreqTable& table = model.table<BushProp2DFreqTable>();
    BushProp2DFreqCard edited = table.card(0);
    edited.dampingTableIds = {1201, 0, 1203, 0};
    REQUIRE_FALSE(BushProp2DFreqFormat::check(edited).has_value());
    table.store(0, edited);
    REQUIRE(cardAt<BushProp2DFreqTable>(model).dampingTableIds == std::array<base::EntityId, 4>{1201, 0, 1203, 0});
    edited.dampingTableIds[3] = -4;
    REQUIRE(BushProp2DFreqFormat::check(edited).has_value());
    requireRoundTrip(model);
}

TEST_CASE("a PBSH2DT refuses a bad flag, a group twice, a negative table and a field outside the grid",
          "[model][cards][pbsh2dt]")
{
    expectInvalid("BushProp2DFreq;0;Stiffness;101", "id");
    expectInvalid("BushProp2DFreq", "id");
    expectInvalid("BushProp2DFreq;20;Stiffness;101;;;;Stiffness;102", "id"); // a group once only
    expectInvalid("BushProp2DFreq;20;Stiffness;101;;;;Damping;201;;;;Mass;301;;;;Damping;202", "id");
    expectInvalid("BushProp2DFreq;20;Stiffness;-101", "stiffnessTableId11");
    expectInvalid("BushProp2DFreq;20;Stiffness;;;;-1", "stiffnessTableId21");
    expectInvalid("BushProp2DFreq;20;Stiffness;101;;;;Damping;;;;-2", "dampingTableId21");
    expectInvalid("BushProp2DFreq;20;Stiffness;101;;;;Damping;201;;;;Mass;;-3", "massTableId22");
    expectInvalid("BushProp2DFreq;20;Stiffness;abc", "stiffnessTableId11");
}

// --- PSPRMAT --------------------------------------------------------------------------------------------------------

TEST_CASE("a PSPRMAT is a linear spring (K) or a nonlinear one (LOAD) and has its values on the third line",
          "[model][cards][psprmat]")
{
    REQUIRE((SpringOffsetPropTable::INFO.category == CardCategory::Property &&
             SpringOffsetPropTable::INFO.idSpace == base::IdSpace::Property));
    Model model = readBulk(
        "SpringOffsetProp;1;12500.\n"
        "SpringOffsetProp;2;2.5E+4;300.;150.\n"
        "SpringOffsetProp;3;2.5E+4;;150.\n"
        "SpringOffsetProp;4;;;;201\n"
        "SpringOffsetProp;5;;;;201;202\n"); // with an unloading path

    const SpringOffsetPropCard example = cardAt<SpringOffsetPropTable>(model, 0);
    REQUIRE((example.id == 1 && example.stiffness == 12500.0 && example.isLinear()));
    // no default: the spring does not fail
    REQUIRE_FALSE((example.tensileFailureForce.has_value() || example.compressiveFailureForce.has_value()));
    REQUIRE((example.loadingTableId == 0 && example.unloadingTableId == 0));
    const SpringOffsetPropCard failing = cardAt<SpringOffsetPropTable>(model, 1);
    REQUIRE((failing.tensileFailureForce == 300.0 && failing.compressiveFailureForce == 150.0));
    const SpringOffsetPropCard compressive = cardAt<SpringOffsetPropTable>(model, 2);
    REQUIRE_FALSE(compressive.tensileFailureForce.has_value());
    REQUIRE(compressive.compressiveFailureForce == 150.0);
    const SpringOffsetPropCard curve = cardAt<SpringOffsetPropTable>(model, 3);
    REQUIRE((!curve.isLinear() && curve.loadingTableId == 201 && curve.unloadingTableId == 0));
    REQUIRE((cardAt<SpringOffsetPropTable>(model, 4).loadingTableId == 201 &&
             cardAt<SpringOffsetPropTable>(model, 4).unloadingTableId == 202));
    requireRoundTrip(model);

    // The two tables are TABLED1 entries (the unloading one is a TABLExx on the format: see the comment of the card).
    const Visited visited = visitedBy<SpringOffsetPropTable>(model, 4);
    REQUIRE(visited.fields == std::vector<std::string>{"loadingTableId", "unloadingTableId"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::DynamicTable));
    REQUIRE(visitedBy<SpringOffsetPropTable>(model, 0).fields.empty());
    REQUIRE(visitedBy<SpringOffsetPropTable>(model, 3).fields == std::vector<std::string>{"loadingTableId"});
}

TEST_CASE("a PSPRMAT is named by a CSPR, names its curves and follows the renumbering", "[model][cards][psprmat]")
{
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "SpringOffsetExplicit;10;3;1;2\n"
        "SpringOffsetExplicit;11;1;1;2\n"
        "SpringOffsetProp;1;12500.\n"
        "SpringOffsetProp;3;;;;201;202\n" +
        tabled1(201) + tabled1(202));
    REQUIRE(unresolvedIn(model) == 0); // the CSPR cards find their property now that the card is typed

    Model missing = readBulk("SpringOffsetProp;3;;;;201;202\n");
    REQUIRE(unresolvedIn(missing) == 2);

    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(1000));
    REQUIRE((cardAt<SpringOffsetPropTable>(model, 1).loadingTableId == 1201 &&
             cardAt<SpringOffsetPropTable>(model, 1).unloadingTableId == 1202));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(100));
    REQUIRE((cardAt<SpringOffsetPropTable>(model, 0).id == 101 && cardAt<SpringOffsetPropTable>(model, 1).id == 103));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // Editing as the Python layer does: a linear spring becomes nonlinear by changing K and LOAD together.
    SpringOffsetPropTable& table = model.table<SpringOffsetPropTable>();
    SpringOffsetPropCard edited = table.card(0);
    edited.loadingTableId = 1201;
    REQUIRE(SpringOffsetPropFormat::check(edited).has_value()); // K and LOAD together
    edited.stiffness.reset();
    REQUIRE_FALSE(SpringOffsetPropFormat::check(edited).has_value());
    table.store(0, edited);
    REQUIRE_FALSE(cardAt<SpringOffsetPropTable>(model, 0).isLinear());
    edited.loadingTableId = 0;
    REQUIRE(SpringOffsetPropFormat::check(edited).has_value()); // neither

    REQUIRE_THROWS_AS(readBulk("SpringOffsetProp;1;12500.\nSpringProp;1;1.\n"), base::DeckError); // the property space
}

TEST_CASE("a PSPRMAT refuses a mix of the two forms, a missing K or LOAD and a K in the wrong field",
          "[model][cards][psprmat]")
{
    expectInvalid("SpringOffsetProp;0;12500.", "id");
    expectInvalid("SpringOffsetProp;1", "stiffness");       // neither K nor LOAD
    expectInvalid("SpringOffsetProp;1;;300.", "stiffness"); // a failure force needs K, and LOAD alone is not given
    expectInvalid("SpringOffsetProp;1;0.", "stiffness");
    expectInvalid("SpringOffsetProp;1;-5.", "stiffness");
    expectInvalid("SpringOffsetProp;1;abc", "stiffness");
    expectInvalid("SpringOffsetProp;1;12500.;0.", "tensileFailureForce");
    expectInvalid("SpringOffsetProp;1;12500.;;-1.", "compressiveFailureForce");
    expectInvalid("SpringOffsetProp;1;12500.;;;201", "loadingTableId"); // K excludes LOAD
    expectInvalid("SpringOffsetProp;1;12500.;;;;202", "unloadingTableId");
    expectInvalid("SpringOffsetProp;1;;;;-201", "loadingTableId");
    expectInvalid("SpringOffsetProp;1;;;;201;-202", "unloadingTableId");
    expectInvalid("SpringOffsetProp;1;;;;;202", "stiffness"); // an UNLOAD with no LOAD
    // The values are on the third line: a K on the second one is a field of the grid that must stay empty.
}

// --- RBAX3D ---------------------------------------------------------------------------------------------------------

TEST_CASE("an RBAX3D is a rigid element between a point on the axis and a point of an axisymmetric element",
          "[model][cards][rbax3d]")
{
    REQUIRE((RigidAxisymConnectionTable::INFO.category == CardCategory::Element &&
             RigidAxisymConnectionTable::INFO.idSpace == base::IdSpace::Element));
    Model model = readBulk(
        "Node;4010;;0.;0.;1.\n"
        "Node;4020;;2.;0.;1.\n"
        "Node;100000012;;0.;0.;2.\n"
        "Node;4021;;3.;0.;2.\n"
        "RigidAxisymConnection;4100;4010;4020\n"
        "RigidAxisymConnection;4101;100000012;4021\n");

    // A G3D below the describer's lower limit of 100,000,000 is read all the same.
    const RigidAxisymConnectionCard example = cardAt<RigidAxisymConnectionTable>(model, 0);
    REQUIRE((example.id == 4100 && example.node3D == 4010 && example.axisymNode == 4020));
    const RigidAxisymConnectionCard large = cardAt<RigidAxisymConnectionTable>(model, 1);
    REQUIRE((large.id == 4101 && large.node3D == 100000012 && large.axisymNode == 4021));
    requireRoundTrip(model);

    REQUIRE(unresolvedIn(model) == 0);
    const Visited visited = visitedBy<RigidAxisymConnectionTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"node3D", "axisymNode"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::Node));
    Model missing = readBulk("Node;4010;;0.;0.;1.\nRigidAxisymConnection;4100;4010;4020\n");
    REQUIRE(unresolvedIn(missing) == 1);

    // A rigid element is an element: the ID is shared with all the others.
    REQUIRE_THROWS_AS(readBulk("RigidAxisymConnection;4100;4010;4020\nRod2;4100;1;1;2"), base::DeckError);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<RigidAxisymConnectionTable>(model, 0).node3D == 5010 &&
             cardAt<RigidAxisymConnectionTable>(model, 0).axisymNode == 5020));
    REQUIRE(cardAt<RigidAxisymConnectionTable>(model, 1).node3D == 100001012);
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(5));
    REQUIRE((cardAt<RigidAxisymConnectionTable>(model, 0).id == 4105 &&
             cardAt<RigidAxisymConnectionTable>(model, 1).id == 4106));
    REQUIRE(model.locate(base::IdSpace::Element, 4105).has_value());
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // Editing as the Python layer does.
    RigidAxisymConnectionTable& table = model.table<RigidAxisymConnectionTable>();
    RigidAxisymConnectionCard edited = table.card(0);
    edited.axisymNode = edited.node3D;
    REQUIRE(RigidAxisymConnectionFormat::check(edited).has_value());
    edited.axisymNode = 5030;
    REQUIRE_FALSE(RigidAxisymConnectionFormat::check(edited).has_value());
    table.store(0, edited);
    REQUIRE(cardAt<RigidAxisymConnectionTable>(model, 0).axisymNode == 5030);

    expectInvalid("RigidAxisymConnection;0;4010;4020", "id");
    expectInvalid("RigidAxisymConnection;100000000;4010;4020", "id"); // below 100,000,000
    expectInvalid("RigidAxisymConnection;-5;4010;4020", "id");
    expectInvalid("RigidAxisymConnection;4100;0;4020", "node3D");
    expectInvalid("RigidAxisymConnection;4100", "node3D");
    expectInvalid("RigidAxisymConnection;4100;4010", "axisymNode");
    expectInvalid("RigidAxisymConnection;4100;4010;0", "axisymNode");
    expectInvalid("RigidAxisymConnection;4100;4010;100000000", "axisymNode"); // below 100,000,000
    expectInvalid("RigidAxisymConnection;4100;4010;4010", "axisymNode");      // two different points
    expectInvalid("RigidAxisymConnection;4100;4010;abc", "axisymNode");
}

// --- LBSH3DG --------------------------------------------------------------------------------------------------------

TEST_CASE("an LBSH3DG of the form SYM has the lower triangle of the matrix, one row to a line",
          "[model][cards][lbsh3dg]")
{
    REQUIRE((NonlinearBushForceTablesTable::INFO.category == CardCategory::BushingForceSet &&
             NonlinearBushForceTablesTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(symmetricText(1));
    const NonlinearBushForceTablesCard card = cardAt<NonlinearBushForceTablesTable>(model);
    REQUIRE((card.id == 1 && card.form == BushingForm::Symmetric)); // a blank FORM is SYM
    REQUIRE((card.minimumFreq == 100.0 && card.maximumFreq == 6000.0));
    REQUIRE((card.tables[0][0] == 511 && card.tables[1][0] == 521 && card.tables[1][1] == 522));
    REQUIRE((card.tables[2][1] == 532 && card.tables[5][0] == 561 && card.tables[5][5] == 566));
    // The terms over the diagonal are not in the form.
    REQUIRE((card.tables[0][1] == 0 && card.tables[0][5] == 0 && card.tables[4][5] == 0));
    requireRoundTrip(model);

    // The form SYM written is the one the default leaves blank.
    const std::string text = writeModelText(model);
    REQUIRE_FALSE(contains(text, "SymmetricForm"));
    REQUIRE(contains(writeModelText(model, false), "SymmetricForm"));

    // SYM may be given as a word, in any case, and a card may have no table at all (a term is blank, not required).
    const Model word = readBulk("NonlinearBushForceTables;2;SymmetricForm;;5.E+3;511\n");
    REQUIRE(cardAt<NonlinearBushForceTablesTable>(word).form == BushingForm::Symmetric);
    const Model empty = readBulk("NonlinearBushForceTables;3;;;5.E+3");
    REQUIRE((cardAt<NonlinearBushForceTablesTable>(empty).minimumFreq == 0.0 &&
             cardAt<NonlinearBushForceTablesTable>(empty).tables[0][0] == 0)); // MIN_FREQ blank: 0.0
    requireRoundTrip(word);
    requireRoundTrip(empty);
}

TEST_CASE("an LBSH3DG of the form DIAG has the six diagonal terms on one line", "[model][cards][lbsh3dg]")
{
    Model model = readBulk("NonlinearBushForceTables;2;DiagonalForm;;5.E+3;511;522;533;544;555;566\n");
    const NonlinearBushForceTablesCard card = cardAt<NonlinearBushForceTablesTable>(model);
    REQUIRE((card.form == BushingForm::Diagonal && card.minimumFreq == 0.0 && card.maximumFreq == 5000.0));
    for (std::size_t index = 0; index < 6; ++index)
        REQUIRE(card.tables[index][index] == static_cast<base::EntityId>(511 + 11 * index));
    REQUIRE((card.tables[1][0] == 0 && card.tables[0][1] == 0));
    requireRoundTrip(model);

    // The off-diagonal fields of the grid are not in the form: a term on the next line is an error, and so is one more.
    expectInvalid("NonlinearBushForceTables;2;DiagonalForm;;5.E+3;511;522;;;;;521", "");
}

TEST_CASE("an LBSH3DG of the form ASYM has the 36 terms, six to a line", "[model][cards][lbsh3dg]")
{
    Model model = readBulk(asymmetricText(3));
    const NonlinearBushForceTablesCard card = cardAt<NonlinearBushForceTablesTable>(model);
    REQUIRE((card.form == BushingForm::Asymmetric && card.minimumFreq == 10.0 && card.maximumFreq == 7000.0));
    for (int row = 1; row <= 6; ++row)
    {
        for (int column = 1; column <= 6; ++column)
            REQUIRE(card.tables[row - 1][column - 1] == termId(row, column));
    }
    requireRoundTrip(model);

    // Terms may be left blank: the rest of the grid is where the format puts it.
    const Model sparse = readBulk("NonlinearBushForceTables;4;AsymmetricForm;;2500.;611;;;;;616;;622\n");
    const NonlinearBushForceTablesCard few = cardAt<NonlinearBushForceTablesTable>(sparse);
    REQUIRE((few.tables[0][0] == 611 && few.tables[0][5] == 616 && few.tables[1][1] == 622));
    REQUIRE((few.tables[0][1] == 0 && few.tables[1][0] == 0));
    requireRoundTrip(sparse);
}

TEST_CASE("an LBSH3DG names a TABLED5 for each term and follows the renumbering of the tables and of its own ID",
          "[model][cards][lbsh3dg]")
{
    Model model = readBulk(symmetricText(1) + triangleTables());
    REQUIRE(unresolvedIn(model) == 0);
    const Visited visited = visitedBy<NonlinearBushForceTablesTable>(model, 0);
    REQUIRE(visited.fields.size() == 21);
    REQUIRE((visited.fields.front() == "termTable11" && visited.fields[1] == "termTable21" &&
             visited.fields[2] == "termTable22"));
    REQUIRE(visited.fields.back() == "termTable66");
    REQUIRE(visited.targets == std::vector<ReferenceTarget>(21, ReferenceTarget::DynamicTable));
    // A DIAG card names six, an ASYM card 36.
    Model diagonal = readBulk("NonlinearBushForceTables;2;DiagonalForm;;5.E+3;511;522;533;544;555;566\n");
    REQUIRE(visitedBy<NonlinearBushForceTablesTable>(diagonal, 0).fields.size() == 6);
    Model asymmetric = readBulk(asymmetricText(3));
    REQUIRE(visitedBy<NonlinearBushForceTablesTable>(asymmetric, 0).fields.size() == 36);
    // The tables are not there: every term is an unresolved reference.
    REQUIRE(unresolvedIn(diagonal) == 6);

    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(1000));
    const NonlinearBushForceTablesCard moved = cardAt<NonlinearBushForceTablesTable>(model);
    REQUIRE((moved.tables[0][0] == 1511 && moved.tables[5][5] == 1566 && moved.tables[0][1] == 0));
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::BushingForceSet, Renumbering::offset(100));
    REQUIRE(cardAt<NonlinearBushForceTablesTable>(model).id == 101);
    REQUIRE(model.locateInType(typeIndexOf<NonlinearBushForceTablesTable>(), 101).has_value());
    requireRoundTrip(model);

    // The ID space is the card type's own: a second LBSH3DG with the ID is a duplicate.
    REQUIRE_THROWS_AS(readBulk("NonlinearBushForceTables;1;;;5.E+3\nNonlinearBushForceTables;1;DiagonalForm;;6.E+3"),
                      base::DeckError);
}

TEST_CASE("an LBSH3DG is edited with the tables that have a place in its form", "[model][cards][lbsh3dg]")
{
    Model model = readBulk(symmetricText(1));
    NonlinearBushForceTablesTable& table = model.table<NonlinearBushForceTablesTable>();
    NonlinearBushForceTablesCard edited = table.card(0);
    // A term over the diagonal has no place in the form SYM.
    edited.tables[0][1] = 512;
    const auto problem = NonlinearBushForceTablesFormat::check(edited);
    REQUIRE(problem.has_value());
    REQUIRE(std::string(problem->field) == "termTable12");
    // The form ASYM has it.
    edited.form = BushingForm::Asymmetric;
    REQUIRE_FALSE(NonlinearBushForceTablesFormat::check(edited).has_value());
    table.store(0, edited);
    REQUIRE(cardAt<NonlinearBushForceTablesTable>(model).tables[0][1] == 512);
    requireRoundTrip(model);
    // And the form DIAG has not the terms under the diagonal either.
    edited.form = BushingForm::Diagonal;
    const auto diagonal = NonlinearBushForceTablesFormat::check(edited);
    REQUIRE(diagonal.has_value());
    REQUIRE(std::string(diagonal->field) == "termTable12");

    // The positions of the terms: row i is on the line of its number, the diagonal of DIAG on the first line.
    REQUIRE(bushingTermField(BushingForm::Symmetric, 0, 0) == 9);
    REQUIRE(bushingTermField(BushingForm::Symmetric, 1, 0) == 17);
    REQUIRE(bushingTermField(BushingForm::Asymmetric, 0, 5) == 14);
    REQUIRE(bushingTermField(BushingForm::Asymmetric, 5, 5) == 54);
    REQUIRE(bushingTermField(BushingForm::Diagonal, 0, 0) == 9);
    REQUIRE(bushingTermField(BushingForm::Diagonal, 5, 5) == 14);
}

TEST_CASE("an LBSH3DG refuses a bad FORM, a rotor speed range that is empty, a negative table and a stray field",
          "[model][cards][lbsh3dg]")
{
    expectInvalid("NonlinearBushForceTables;0;;;5.E+3", "id");
    expectInvalid("NonlinearBushForceTables;1", "maximumFreq"); // required
    expectInvalid("NonlinearBushForceTables;1;;;0.", "maximumFreq");
    expectInvalid("NonlinearBushForceTables;1;;;-5.", "maximumFreq");
    expectInvalid("NonlinearBushForceTables;1;;7.E+3;6.E+3", "minimumFreq");
    expectInvalid("NonlinearBushForceTables;1;;abc;6.E+3", "minimumFreq");
    expectInvalid("NonlinearBushForceTables;1;BAD;;5.E+3", "form");
    expectInvalid("NonlinearBushForceTables;1;;;5.E+3;-5", "termTable11");
    expectInvalid("NonlinearBushForceTables;1;;;5.E+3;511;512", "");              // the form SYM has no P12
    expectInvalid("NonlinearBushForceTables;1;;;5.E+3;511;;;;;;521;522;523", ""); // nor P23
    expectInvalid("NonlinearBushForceTables;1;AsymmetricForm;;5.E+3;x", "termTable11");
}

// --- NLBSH3D --------------------------------------------------------------------------------------------------------

TEST_CASE("an NLBSH3D is a nonlinear load between two grid points that names two LBSH3DG", "[model][cards][nlbsh3d]")
{
    REQUIRE((NonlinearBushLoad3DTable::INFO.category == CardCategory::DynamicLoad &&
             NonlinearBushLoad3DTable::INFO.idSpace == base::IdSpace::Group));
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "Node;2;;0.;0.;0.\n"
        "Node;3;;1.;0.;0.\n"
        "Node;4;;1.;0.;0.\n"
        "NonlinearBushForceTables;10;DiagonalForm;;5.E+3\n"
        "NonlinearBushForceTables;11;;;5.E+3\n"
        "NonlinearBushForceTables;12;AsymmetricForm;;5.E+3\n"
        "NonlinearBushLoad3D;7;1;2;10;11\n"
        "NonlinearBushLoad3D;7;3;4;12;12\n"
        "NonlinearBushLoad3D;8;1;2;10;10\n");

    const NonlinearBushLoad3DCard card = cardAt<NonlinearBushLoad3DTable>(model, 0);
    REQUIRE((card.setId == 7 && card.nodeA == 1 && card.nodeB == 2 && card.loadId1 == 10 && card.loadId2 == 11));
    // Several cards share a SID, with other grid pairs, and several may name one LBSH3DG.
    REQUIRE((cardAt<NonlinearBushLoad3DTable>(model, 1).setId == 7 &&
             cardAt<NonlinearBushLoad3DTable>(model, 1).nodeA == 3));
    REQUIRE((cardAt<NonlinearBushLoad3DTable>(model, 1).loadId1 == 12 &&
             cardAt<NonlinearBushLoad3DTable>(model, 1).loadId2 == 12));
    REQUIRE((cardAt<NonlinearBushLoad3DTable>(model, 2).setId == 8 &&
             cardAt<NonlinearBushLoad3DTable>(model, 2).loadId2 == 10));
    requireRoundTrip(model);

    REQUIRE(unresolvedIn(model) == 0);
    const Visited visited = visitedBy<NonlinearBushLoad3DTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"nodeA", "nodeB", "loadId1", "loadId2"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Node,
                                                            ReferenceTarget::BushingForceSet,
                                                            ReferenceTarget::BushingForceSet});
    // The two LBSH3DG and the two grids that no card defines are four unresolved references.
    Model missing = readBulk("NonlinearBushLoad3D;7;1;2;10;11\n");
    REQUIRE(unresolvedIn(missing) == 4);

    Renumbering::apply(model, RenumberSpace::BushingForceSet, Renumbering::offset(100));
    REQUIRE((cardAt<NonlinearBushLoad3DTable>(model, 0).loadId1 == 110 &&
             cardAt<NonlinearBushLoad3DTable>(model, 0).loadId2 == 111));
    REQUIRE(cardAt<NonlinearBushLoad3DTable>(model, 1).loadId1 == 112);
    REQUIRE((cardAt<NonlinearBushForceTablesTable>(model, 0).id == 110 &&
             cardAt<NonlinearBushForceTablesTable>(model, 2).id == 112));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<NonlinearBushLoad3DTable>(model, 0).nodeA == 1001 &&
             cardAt<NonlinearBushLoad3DTable>(model, 1).nodeB == 1004));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // Editing as the Python layer does.
    NonlinearBushLoad3DTable& table = model.table<NonlinearBushLoad3DTable>();
    NonlinearBushLoad3DCard edited = table.card(0);
    edited.nodeB = edited.nodeA;
    REQUIRE(NonlinearBushLoad3DFormat::check(edited).has_value());
    edited.nodeB = 1003;
    REQUIRE_FALSE(NonlinearBushLoad3DFormat::check(edited).has_value());
    table.store(0, edited);
    REQUIRE(cardAt<NonlinearBushLoad3DTable>(model, 0).nodeB == 1003);
}

TEST_CASE("an NLBSH3D refuses a missing field, equal grid points and a field outside the layout",
          "[model][cards][nlbsh3d]")
{
    expectInvalid("NonlinearBushLoad3D;0;1;2;10;11", "setId");
    expectInvalid("NonlinearBushLoad3D;7;0;2;10;11", "nodeA");
    expectInvalid("NonlinearBushLoad3D;7;1;0;10;11", "nodeB");
    expectInvalid("NonlinearBushLoad3D;7;1;1;10;11", "nodeB"); // two different points
    expectInvalid("NonlinearBushLoad3D;7;1;2;0;11", "loadId1");
    expectInvalid("NonlinearBushLoad3D;7;1;2;10", "loadId2"); // required
    expectInvalid("NonlinearBushLoad3D;7;1;2;10;0", "loadId2");
    expectInvalid("NonlinearBushLoad3D;7;1;2;10;x", "loadId2");
}

// --- PLTSURF --------------------------------------------------------------------------------------------------------

TEST_CASE("a PLTSURF has 3, 4, 6 or 8 grid points in the order of the shells", "[model][cards][pltsurf]")
{
    REQUIRE((PlotSurfaceTable::INFO.category == CardCategory::Element &&
             PlotSurfaceTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "PlotSurface;10;1;2;3\n"
        "PlotSurface;11;1;2;3;4\n"
        "PlotSurface;12;1;2;3;4;5;6\n"
        "PlotSurface;13;1;2;3;4;5;6;7;8\n");
    const std::array<std::size_t, 4> counts{3, 4, 6, 8};
    for (std::uint32_t row = 0; row < 4; ++row)
    {
        const PlotSurfaceCard card = cardAt<PlotSurfaceTable>(model, row);
        REQUIRE(card.id == 10 + static_cast<base::EntityId>(row));
        REQUIRE(card.gridCount() == counts[row]);
        for (std::size_t index = 0; index < PlotSurfaceCard::MAX_GRIDS; ++index)
        {
            const base::EntityId expected = index < counts[row] ? static_cast<base::EntityId>(index + 1) : 0;
            REQUIRE(card.grids[index] == expected);
        }
    }
    requireRoundTrip(model);

    // The eighth point is the first field of the second line, as the format draws it.
    REQUIRE(cardAt<PlotSurfaceTable>(model, 3).grids[7] == 8);
}

TEST_CASE("a PLTSURF is a plot element with an ID space of its own that names its grid points",
          "[model][cards][pltsurf]")
{
    // A structural element may have the ID of a plot element (a SET3 names the plot surface by ID: the PLOTEL case).
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "Node;3;;1.;1.;0.\n"
        "Node;4;;0.;1.;0.\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "RodProp;1;1;1.\n"
        "Rod2;10;1;1;2\n"
        "PlotSurface;10;1;2;3\n"
        "PlotSurface;11;1;2;3;4\n");
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(model.locate(base::IdSpace::Element, 10).has_value());
    REQUIRE(model.locateInType(typeIndexOf<PlotSurfaceTable>(), 10).has_value());
    REQUIRE_THROWS_AS(readBulk("PlotSurface;10;1;2;3\nPlotSurface;10;1;2;4"), base::DeckError);

    const Visited visited = visitedBy<PlotSurfaceTable>(model, 1);
    REQUIRE(visited.fields == std::vector<std::string>(4, "nodeId1"));
    REQUIRE(visited.targets == std::vector<ReferenceTarget>(4, ReferenceTarget::Node));
    Model missing = readBulk("Node;1;;0.;0.;0.\nPlotSurface;10;1;2;3\n");
    REQUIRE(unresolvedIn(missing) == 2);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<PlotSurfaceTable>(model, 1).grids ==
            std::array<base::EntityId, 8>{101, 102, 103, 104, 0, 0, 0, 0});
    REQUIRE(unresolvedIn(model) == 0);
    // The element space is not the plot elements': only the CROD moves.
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE(model.locate(base::IdSpace::Element, 1010).has_value());
    REQUIRE((cardAt<PlotSurfaceTable>(model, 0).id == 10 && cardAt<PlotSurfaceTable>(model, 1).id == 11));
    requireRoundTrip(model);

    // Editing as the Python layer does: the number of points follows the list.
    PlotSurfaceTable& table = model.table<PlotSurfaceTable>();
    PlotSurfaceCard edited = table.card(0);
    edited.grids = {101, 102, 103, 104, 0, 0, 0, 0};
    REQUIRE_FALSE(PlotSurfaceFormat::check(edited).has_value());
    table.store(0, edited);
    REQUIRE(cardAt<PlotSurfaceTable>(model, 0).gridCount() == 4);
    edited.grids = {101, 102, 103, 104, 105, 0, 0, 0};
    REQUIRE(PlotSurfaceFormat::check(edited).has_value()); // five points
}

TEST_CASE("a PLTSURF refuses a number of points the format does not give, a hole and a repeated point",
          "[model][cards][pltsurf]")
{
    expectInvalid("PlotSurface;0;1;2;3", "id");
    expectInvalid("PlotSurface;10", "nodeId1");
    expectInvalid("PlotSurface;10;1;2", "nodeId1");
    expectInvalid("PlotSurface;10;1;2;3;4;5", "nodeId1");
    expectInvalid("PlotSurface;10;1;2;3;4;5;6;7", "nodeId1");
    expectInvalid("PlotSurface;10;1;2;;4", "nodeId3"); // a blank in the list
    expectInvalid("PlotSurface;10;;2;3;4", "nodeId1");
    expectInvalid("PlotSurface;10;1;2;3;3", "nodeId4"); // repeated
    expectInvalid("PlotSurface;10;1;2;3;-4", "nodeId4");
    expectInvalid("PlotSurface;10;1;2;x", "nodeId3");
}
