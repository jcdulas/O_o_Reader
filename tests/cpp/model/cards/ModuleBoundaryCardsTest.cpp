// The multi-module cards of the first part of the family: MDBULK, MDLABEL, MDBNDRY, MDEXCLD,
// MDCONCT, MDFSET, MDSSET, EXCLUDE, MDDMIG, MDBKSYS, MDBOLT and MDBUSH. Per card: the fields in their format reference
// positions, the defaults, the references the card lists (only what lives in module 0, the main Bulk Data, is a card of
// the model), the renumbering of what it names, the refused values and the round trip through the three formats. The
// generic tests of CardRoundTripTest.cpp read, write and copy every sample; this file covers what each card means.

#include "oor/model/cards/nodes/moduleBolt/ModuleBoltTable.hpp"
#include "oor/model/cards/nodes/moduleBoundaryNodes/ModuleBoundaryNodesTable.hpp"
#include "oor/model/cards/nodes/moduleBrakeSystem/ModuleBrakeSystemTable.hpp"
#include "oor/model/cards/nodes/moduleBulkOptions/ModuleBulkOptionsTable.hpp"
#include "oor/model/cards/nodes/moduleBushing/ModuleBushingTable.hpp"
#include "oor/model/cards/nodes/moduleConnection/ModuleConnectionTable.hpp"
#include "oor/model/cards/nodes/moduleDirectMatrix/ModuleDirectMatrixTable.hpp"
#include "oor/model/cards/nodes/moduleEntryExclusion/ModuleEntryExclusionTable.hpp"
#include "oor/model/cards/nodes/moduleExcludedNodes/ModuleExcludedNodesTable.hpp"
#include "oor/model/cards/nodes/moduleFluidSet/ModuleFluidSetTable.hpp"
#include "oor/model/cards/nodes/moduleLabel/ModuleLabelTable.hpp"
#include "oor/model/cards/nodes/moduleStructureSet/ModuleStructureSetTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

IdRun run(base::EntityId first, base::EntityId last)
{
    return IdRun{first, last};
}

// Node records for the numbers (all at the origin: only the IDs matter to these tests).
std::string gridCards(std::initializer_list<int> ids)
{
    std::string text;
    for (const int id : ids)
        text += "Node;" + std::to_string(id) + ";;0.;0.;0.\n";
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

// The field a refused card names, "" when it is fine.
std::string_view fieldOf(const std::optional<CardProblem>& problem)
{
    return problem ? std::string_view(problem->field) : std::string_view();
}

} // namespace

// --- MDBULK ---------------------------------------------------------------------------------------------------------

TEST_CASE("an MDBULK types a module and resolves its defaults", "[model][cards][modules]")
{
    const Model model = readBulk(
        "ModuleBulkOptions;26;;;Automatic;0.003\n"
        "ModuleBulkOptions;15\n"
        "ModuleBulkOptions;16;Repeat;26\n"
        "ModuleBulkOptions;17;Mirror;26;Manual\n"
        "ModuleBulkOptions;18;Move;26\n"
        "ModuleBulkOptions;19;External\n"
        "ModuleBulkOptions;20;ExternalBinaryResults;;;;30\n"
        "ModuleBulkOptions;21;ExternalMatrixFile;;;0.5;31");
    const ModuleBulkOptionsCard example = cardAt<ModuleBulkOptionsTable>(model, 0);
    REQUIRE(example.module == ModuleOrAll::module(26));
    REQUIRE(example.kind == ModuleBulkOptionsType::Primary);
    REQUIRE(example.method == ModuleBulkOptionsMethod::Auto);
    REQUIRE(example.tolerance == 3.0e-3);
    REQUIRE((example.sourceModuleId == 0 && example.unitNumber == 0));

    const ModuleBulkOptionsCard blank = cardAt<ModuleBulkOptionsTable>(model, 1);
    REQUIRE(blank.kind == ModuleBulkOptionsType::Primary);    // the default of TYPE
    REQUIRE(blank.method == ModuleBulkOptionsMethod::Manual); // the default of METHOD
    REQUIRE(blank.tolerance == 0.0);                          // 0.0 takes the value of PARAM,CONFAC

    REQUIRE(cardAt<ModuleBulkOptionsTable>(model, 2).kind == ModuleBulkOptionsType::Repeat);
    REQUIRE(cardAt<ModuleBulkOptionsTable>(model, 2).sourceModuleId == 26);
    REQUIRE(cardAt<ModuleBulkOptionsTable>(model, 3).kind == ModuleBulkOptionsType::Mirror); // exact neutral keyword
    REQUIRE(cardAt<ModuleBulkOptionsTable>(model, 3).method == ModuleBulkOptionsMethod::Manual);
    REQUIRE(cardAt<ModuleBulkOptionsTable>(model, 4).kind == ModuleBulkOptionsType::Move);
    REQUIRE(cardAt<ModuleBulkOptionsTable>(model, 5).kind == ModuleBulkOptionsType::External);
    REQUIRE(cardAt<ModuleBulkOptionsTable>(model, 6).kind == ModuleBulkOptionsType::ExternalBinaryResults);
    REQUIRE(cardAt<ModuleBulkOptionsTable>(model, 6).unitNumber == 30);
    const ModuleBulkOptionsCard last = cardAt<ModuleBulkOptionsTable>(model, 7);
    REQUIRE((last.kind == ModuleBulkOptionsType::ExternalMatrixFile && last.tolerance == 0.5 && last.unitNumber == 31));
    requireRoundTrip(model);
}

TEST_CASE("an MDBULK with ALL is one card indexed as 0 and has no type", "[model][cards][modules]")
{
    const Model model = readBulk("ModuleBulkOptions;All;;;Automatic;2.E-4\nModuleBulkOptions;5");
    // The ID index keeps 0 for ALL, which no module number has: a second MDBULK,ALL is a duplicate.
    REQUIRE(ModuleBulkOptionsTable::INFO.idSpace == base::IdSpace::PerCardType);
    REQUIRE(cardAt<ModuleBulkOptionsTable>(model, 0).module.all);
    REQUIRE(cardAt<ModuleBulkOptionsTable>(model, 0).method == ModuleBulkOptionsMethod::Auto);
    REQUIRE(cardAt<ModuleBulkOptionsTable>(model, 0).tolerance == 2.0e-4);
    REQUIRE(model.findTable<ModuleBulkOptionsTable>()->idAtRow(0) == 0);
    REQUIRE(model.findTable<ModuleBulkOptionsTable>()->idAtRow(1) == 5);
    REQUIRE(ModuleBulkOptionsTable::cardId(cardAt<ModuleBulkOptionsTable>(model, 0)) == 0);
    REQUIRE(cardAt<ModuleBulkOptionsTable>(readBulk("ModuleBulkOptions;All")).module.all); // exact neutral keyword
    REQUIRE(ModuleOrAll::fromId(0).all);
    REQUIRE(ModuleOrAll::fromId(7) == ModuleOrAll::module(7));
    // The two ALL cards are one ID: refused in a strict deck.
    REQUIRE_THROWS_AS(readBulk("ModuleBulkOptions;All\nModuleBulkOptions;All"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ModuleBulkOptions;5\nModuleBulkOptions;5"), base::DeckError);
    requireRoundTrip(readBulk("ModuleBulkOptions;All;;;Automatic;2.E-4\nModuleBulkOptions;5"));
}

TEST_CASE("an MDBULK names no card: a module is a BEGIN MODULE section", "[model][cards][modules]")
{
    Model model = readBulk("ModuleBulkOptions;28;Repeat;26\nModuleBulkOptions;26");
    REQUIRE(referencesOf<ModuleBulkOptionsTable>(model, 0).fields.empty());
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("an MDBULK refuses what its format does not allow", "[model][cards][modules]")
{
    expectInvalid("ModuleBulkOptions", "id");
    expectInvalid("ModuleBulkOptions;0", "id"); // module 0 is the main Bulk Data: not a module to type
    expectInvalid("ModuleBulkOptions;-3", "id");
    expectInvalid("ModuleBulkOptions;5;PRIMARY1", "kind");
    expectInvalid("ModuleBulkOptions;5;;;MAYBE", "method");
    expectInvalid("ModuleBulkOptions;5;;-1", "sourceModuleId");
    expectInvalid("ModuleBulkOptions;5;;;;abc", "tolerance");
    expectInvalid("ModuleBulkOptions;5;;;;;-1", "unitNumber");
    // With ALL the TYPE and RMODID are blank.
    expectInvalid("ModuleBulkOptions;All;Repeat", "kind");
    expectInvalid("ModuleBulkOptions;All;;3", "sourceModuleId");
    expectInvalid("ModuleBulkOptions;X", "id");
}

// --- MDLABEL --------------------------------------------------------------------------------------------------------

TEST_CASE("an MDLABEL is one per module and its label has the rules of a text over fields", "[model][cards][modules]")
{
    REQUIRE_THROWS_AS(readBulk("ModuleLabel;5;A\nModuleLabel;5;B"), base::DeckError); // one per module
    expectInvalid("ModuleLabel;-1;Wing", "id");
    expectInvalid("ModuleLabel;5", "entryLabel");
    ModuleLabelCard card;
    card.id = 5;
    card.entryLabel = std::string(157, 'A'); // long labels remain one record value
    REQUIRE_FALSE(ModuleLabelFormat::check(card).has_value());
    card.entryLabel = std::string(56, 'A');
    REQUIRE_FALSE(ModuleLabelFormat::check(card).has_value());
    for (const char* label : {"two, words", " leading", "trailing ", "cost $5", "tab\there"})
    {
        card.entryLabel = label;
        INFO(label);
        REQUIRE(ModuleLabelFormat::check(card).has_value() ==
                (std::string_view(label).find('\t') != std::string_view::npos));
    }
    card.entryLabel = "Wing";
    card.id = -2;
    REQUIRE(fieldOf(ModuleLabelFormat::check(card)) == "id");
}

// --- MDBNDRY and MDEXCLD --------------------------------------------------------------------------------------------

TEST_CASE("an MDBNDRY lists the boundary points of a module and the module it is searched against",
          "[model][cards][modules]")
{
    Model model = readBulk(gridCards({10, 20, 30, 31, 32, 33, 34, 35}) +
                           "ModuleBoundaryNodes;350;6;10;20;30;40\n"
                           "ModuleBoundaryNodes;350;All;10;20;30;Through;35\n"
                           "ModuleBoundaryNodes;;;10\n"
                           "ModuleBoundaryNodes;0;0;20;Through;30\n"
                           "ModuleBoundaryNodes;7101;7102;1;2;3;4;5;6;7;8;9;Through;14");
    const ModuleBoundaryNodesCard example = cardAt<ModuleBoundaryNodesTable>(model, 0);
    REQUIRE(example.moduleIdA == 350);
    REQUIRE(example.otherModule == ModuleOrAll::module(6));
    REQUIRE(example.nodes == std::vector<IdRun>{run(10, 10), run(20, 20), run(30, 30), run(40, 40)});
    const ModuleBoundaryNodesCard all = cardAt<ModuleBoundaryNodesTable>(model, 1);
    REQUIRE(all.otherModule.all);
    REQUIRE(all.nodes == std::vector<IdRun>{run(10, 10), run(20, 20), run(30, 35)});
    const ModuleBoundaryNodesCard main = cardAt<ModuleBoundaryNodesTable>(model, 2);
    REQUIRE((main.moduleIdA == 0 && main.otherModule.all)); // a blank MIDA is 0 and a blank MIDB is ALL
    REQUIRE(cardAt<ModuleBoundaryNodesTable>(model, 3).otherModule == ModuleOrAll::module(0)); // 0 is a module, not ALL
    const ModuleBoundaryNodesCard lines = cardAt<ModuleBoundaryNodesTable>(model, 4);
    REQUIRE(lines.nodes.size() == 9);
    REQUIRE(lines.nodes.back() == run(9, 14)); // a THRU range on a continuation line
    requireRoundTrip(model);

    // The points are those of module MIDA: grid points of this model only when MIDA is 0.
    REQUIRE(referencesOf<ModuleBoundaryNodesTable>(model, 0).fields.empty());
    REQUIRE(referencesOf<ModuleBoundaryNodesTable>(model, 1).fields.empty());
    const RecordingVisitor single = referencesOf<ModuleBoundaryNodesTable>(model, 2);
    REQUIRE(single.fields == std::vector<std::string>{"nodesA"});
    REQUIRE(single.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node});
    const RecordingVisitor range = referencesOf<ModuleBoundaryNodesTable>(model, 3);
    REQUIRE(range.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::NodeInRange));
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("ModuleBoundaryNodes;;;10;11\nModuleBoundaryNodes;3;;10;11");
    REQUIRE(unresolvedIn(lacking) == 2); // the two points of module 0; module 3 is not looked up

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<ModuleBoundaryNodesTable>(model, 2).nodes == std::vector<IdRun>{run(110, 110)});
    REQUIRE(cardAt<ModuleBoundaryNodesTable>(model, 3).nodes == std::vector<IdRun>{run(120, 130)});
    REQUIRE(cardAt<ModuleBoundaryNodesTable>(model, 0).nodes.front() == run(10, 10)); // module 350: left alone
}

TEST_CASE("an MDBNDRY refuses what its format does not allow", "[model][cards][modules]")
{
    expectInvalid("ModuleBoundaryNodes;-1;All;10", "moduleIdA");
    expectInvalid("ModuleBoundaryNodes;400;-2;10", "otherModule");
    expectInvalid("ModuleBoundaryNodes;400;ABC;10", "otherModule");
    expectInvalid("ModuleBoundaryNodes;400;All", "nodesA");
    expectInvalid("ModuleBoundaryNodes;400;All;0", "nodesA");
    expectInvalid("ModuleBoundaryNodes;400;All;10;X", "nodesA");
    expectInvalid("ModuleBoundaryNodes;400;All;10;Through;5", "nodesA");
    expectInvalid("ModuleBoundaryNodes;400;All;Through;10", "Through");
}

TEST_CASE("an MDEXCLD lists the points a module does not connect to another one", "[model][cards][modules]")
{
    Model model = readBulk(gridCards({10, 20, 30, 31, 32, 33, 34, 35}) +
                           "ModuleExcludedNodes;350;6;10;20;30;40\nModuleExcludedNodes;350;All;10;20;30;Through;"
                           "35\nModuleExcludedNodes;;;10\nModuleExcludedNodes;0;0;20;Through;30");
    REQUIRE(cardAt<ModuleExcludedNodesTable>(model, 0).moduleIdA == 350);
    REQUIRE(cardAt<ModuleExcludedNodesTable>(model, 0).otherModule == ModuleOrAll::module(6));
    REQUIRE(cardAt<ModuleExcludedNodesTable>(model, 0).nodes ==
            std::vector<IdRun>{run(10, 10), run(20, 20), run(30, 30), run(40, 40)});
    REQUIRE(cardAt<ModuleExcludedNodesTable>(model, 1).otherModule.all);
    REQUIRE(cardAt<ModuleExcludedNodesTable>(model, 1).nodes ==
            std::vector<IdRun>{run(10, 10), run(20, 20), run(30, 35)});
    REQUIRE((cardAt<ModuleExcludedNodesTable>(model, 2).moduleIdA == 0 &&
             cardAt<ModuleExcludedNodesTable>(model, 2).otherModule.all));
    REQUIRE(cardAt<ModuleExcludedNodesTable>(model, 3).otherModule == ModuleOrAll::module(0));
    requireRoundTrip(model);

    REQUIRE(referencesOf<ModuleExcludedNodesTable>(model, 0).fields.empty());
    REQUIRE(referencesOf<ModuleExcludedNodesTable>(model, 2).targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Node});
    REQUIRE(referencesOf<ModuleExcludedNodesTable>(model, 3).targets ==
            std::vector<ReferenceTarget>(2, ReferenceTarget::NodeInRange));
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<ModuleExcludedNodesTable>(model, 2).nodes == std::vector<IdRun>{run(110, 110)});
    REQUIRE(cardAt<ModuleExcludedNodesTable>(model, 1).nodes.front() == run(10, 10));

    expectInvalid("ModuleExcludedNodes;-1;All;10", "moduleIdA");
    expectInvalid("ModuleExcludedNodes;400;-2;10", "otherModule");
    expectInvalid("ModuleExcludedNodes;400;All", "nodesA");
    expectInvalid("ModuleExcludedNodes;400;All;10;Through;5", "nodesA");
    expectInvalid("ModuleExcludedNodes;400;All;Through;10", "Through");
}

// --- MDFSET and MDSSET ----------------------------------------------------------------------------------------------

TEST_CASE("an MDFSET and an MDSSET list the points or elements of a module with THRU ranges", "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleFluidSet;12;41;52;73;84;26;27;28;29\n"
        "ModuleFluidSet;12;35;38;Through;48;63;Through;72;23;55\n"
        "ModuleFluidSet;0;5\n"
        "ModuleStructureSet;12;41;52;73;84;26;27;28;29\n"
        "ModuleStructureSet;12;35;38;Through;48;63;Through;72;23;55\n"
        "ModuleStructureSet;;5;Through;9");
    const ModuleFluidSetCard singles = cardAt<ModuleFluidSetTable>(model, 0);
    REQUIRE(singles.moduleId == 12);
    REQUIRE(singles.ids.size() == 8);
    REQUIRE((singles.ids.front() == run(41, 41) && singles.ids.back() == run(29, 29)));
    REQUIRE(cardAt<ModuleFluidSetTable>(model, 1).ids ==
            std::vector<IdRun>{run(35, 35), run(38, 48), run(63, 72), run(23, 23), run(55, 55)});
    REQUIRE(cardAt<ModuleFluidSetTable>(model, 2).moduleId == 0);
    REQUIRE(cardAt<ModuleStructureSetTable>(model, 0).ids.size() == 8);
    REQUIRE(cardAt<ModuleStructureSetTable>(model, 1).ids ==
            std::vector<IdRun>{run(35, 35), run(38, 48), run(63, 72), run(23, 23), run(55, 55)});
    REQUIRE(cardAt<ModuleStructureSetTable>(model, 2).moduleId == 0); // a blank MODID is the main Bulk Data
    REQUIRE(cardAt<ModuleStructureSetTable>(model, 2).ids == std::vector<IdRun>{run(5, 9)});
    requireRoundTrip(model);

    // The IDs are fluid or structural points or elements and the card does not say which: nothing is looked up, even
    // for module 0.
    REQUIRE(referencesOf<ModuleFluidSetTable>(model, 2).fields.empty());
    REQUIRE(referencesOf<ModuleStructureSetTable>(model, 2).fields.empty());
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(ModuleFluidSetTable::INFO.idSpace == base::IdSpace::None);
    REQUIRE(ModuleStructureSetTable::INFO.idSpace == base::IdSpace::None);
}

TEST_CASE("an MDFSET and an MDSSET refuse what their format does not allow", "[model][cards][modules]")
{
    for (const std::string name : {"ModuleFluidSet", "ModuleStructureSet"})
    {
        expectInvalid(name + ";-1;5", "moduleId");
        expectInvalid(name + ";3", "ids");
        expectInvalid(name + ";3;0", "ids");
        expectInvalid(name + ";3;10;Through;5", "ids");
        expectInvalid(name + ";3;Through;5", "Through");
        expectInvalid(name + ";3;5;x", "ids");
    }
}

// --- EXCLUDE --------------------------------------------------------------------------------------------------------

TEST_CASE("an EXCLUDE lists the names of the entries a copied module ignores", "[model][cards][modules]")
{
    const Model model = readBulk(
        "ModuleEntryExclusion;SinglePointConstr;PointForceLoad\nModuleEntryExclusion;NodeTemperatureLoad;ShellTria3;"
        "ShellQuad4;IsoMat;SinglePointConstrList;BeamProp;Beam2;ShellProp;PointMassInertia;"
        "SpringProp");
    REQUIRE(cardAt<ModuleEntryExclusionTable>(model, 0).entries ==
            std::vector<std::string>{"SinglePointConstr", "PointForceLoad"});
    const ModuleEntryExclusionCard many = cardAt<ModuleEntryExclusionTable>(model, 1);
    REQUIRE(many.entries.size() == 10);
    REQUIRE(many.entries[1] == "ShellTria3");
    REQUIRE(many.entries.back() == "SpringProp");
    REQUIRE(ModuleEntryExclusionTable::INFO.idSpace == base::IdSpace::None);
    requireRoundTrip(model);
    Model copy = readBulk("ModuleEntryExclusion;SinglePointConstr");
    REQUIRE(referencesOf<ModuleEntryExclusionTable>(copy).fields.empty());
}

TEST_CASE("an EXCLUDE refuses an empty list, Node and a name that is not a name", "[model][cards][modules]")
{
    expectInvalid("ModuleEntryExclusion", "entry1");
    expectInvalid("ModuleEntryExclusion;Node", "entry"); // a fatal error
    expectInvalid("ModuleEntryExclusion;SinglePointConstr;Node", "entry");
    ModuleEntryExclusionCard card;
    card.entries = {"SinglePointConstr", "FutureEntryType"};
    REQUIRE_FALSE(ModuleEntryExclusionFormat::check(card).has_value());
    card.entries = {"SP C"};
    REQUIRE_FALSE(ModuleEntryExclusionFormat::check(card).has_value());
    card.entries = {};
    REQUIRE(fieldOf(ModuleEntryExclusionFormat::check(card)) == "entry1");
    card.entries = {"SinglePointConstr", "PointForceLoad"};
    REQUIRE_FALSE(ModuleEntryExclusionFormat::check(card).has_value());
}

// --- MDCONCT --------------------------------------------------------------------------------------------------------

TEST_CASE("an MDCONCT has two layouts of the location and pairs of module and point", "[model][cards][modules]")
{
    const Model model = readBulk(
        "ModuleConnection;2201;;0.45;1.5;;42.75;13.25;;;10;3300;20;3400\n"
        "ModuleConnection;2202;RigidStarReleased;0.45;;3555;;;;;10;;20\n"
        "ModuleConnection;2203;Merge;0.01;1.;;2.;3.;;5;0;0;10;7\n"
        "ModuleConnection;2204;Rigid;;;100;;;11;;0;5;11;6;12;7\n"
        "ModuleConnection;2205");
    // A point X, Y, Z with the defaults of TYPE and CID.
    const ModuleConnectionCard example = cardAt<ModuleConnectionTable>(model, 0);
    REQUIRE(example.id == 2201);
    REQUIRE(example.type == ModuleConnectionType::Mrbe2);
    REQUIRE(example.tolerance == 0.45);
    REQUIRE((example.xCoord == 1.5 && example.yCoord == 42.75 && example.zCoord == 13.25));
    REQUIRE((example.frameId == 0 && example.locationNode == 0 && example.moduleId == 0));
    REQUIRE_FALSE(example.usesGrid());
    REQUIRE(example.points == std::vector<ModuleId>{{10, 3300}, {20, 3400}});
    // An integer in field 4 is the GRID of the other layout; a pair with no GID is every grid point.
    const ModuleConnectionCard gridCard = cardAt<ModuleConnectionTable>(model, 1);
    REQUIRE(gridCard.type == ModuleConnectionType::Rrbe2);
    REQUIRE((gridCard.locationNode == 3555 && gridCard.moduleId == 0 && gridCard.usesGrid()));
    REQUIRE((gridCard.xCoord == 0.0 && gridCard.yCoord == 0.0 && gridCard.zCoord == 0.0));
    REQUIRE(gridCard.points == std::vector<ModuleId>{{10, 0}, {20, 0}});
    // The coordinate system, and a pair of the main Bulk Data with every grid point.
    const ModuleConnectionCard moved = cardAt<ModuleConnectionTable>(model, 2);
    REQUIRE(moved.type == ModuleConnectionType::Merge);
    REQUIRE((moved.xCoord == 1.0 && moved.yCoord == 2.0 && moved.zCoord == 3.0 && moved.frameId == 5));
    REQUIRE(moved.points == std::vector<ModuleId>{{0, 0}, {10, 7}});
    // The GRID of a module and the MODID in field 7.
    const ModuleConnectionCard inModule = cardAt<ModuleConnectionTable>(model, 3);
    REQUIRE(inModule.type == ModuleConnectionType::Rigid);
    REQUIRE((inModule.locationNode == 100 && inModule.moduleId == 11 && inModule.tolerance == 0.0));
    REQUIRE(inModule.points == std::vector<ModuleId>{{0, 5}, {11, 6}, {12, 7}});
    const ModuleConnectionCard minimal = cardAt<ModuleConnectionTable>(model, 4);
    REQUIRE((minimal.id == 2205 && minimal.points.empty())); // the continuation line is optional
    REQUIRE(ModuleConnectionTable::INFO.idSpace == base::IdSpace::PerCardType);
    requireRoundTrip(model);
}

TEST_CASE("an MDCONCT reads every type and looks up what the main Bulk Data holds", "[model][cards][modules]")
{
    const char* const words[] = {"Merge", "Rigid", "RigidStarReleased", "ModuleRigidStar"};
    for (std::size_t index = 0; index < 4; ++index)
    {
        const Model model = readBulk(std::string("ModuleConnection;5;") + words[index]);
        REQUIRE(static_cast<std::size_t>(cardAt<ModuleConnectionTable>(model).type) == index);
    }
    Model model = readBulk(
        "Node;50;;0.;0.;0.\n"
        "Node;5;;0.;0.;0.\n"
        "Node;7;;0.;0.;0.\n"
        "ScalarPoint;6\n"
        "RectFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "ModuleConnection;128;;;;50;;;;;0;5;0;6;7;9\n"
        "ModuleConnection;129;;0.1;0.;;0.;0.;;3;0;7;10;70\n"
        "ModuleConnection;130;;;;50;;;11;;0;5");
    // Module 0: the GRID (with a MODID of 0) and the pairs; a coordinate system is one of module 0.
    const RecordingVisitor grid = referencesOf<ModuleConnectionTable>(model, 0);
    REQUIRE(grid.fields == std::vector<std::string>{"locationNode", "memberNodeId",
                                                    "memberNodeId"}); // the pair of module 7 is not looked up
    REQUIRE(grid.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::ScalarOrGridPoint,
                                                         ReferenceTarget::ScalarOrGridPoint});
    const RecordingVisitor point = referencesOf<ModuleConnectionTable>(model, 1);
    REQUIRE(point.fields ==
            std::vector<std::string>{"frameId", "memberNodeId"}); // the pair of module 10 is not looked up
    REQUIRE(point.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Coordinate, ReferenceTarget::ScalarOrGridPoint});
    REQUIRE(referencesOf<ModuleConnectionTable>(model, 2).fields ==
            std::vector<std::string>{"memberNodeId"}); // GRID in module 11
    REQUIRE(unresolvedIn(model) == 0);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<ModuleConnectionTable>(model, 0).locationNode == 150);
    REQUIRE(cardAt<ModuleConnectionTable>(model, 0).points == std::vector<ModuleId>{{0, 105}, {0, 106}, {7, 9}});
    REQUIRE(cardAt<ModuleConnectionTable>(model, 1).points == std::vector<ModuleId>{{0, 107}, {10, 70}});
    REQUIRE(cardAt<ModuleConnectionTable>(model, 2).locationNode == 50); // a GRID of module 11: left alone
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<ModuleConnectionTable>(model, 1).frameId == 13);
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("an MDCONCT refuses what its format does not allow", "[model][cards][modules]")
{
    expectInvalid("ModuleConnection;0", "id");
    expectInvalid("ModuleConnection;-4", "id");
    expectInvalid("ModuleConnection;5;SOFT", "kind");
    expectInvalid("ModuleConnection;5;;;x", "xCoord");
    expectInvalid("ModuleConnection;5;;;;;;;;-1", "frameId");
    expectInvalid("ModuleConnection;5;;;;;;;;;-1;3", "memberModuleId");
    expectInvalid("ModuleConnection;5;;;;;;;;;10;-3", "memberNodeId");
    expectInvalid("ModuleConnection;5;;;;100;1.");   // a GRID has no Y
    expectInvalid("ModuleConnection;5;;;;100;;;;3"); // and no CID
    expectInvalid("ModuleConnection;5;;;1.;;;;3");   // a MODID needs a GRID
    REQUIRE_THROWS_AS(readBulk("ModuleConnection;5\nModuleConnection;5"), base::DeckError);
    ModuleConnectionCard card;
    card.id = 5;
    card.locationNode = 100;
    card.xCoord = 1.0;
    REQUIRE(fieldOf(ModuleConnectionFormat::check(card)) == "locationNode"); // both layouts together
    card.xCoord = 0.0;
    card.frameId = 2;
    REQUIRE(fieldOf(ModuleConnectionFormat::check(card)) == "locationNode");
    card.frameId = 0;
    card.locationNode = 0;
    card.moduleId = 3;
    REQUIRE(fieldOf(ModuleConnectionFormat::check(card)) == "locationNode");
}

// --- MDDMIG ---------------------------------------------------------------------------------------------------------

TEST_CASE("an MDDMIG has a header and columns whose terms carry a module", "[model][cards][modules]")
{
    const Model model = readBulk(
        "ModuleDirectMatrix;KJNT;0;1;3;4\n"
        "ModuleDirectMatrix;KJNT;13;47;1;;;;36;2;3;3.5E+5;2500.;36;2;4;6.5E+9;0.;52;44;;2.;0.\n"
        "ModuleDirectMatrix;KMAT;0;6;1\n"
        "ModuleDirectMatrix;KMAT;0;5;1;;;;0;5;1;1.5;;;6;3;-0.5\n"
        "ModuleDirectMatrix;RECT;0;9;2;;;3\n"
        "ModuleDirectMatrix;RECT;3;;;;;;10;5;1;2.5");
    const ModuleDirectMatrixCard header = cardAt<ModuleDirectMatrixTable>(model, 0);
    REQUIRE(header.isHeader());
    REQUIRE((header.entryName == "KJNT" && header.matrixShape == 1 && header.inputType == 3 && header.outputType == 4));
    const ModuleDirectMatrixCard column = cardAt<ModuleDirectMatrixTable>(model, 1);
    REQUIRE_FALSE(column.isHeader());
    REQUIRE((column.columnModuleId == 13 && column.nodeJ == 47 && column.componentsJ == 1));
    REQUIRE(column.imaginary);
    REQUIRE(column.terms.size() == 3);
    REQUIRE(column.terms[0] == ModuleDirectMatrixTerm{36, 2, 3, 3.5e5, 2.5e3});
    REQUIRE(column.terms[1] == ModuleDirectMatrixTerm{36, 2, 4, 6.5e9, 0.0});
    REQUIRE(column.terms[2] == ModuleDirectMatrixTerm{52, 44, 0, 2.0, 0.0}); // a blank component is 0
    // The default of the header is a symmetric, real matrix.
    REQUIRE((cardAt<ModuleDirectMatrixTable>(model, 2).matrixShape == 6 &&
             cardAt<ModuleDirectMatrixTable>(model, 2).inputType == 1));
    // A column of module 0 has its terms (a card with MODJ 0 and none is a header); a blank module is 0.
    const ModuleDirectMatrixCard main = cardAt<ModuleDirectMatrixTable>(model, 3);
    REQUIRE_FALSE(main.isHeader());
    REQUIRE((main.columnModuleId == 0 && main.nodeJ == 5 && main.componentsJ == 1 && !main.imaginary));
    REQUIRE(main.terms == std::vector<ModuleDirectMatrixTerm>{{0, 5, 1, 1.5, 0.0}, {0, 6, 3, -0.5, 0.0}});
    const ModuleDirectMatrixCard rectangular = cardAt<ModuleDirectMatrixTable>(model, 4);
    REQUIRE((rectangular.matrixShape == 9 && rectangular.inputType == 2 && rectangular.columnCount == 3));
    const ModuleDirectMatrixCard numbered = cardAt<ModuleDirectMatrixTable>(model, 5); // MODJ is a column number here
    REQUIRE((numbered.columnModuleId == 3 && numbered.nodeJ == 0 && numbered.terms.size() == 1));
    REQUIRE(ModuleDirectMatrixTable::INFO.idSpace == base::IdSpace::None);
    REQUIRE(ModuleDirectMatrixTable::INFO.category == CardCategory::Matrix);
    requireRoundTrip(model);
}

TEST_CASE("an MDDMIG looks up the points of module 0 and no other", "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleDirectMatrix;KMAT;0;5;1;;;;0;5;1;1.5;;;6;3;-0.5;;9;7;3;2.\n"
        "ModuleDirectMatrix;KJNT;13;47;1;;;;36;2;3;4.5\n"
        "ModuleDirectMatrix;RECT;3;;;;;;0;5;1;2.5\n" +
        gridCards({5, 6}));
    const RecordingVisitor main = referencesOf<ModuleDirectMatrixTable>(model, 0);
    REQUIRE(main.fields == std::vector<std::string>{"nodeJ", "node", "node"}); // the term of module 9 is not looked up
    REQUIRE(main.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::ScalarOrGridPoint));
    REQUIRE(referencesOf<ModuleDirectMatrixTable>(model, 1).fields.empty()); // module 13: nothing
    REQUIRE(referencesOf<ModuleDirectMatrixTable>(model, 2).fields ==
            std::vector<std::string>{"node"}); // MODJ is a column number
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("ModuleDirectMatrix;KMAT;0;5;1;;;;0;5;1;1.5;;;6;3;-0.5;;9;7;3;2.");
    // A point that is not there is not reported: the target may name a scalar point that no card defines (as for a
    // DMIG).
    REQUIRE(unresolvedFields<ModuleDirectMatrixTable>(lacking, 0).empty());

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<ModuleDirectMatrixTable>(model, 0).nodeJ == 105);
    REQUIRE(cardAt<ModuleDirectMatrixTable>(model, 0).terms[0].point == 105);
    REQUIRE(cardAt<ModuleDirectMatrixTable>(model, 0).terms[1].point == 106);
    REQUIRE(cardAt<ModuleDirectMatrixTable>(model, 0).terms[2].point == 7); // module 9: left alone
    REQUIRE(cardAt<ModuleDirectMatrixTable>(model, 1).nodeJ == 47);         // module 13: left alone
    REQUIRE(cardAt<ModuleDirectMatrixTable>(model, 2).terms[0].point == 105);
}

TEST_CASE("an MDDMIG refuses what its format does not allow", "[model][cards][modules]")
{
    const Model named = readBulk("ModuleDirectMatrix;1BAD;0;1;3\nModuleDirectMatrix;Long.Mixed.MatrixName;0;1;3");
    REQUIRE(cardAt<ModuleDirectMatrixTable>(named, 0).entryName == "1BAD");
    REQUIRE(cardAt<ModuleDirectMatrixTable>(named, 1).entryName == "Long.Mixed.MatrixName");
    requireRoundTrip(named);
    expectInvalid("ModuleDirectMatrix;K;0;1", "inputType");
    expectInvalid("ModuleDirectMatrix;K;0;3;1", "matrixShape");
    expectInvalid("ModuleDirectMatrix;K;0;1;5", "inputType");
    expectInvalid("ModuleDirectMatrix;K;0;1;3;7", "outputType");
    expectInvalid("ModuleDirectMatrix;K;0;1;3;;-1", "polarFlag");
    expectInvalid("ModuleDirectMatrix;K;0;1;3;;;-2", "columnCount");
    expectInvalid("ModuleDirectMatrix;K;-1;;;;;;0;5;1;1.", "columnModuleId");
    expectInvalid("ModuleDirectMatrix;K;5;1;7;;;;0;5;1;1.", "componentsJ");
    expectInvalid("ModuleDirectMatrix;K;5;;;;;;0;0;1;1.", "node");
    expectInvalid("ModuleDirectMatrix;K;5;;;;;;0;5;9;1.", "components");
    expectInvalid("ModuleDirectMatrix;K;5;;;;;;-1;5;1;1.", "moduleId");
    expectInvalid("ModuleDirectMatrix;K;5;1;1;9"); // fields 5 to 8 of a column line are blank
    ModuleDirectMatrixCard card;
    card.entryName = "K";
    card.columnModuleId = 4;
    card.terms = {ModuleDirectMatrixTerm{0, 5, 1, 1.0, 2.0}};
    REQUIRE(fieldOf(ModuleDirectMatrixFormat::check(card)) == "imaginaryValue"); // a real matrix has no imaginary part
    card.imaginary = true;
    REQUIRE_FALSE(ModuleDirectMatrixFormat::check(card).has_value());
    card.matrixShape = 1;
    REQUIRE(fieldOf(ModuleDirectMatrixFormat::check(card)) == "matrixShape"); // the header fields stay with the header
    card = ModuleDirectMatrixCard{};
    card.entryName = "K";
    card.nodeJ = 5; // MODJ 0 and no terms: a header, which has no GJ
    REQUIRE(fieldOf(ModuleDirectMatrixFormat::check(card)) == "nodeJ");
}

// --- MDBKSYS --------------------------------------------------------------------------------------------------------

TEST_CASE("an MDBKSYS has a header and a line for each brake system", "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleBrakeSystem;610;0.3;0;Yes;;DISKA;1;3;2;5;1;3;6;DISKB;2;6;3;7;2;5;3\n"
        "ModuleBrakeSystem;611\n"
        "ModuleBrakeSystem;612;1.5;1;No;1;PAD;;5;;6");
    const ModuleBrakeSystemCard example = cardAt<ModuleBrakeSystemTable>(model, 0);
    REQUIRE(example.id == 610);
    REQUIRE((example.squealLoadFactor == 0.3 && example.frictionVectorMethod == 0 &&
             example.squealOnly == ModuleBrakeSystemSquealOnly::Yes));
    REQUIRE(example.contactMethod == 0);
    REQUIRE(example.disks.size() == 2);
    REQUIRE(example.disks[0] == ModuleBrakeSystemDisk{"DISKA", 1, 3, 2, 5, 1, 3, 6});
    REQUIRE(example.disks[1] == ModuleBrakeSystemDisk{"DISKB", 2, 6, 3, 7, 2, 5, 3});
    const ModuleBrakeSystemCard defaults = cardAt<ModuleBrakeSystemTable>(model, 1);
    REQUIRE((defaults.squealLoadFactor == 0.0 && defaults.frictionVectorMethod == 0 &&
             defaults.squealOnly == ModuleBrakeSystemSquealOnly::Yes));
    REQUIRE((defaults.contactMethod == 0 && defaults.disks.empty()));
    const ModuleBrakeSystemCard full = cardAt<ModuleBrakeSystemTable>(model, 2);
    REQUIRE((full.squealLoadFactor == 1.5 && full.frictionVectorMethod == 1 &&
             full.squealOnly == ModuleBrakeSystemSquealOnly::No && full.contactMethod == 1));
    REQUIRE(full.disks == std::vector<ModuleBrakeSystemDisk>{ModuleBrakeSystemDisk{"PAD", 0, 5, 0, 6, 0, 0, 0}});
    REQUIRE(ModuleBrakeSystemTable::INFO.idSpace == base::IdSpace::PerCardType);
    requireRoundTrip(model);

    // The bodies are BCBODY1 cards of their module: looked up in module 0 only. MOTION and BRKSYS are plain IDs.
    REQUIRE(referencesOf<ModuleBrakeSystemTable>(model, 0).fields.empty());
    const RecordingVisitor main = referencesOf<ModuleBrakeSystemTable>(model, 2);
    REQUIRE(main.fields == std::vector<std::string>{"body1Id", "body2Id"});
    REQUIRE(main.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::ContactBody));
    REQUIRE(unresolvedFields<ModuleBrakeSystemTable>(model, 2) == std::vector<std::string>{"body1Id", "body2Id"});
    REQUIRE(unresolvedFields<ModuleBrakeSystemTable>(model, 0).empty());
    REQUIRE_THROWS_AS(readBulk("ModuleBrakeSystem;900\nModuleBrakeSystem;900"), base::DeckError); // the ID is unique
}

TEST_CASE("an MDBKSYS refuses what its format does not allow", "[model][cards][modules]")
{
    expectInvalid("ModuleBrakeSystem;0", "id");
    expectInvalid("ModuleBrakeSystem;5;;2", "frictionVectorMethod");
    expectInvalid("ModuleBrakeSystem;5;;;MAYBE", "squealOnly");
    expectInvalid("ModuleBrakeSystem;5;;;;3", "contactMethod");
    expectInvalid("ModuleBrakeSystem;5;abc", "squealLoadFactor");
    expectInvalid("ModuleBrakeSystem;5;;;;;D1;0;0;0;3", "body1Id");
    expectInvalid("ModuleBrakeSystem;5;;;;;D1;0;1;0", "body2Id");
    expectInvalid("ModuleBrakeSystem;5;;;;;D1;-1;1;0;2", "moduleId1");
    REQUIRE_NOTHROW(readBulk("ModuleBrakeSystem;5;;;;;TOOLONGNAME;0;1;0;2"));
    expectInvalid("ModuleBrakeSystem;5;;;;;D1;0;1;0;2;-1", "motion1Id");
}

// --- MDBOLT ---------------------------------------------------------------------------------------------------------

TEST_CASE("an MDBOLT lists the top and bottom grid points of a bolt in their modules", "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleBolt;70;1250;12;12;34;Top;5;201;202;203;204;205;Bottom;5;6;7;8;9;10\n"
        "ModuleBolt;71;8000;;;;Top;9;1;2;3;4;5;6;7;8;9;Bottom;9;11;12;13;14;15;16;17;18;19");
    const ModuleBoltCard example = cardAt<ModuleBoltTable>(model, 0);
    REQUIRE((example.id == 70 && example.controlNode == 1250));
    REQUIRE((example.controlModuleId == 12 && example.topModuleId == 12 && example.bottomModuleId == 34));
    REQUIRE(example.topNodes == std::vector<base::EntityId>{201, 202, 203, 204, 205});
    REQUIRE(example.bottomNodes == std::vector<base::EntityId>{6, 7, 8, 9, 10});
    const ModuleBoltCard main = cardAt<ModuleBoltTable>(model, 1);
    REQUIRE(
        (main.controlModuleId == 0 && main.topModuleId == 0 && main.bottomModuleId == 0)); // the module defaults are 0
    REQUIRE(main.topNodes.size() == 9);
    REQUIRE(main.bottomNodes.size() == 9);
    REQUIRE((main.topNodes.back() == 9 && main.bottomNodes.back() == 19)); // the lists go on over lines
    REQUIRE(ModuleBoltTable::INFO.idSpace == base::IdSpace::PerCardType);
    requireRoundTrip(model);

    // Module 12 and 34 are not in this model; the bolt of the main Bulk Data names grid points.
    REQUIRE(referencesOf<ModuleBoltTable>(model, 0).fields.empty());
    const RecordingVisitor visitor = referencesOf<ModuleBoltTable>(model, 1);
    REQUIRE(visitor.fields.size() == 19);
    REQUIRE(visitor.fields.front() == "controlNode");
    REQUIRE(visitor.fields[1] == "topNodes");
    REQUIRE(visitor.fields.back() == "bottomNodes");
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(19, ReferenceTarget::Node));
    REQUIRE(unresolvedIn(model) == 19);
}

TEST_CASE("an MDBOLT follows a renumbering of the grid points of the main Bulk Data only", "[model][cards][modules]")
{
    Model model = readBulk(
        gridCards({1, 2, 3, 11, 12, 13, 100}) +
        "ModuleBolt;7;100;;;;Top;3;1;2;3;Bottom;3;11;12;13\nModuleBolt;8;100;1;1;1;Top;3;1;2;3;Bottom;3;11;12;13");
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    const ModuleBoltCard main = cardAt<ModuleBoltTable>(model, 0);
    REQUIRE(main.controlNode == 1100);
    REQUIRE(main.topNodes == std::vector<base::EntityId>{1001, 1002, 1003});
    REQUIRE(main.bottomNodes == std::vector<base::EntityId>{1011, 1012, 1013});
    const ModuleBoltCard other = cardAt<ModuleBoltTable>(model, 1);
    REQUIRE(other.controlNode == 100);
    REQUIRE(other.topNodes == std::vector<base::EntityId>{1, 2, 3}); // module 1
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("an MDBOLT refuses what its format and the BOLT do not allow", "[model][cards][modules]")
{
    expectInvalid("ModuleBolt;0;5;;;;Top;1;1;Bottom;1;2", "id");
    expectInvalid("ModuleBolt;5;0;;;;Top;1;1;Bottom;1;2", "controlNode");
    expectInvalid("ModuleBolt;5;9;-1;;;Top;1;1;Bottom;1;2", "controlModuleId");
    expectInvalid("ModuleBolt;5;9;3;1;2;Top;1;1;Bottom;1;2", "controlModuleId"); // MODC above 0 is MODT or MODB
    expectInvalid("ModuleBolt;5;9;;;;Top;1;1", "topList");                       // no BOTTOM list
    expectInvalid("ModuleBolt;5;9;;;;Bottom;1;1", "topList");                    // no TOP list
    expectInvalid("ModuleBolt;5;9;;;;Top;1;1;Bottom;1;2;Top;1;3", "topList");    // a list given twice
    expectInvalid("ModuleBolt;5;9;;;;Top;2;1;2;Bottom;1;3", "bottomList");       // not the same number of points
    expectInvalid("ModuleBolt;5;9;;;;Top;1;0;Bottom;1;2", "topNodes");
    expectInvalid("ModuleBolt;5;9;;;;Top;2;1;x;Bottom;2;2;3", "topNodes");
    expectInvalid("ModuleBolt;5;9;;;;Top;1;1;Bottom;1;9", "bottomNodes"); // GRIDC is not in the BOTTOM list
    // GRIDC may have the number of a bottom point of another module: the bottom points are in module 1 here.
    const Model other = readBulk("ModuleBolt;5;9;;;1;Top;1;1;Bottom;1;9");
    REQUIRE(cardAt<ModuleBoltTable>(other).bottomNodes == std::vector<base::EntityId>{9});
    REQUIRE_THROWS_AS(readBulk("ModuleBolt;5;9;;;;Top;1;1;Bottom;1;2\nModuleBolt;5;9;;;;Top;1;1;Bottom;1;2"),
                      base::DeckError);
}

// --- MDBUSH ---------------------------------------------------------------------------------------------------------

TEST_CASE("an MDBUSH is a CBUSH with the modules of its points", "[model][cards][modules]")
{
    const Model model = readBulk(
        "ModuleBushing;2701;2706;1;100;;6300;;;;;;;;;15;18\n"
        "ModuleBushing;40;;1;100\n"
        "ModuleBushing;41;7;5;6;0.;;1.;0.;2;0.25;;;;;3;4;5\n"
        "ModuleBushing;42;9;1;2;;3;;;0;0.75;1;0.1;0.2;0.3;5;6;7\n"
        "ModuleBushing;43;5;10;10;;;;;;;;;;;1;2");
    // The orientation is the grid point GO and the end points are in modules 15 and 18.
    const ModuleBushingCard example = cardAt<ModuleBushingTable>(model, 0);
    REQUIRE((example.id == 2701 && example.propId == 2706 && example.nodeA == 1 && example.nodeB == 100 &&
             example.orientationNode == 6300));
    REQUIRE((example.frameId == ModuleBushingCard::NO_FRAME_ID && example.offsetFraction == 0.5 &&
             example.offsetFrameId == ModuleBushingCard::NO_OFFSET_FRAME_ID));
    REQUIRE((example.moduleIdA == 15 && example.moduleIdB == 18 && example.orientationModuleId == 0));
    REQUIRE_FALSE(example.hasVector());
    const ModuleBushingCard minimal = cardAt<ModuleBushingTable>(model, 1);
    REQUIRE(minimal.propId == 40); // a blank PID is the EID, stored resolved
    REQUIRE((minimal.orientationNode == 0 && !minimal.hasVector()));
    REQUIRE((minimal.moduleIdA == 0 && minimal.moduleIdB == 0 && minimal.orientationModuleId == 0));
    const ModuleBushingCard vector = cardAt<ModuleBushingTable>(model, 2);
    REQUIRE((vector.orientationVector == std::array<double, 3>{0.0, 1.0, 0.0} && vector.orientationNode == 0 &&
             vector.frameId == 2));
    REQUIRE(vector.offsetFraction == 0.25);
    REQUIRE((vector.moduleIdA == 3 && vector.moduleIdB == 4 && vector.orientationModuleId == 5));
    const ModuleBushingCard offset = cardAt<ModuleBushingTable>(model, 3);
    REQUIRE((offset.orientationNode == 3 && offset.frameId == 0 && offset.offsetFraction == 0.75 &&
             offset.offsetFrameId == 1));
    REQUIRE(offset.offsets == std::array<double, 3>{0.1, 0.2, 0.3});
    REQUIRE((offset.moduleIdA == 5 && offset.moduleIdB == 6 && offset.orientationModuleId == 7));
    // The two points have the same number in different modules, which is what the card is for.
    const ModuleBushingCard twin = cardAt<ModuleBushingTable>(model, 4);
    REQUIRE((twin.nodeA == 10 && twin.nodeB == 10 && twin.moduleIdA == 1 && twin.moduleIdB == 2));
    REQUIRE(ModuleBushingTable::INFO.idSpace == base::IdSpace::PerCardType);
    requireRoundTrip(model);
}

TEST_CASE("an MDBUSH looks up what the main Bulk Data holds, in the module each field names", "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleBushing;2701;2706;1;100;;6300;;;;;;;;;15;18\n"
        "ModuleBushing;40;;1;100\n"
        "ModuleBushing;41;7;5;6;0.;;1.;0.;2;0.25;;;;;3;4;5\n"
        "ModuleBushing;50;;1;2;;;;;3");
    // GA and GB are looked up when their module is 0, the orientation point, CID and OCID in module MODC.
    const RecordingVisitor example = referencesOf<ModuleBushingTable>(model, 0);
    REQUIRE(example.fields == std::vector<std::string>{"propId", "orientationNode"});
    REQUIRE(example.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node});
    const RecordingVisitor minimal = referencesOf<ModuleBushingTable>(model, 1);
    REQUIRE(minimal.fields == std::vector<std::string>{"propId", "nodeA", "nodeB"});
    REQUIRE(minimal.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node, ReferenceTarget::Node});
    REQUIRE(referencesOf<ModuleBushingTable>(model, 2).fields ==
            std::vector<std::string>{"propId"}); // modules 3, 4 and 5
    const RecordingVisitor coordinate = referencesOf<ModuleBushingTable>(model, 3);
    REQUIRE(coordinate.fields == std::vector<std::string>{"propId", "nodeA", "nodeB", "frameId"});
    REQUIRE(coordinate.targets.back() == ReferenceTarget::Coordinate);
    REQUIRE(unresolvedFields<ModuleBushingTable>(model, 1) == std::vector<std::string>{"propId", "nodeA", "nodeB"});

    Model complete = readBulk(
        "Node;1;;0.;0.;0.\nNode;100;;1.;0.;0.\nBushProp;40;Stiffness;100.\nModuleBushing;40;;1;100\nModuleBushing;41;"
        "40;1;"
        "100;;;;;;;;;;;3;4");
    REQUIRE(unresolvedIn(complete) == 0);
    Renumbering::apply(complete, RenumberSpace::Property, Renumbering::offset(1000));
    Renumbering::apply(complete, RenumberSpace::Node, Renumbering::offset(10));
    const ModuleBushingCard first = cardAt<ModuleBushingTable>(complete, 0);
    REQUIRE((first.propId == 1040 && first.nodeA == 11 && first.nodeB == 110));
    const ModuleBushingCard second = cardAt<ModuleBushingTable>(complete, 1);
    REQUIRE((second.propId == 1040 && second.nodeA == 1 && second.nodeB == 100)); // modules 3 and 4: left alone
    REQUIRE(unresolvedIn(complete) == 0);
}

TEST_CASE("an MDBUSH refuses what its format does not allow", "[model][cards][modules]")
{
    expectInvalid("ModuleBushing;0;1;1;2", "id");
    expectInvalid("ModuleBushing;100000000;;1;2", "id");
    expectInvalid("ModuleBushing;5;;0;2", "nodeA");
    expectInvalid("ModuleBushing;5;;1;-1", "nodeB");
    expectInvalid("ModuleBushing;5;;1;1", "nodeB"); // the same point in the same module
    expectInvalid("ModuleBushing;5;;1;2;0.;;0.;1.;-2", "frameId");
    expectInvalid("ModuleBushing;5;;1;2;;;;;;;-2", "offsetFrameId");
    expectInvalid("ModuleBushing;5;;1;2;;;;;;;;;;;-1", "moduleIdA");
    expectInvalid("ModuleBushing;5;;1;2;;;;;;;;;;;;-1", "moduleIdB");
    expectInvalid("ModuleBushing;5;;1;2;;;;;;;;;;;;;-1", "orientationModuleId");
    expectInvalid("ModuleBushing;5;;1;2;x", "orientationNode/position1");
    REQUIRE_THROWS_AS(readBulk("ModuleBushing;5;;1;2\nModuleBushing;5;;3;4"), base::DeckError);
    ModuleBushingCard card;
    card.id = 5;
    card.propId = 5;
    card.nodeA = 1;
    card.orientationNode = 3;
    card.orientationVector = {1.0, 0.0, 0.0};
    REQUIRE(fieldOf(ModuleBushingFormat::check(card)) == "orientationNode"); // a grid point or a vector, not both
    card.orientationVector = {};
    REQUIRE_FALSE(ModuleBushingFormat::check(card).has_value());
    card.nodeB = 1;
    card.moduleIdA = 1;
    card.moduleIdB = 2;
    REQUIRE_FALSE(ModuleBushingFormat::check(card).has_value()); // the same number in two modules
    card.moduleIdB = 1;
    REQUIRE(fieldOf(ModuleBushingFormat::check(card)) == "nodeB");
}

// --- the kinds of ID
// --------------------------------------------------------------------------------------------------

TEST_CASE("the multi-module cards of this group are filed as the format of the step says", "[model][cards][modules]")
{
    REQUIRE(ModuleBulkOptionsTable::INFO.category == CardCategory::Other);
    REQUIRE(ModuleLabelTable::INFO.category == CardCategory::Other);
    REQUIRE(ModuleBoundaryNodesTable::INFO.idSpace == base::IdSpace::None);
    REQUIRE(ModuleExcludedNodesTable::INFO.idSpace == base::IdSpace::None);
    REQUIRE(ModuleConnectionTable::INFO.category == CardCategory::Other);
    REQUIRE(ModuleEntryExclusionTable::INFO.category == CardCategory::Other);
    REQUIRE(ModuleDirectMatrixTable::INFO.category == CardCategory::Matrix);
    REQUIRE(ModuleBrakeSystemTable::INFO.idSpace == base::IdSpace::PerCardType);
    REQUIRE(ModuleBoltTable::INFO.idSpace == base::IdSpace::PerCardType);
    REQUIRE(ModuleBushingTable::INFO.idSpace == base::IdSpace::PerCardType);
    // An MDBUSH and a CBUSH of the same number do not clash: the element of a module is not an element of the model.
    const Model model = readBulk(
        "Node;1;;0.;0.;0.\nNode;2;;1.;0.;0.\nBushProp;5;Stiffness;100.\nBushing;7;5;1;2\nModuleBushing;7;5;1;2");
    REQUIRE(model.cardCount() == 5);
}
