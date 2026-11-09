// Checks of the cards that connect modules (MDFAST, MDMPC, MDRBE2, MDRBE3, MDRJNT,
// MDRROD, MDSEAM and MDWELD). A module is a BEGIN MODULE section of the deck, which the library keeps as text: module 0
// (a blank field) is the main Bulk Data, and only an ID that a card places there is a reference to look up. Per card:
// the fields in their format reference positions, the defaults, the references the card lists (and which of them depend
// on a module), the renumbering of the spaces the references name, the refused values and the round trip through the
// three formats.

#include "oor/model/cards/constraints/multiPointConstr/MultiPointConstrTable.hpp"
#include "oor/model/cards/nodes/moduleFastener/ModuleFastenerTable.hpp"
#include "oor/model/cards/nodes/moduleMultiPointConstr/ModuleMultiPointConstrTable.hpp"
#include "oor/model/cards/nodes/moduleRigidBodyStar/ModuleRigidBodyStarTable.hpp"
#include "oor/model/cards/nodes/moduleRigidInterpolation/ModuleRigidInterpolationTable.hpp"
#include "oor/model/cards/nodes/moduleRigidJoint/ModuleRigidJointTable.hpp"
#include "oor/model/cards/nodes/moduleRigidRod/ModuleRigidRodTable.hpp"
#include "oor/model/cards/nodes/moduleSeam/ModuleSeamTable.hpp"
#include "oor/model/cards/nodes/moduleWeld/ModuleWeldTable.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
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

// The number a command of the global Case Control section holds.
std::int64_t globalCommand(const Model& model, const char* name)
{
    const CaseEntry* entry = CaseControl::ownCommand(model.control().caseControl.global(), name);
    REQUIRE(entry != nullptr);
    return entry->value.integer;
}

using Pairs = std::vector<ModuleId>;
using Xyz = std::array<std::optional<double>, 3>;

} // namespace

// ------------------------------------------------------------------------------------------------------------ MDFAST

TEST_CASE("an MDFAST puts its two patches in modules, takes its element number for a blank property and locates itself",
          "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleFastener;4;30;Property;31;35;412;;;;;;13;23\n"
        "ModuleFastener;6;60;Elements;36;58;613;;;;;;14;16\n"
        "ModuleFastener;8;;Property;1;2;;9\n"
        "ModuleFastener;9;;Elements;1;2;;;;1.5;-2.5;100.");
    const ModuleFastenerCard property = cardAt<ModuleFastenerTable>(model, 0);
    REQUIRE((property.id == 4 && property.propId == 30 && property.type == ModuleFastenerType::Property));
    REQUIRE((property.patchIdA == 31 && property.patchIdB == 35 && property.spotNode == 412 && property.nodeA == 0 &&
             property.nodeB == 0));
    REQUIRE((property.moduleIdA == 13 && property.moduleIdB == 23 && !property.location.has_value()));
    const ModuleFastenerCard element = cardAt<ModuleFastenerTable>(model, 1);
    REQUIRE((element.type == ModuleFastenerType::Element && element.moduleIdA == 14 && element.moduleIdB == 16));
    // A blank PID is the element number and a blank module is the main Bulk Data.
    const ModuleFastenerCard blank = cardAt<ModuleFastenerTable>(model, 2);
    REQUIRE((blank.propId == 8 && blank.nodeA == 9 && blank.moduleIdA == 0 && blank.moduleIdB == 0));
    const ModuleFastenerCard located = cardAt<ModuleFastenerTable>(model, 3);
    REQUIRE(located.location.has_value());
    REQUIRE(*located.location == std::array<double, 3>{1.5, -2.5, 100.0});
    REQUIRE((located.spotNode == 0 && located.nodeA == 0 && located.nodeB == 0));
    requireRoundTrip(model);
    // The property is resolved: it is always written.

    // PID names a PFAST; IDA and IDB are looked up only in the module that is the main Bulk Data, and name properties
    // or elements as TYPE says; the three grid points are in the main Bulk Data.
    const RecordingVisitor modules = referencesOf<ModuleFastenerTable>(model, 0);
    REQUIRE(modules.fields == std::vector<std::string>{"propId", "spotNode"});
    REQUIRE(modules.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node});
    const RecordingVisitor main = referencesOf<ModuleFastenerTable>(model, 2);
    REQUIRE(main.fields == std::vector<std::string>{"propId", "patchIdA", "patchIdB", "nodeA"});
    REQUIRE(main.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Property,
                                                         ReferenceTarget::Property, ReferenceTarget::Node});
    const RecordingVisitor elements = referencesOf<ModuleFastenerTable>(model, 3);
    REQUIRE(elements.fields == std::vector<std::string>{"propId", "patchIdA", "patchIdB"});
    REQUIRE(elements.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Element,
                                                             ReferenceTarget::Element});

    Model lacking = readBulk("ModuleFastener;8;;Property;1;2;;9");
    REQUIRE(unresolvedFields<ModuleFastenerTable>(lacking) ==
            std::vector<std::string>{"propId", "patchIdA", "patchIdB", "nodeA"});
    Model elsewhere = readBulk("ModuleFastener;8;;Property;1;2;;9;;;;;3;4");
    REQUIRE(unresolvedFields<ModuleFastenerTable>(elsewhere) == std::vector<std::string>{"propId", "nodeA"});
    Model resolved = readBulk(
        "Node;9;;0.;0.;0.\n"
        "FastenerProp;8;1.2;;;1.2E+5;5.2E+4;13700.\n"
        "FastenerProp;9;1.2;;;1.2E+5;5.2E+4;13700.\n"
        "ShellProp;1;1;0.01\n"
        "ShellProp;2;1;0.01\n"
        "ShellQuad4;1;1;1;2;3;4\n"
        "ShellQuad4;2;1;1;2;3;4\n"
        "ModuleFastener;8;;Property;1;2;;9\n"
        "ModuleFastener;9;;Elements;1;2;9");
    REQUIRE(unresolvedFields<ModuleFastenerTable>(resolved, 0).empty());
    REQUIRE(unresolvedFields<ModuleFastenerTable>(resolved, 1).empty());
}

TEST_CASE("the properties, elements and grid points of an MDFAST follow a renumbering, those of another module do not",
          "[model][cards][modules]")
{
    Model model = readBulk(
        "Node;9;;0.;0.;0.\n"
        "ShellProp;1;1;0.01\n"
        "ShellProp;2;1;0.01\n"
        "ShellQuad4;1;1;1;2;3;4\n"
        "ShellQuad4;2;1;1;2;3;4\n"
        "ModuleFastener;8;;Property;1;2;;9\n"
        "ModuleFastener;9;;Elements;1;2;9;;;;;;3;4\n"
        "ModuleFastener;10;;Property;1;2;5;;;;;;3;4\n"
        "ModuleFastener;11;;Elements;1;2;9");
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(100));
    REQUIRE((cardAt<ModuleFastenerTable>(model, 0).patchIdA == 101 &&
             cardAt<ModuleFastenerTable>(model, 0).patchIdB == 102));
    REQUIRE((cardAt<ModuleFastenerTable>(model, 1).patchIdA == 1 &&
             cardAt<ModuleFastenerTable>(model, 1).patchIdB == 2)); // elements
    REQUIRE((cardAt<ModuleFastenerTable>(model, 2).patchIdA == 1 &&
             cardAt<ModuleFastenerTable>(model, 2).patchIdB == 2)); // in modules 3 and 4
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE((cardAt<ModuleFastenerTable>(model, 3).patchIdA == 101 &&
             cardAt<ModuleFastenerTable>(model, 3).patchIdB == 102));
    REQUIRE((cardAt<ModuleFastenerTable>(model, 1).patchIdA == 1 &&
             cardAt<ModuleFastenerTable>(model, 1).patchIdB == 2)); // in modules 3 and 4
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<ModuleFastenerTable>(model, 0).nodeA == 1009 &&
             cardAt<ModuleFastenerTable>(model, 1).spotNode == 1009));
    REQUIRE((cardAt<ModuleFastenerTable>(model, 3).spotNode == 1009 &&
             cardAt<ModuleFastenerTable>(model, 2).spotNode == 1005));
    // The element number belongs to the card: no renumbering space moves it.
    REQUIRE((cardAt<ModuleFastenerTable>(model, 0).id == 8 && cardAt<ModuleFastenerTable>(model, 3).id == 11));
}

TEST_CASE("an MDFAST needs its location, refuses what its layout does not use and keeps its number per card type",
          "[model][cards][modules]")
{
    expectInvalid("ModuleFastener;0;;Property;1;2;3", "id");
    expectInvalid("ModuleFastener;100000000;;Property;1;2;3", "id");
    expectInvalid("ModuleFastener;8;0;Property;1;2;3", "propId");
    expectInvalid("ModuleFastener;8;;SPOT;1;2;3", "kind");
    expectInvalid("ModuleFastener;8;;;1;2;3", "kind");
    expectInvalid("ModuleFastener;8;;Property;;2;3", "patchIdA");
    expectInvalid("ModuleFastener;8;;Property;1;;3", "patchIdB");
    expectInvalid("ModuleFastener;8;;Property;1;2;-3", "spotNode");
    expectInvalid("ModuleFastener;8;;Property;1;2", "spotNode"); // the location is GS, GA or XS, YS and ZS
    expectInvalid("ModuleFastener;8;;Property;1;2;3;;;1.;2.", "xStation");
    expectInvalid("ModuleFastener;8;;Property;1;2;3;;;;;;-1", "moduleIdA");
    expectInvalid("ModuleFastener;8;;Property;1;2;3;;;;;;3;-1", "moduleIdB");

    // The same number is the same patch only in one module.
    REQUIRE_NOTHROW(readBulk("ModuleFastener;8;;Property;1;1;5;;;;;;3;4"));
    expectInvalid("ModuleFastener;8;;Property;1;1;5;;;;;;3;3", "patchIdB");
    expectInvalid("ModuleFastener;8;;Property;1;1;5", "patchIdB");

    // The element number is unique among the MDFAST cards; another kind of card may have it.
    REQUIRE_THROWS_AS(readBulk("ModuleFastener;8;;Property;1;2;5\nModuleFastener;8;;Property;3;4;5"),
                      base::DeckError);
    REQUIRE_NOTHROW(readBulk("ModuleFastener;8;;Property;1;2;5\nModuleRigidJoint;8;1;1;2;1"));
}

// -------------------------------------------------------------------------------------------------------------- MDMPC

TEST_CASE("an MDMPC has a point of a module in each term and is a constraint set that the Case Control names",
          "[model][cards][modules]")
{
    Model model = readModelText(
        "[case]\n"
        "MultiPointConstrSelection;52201\n"
        "[model]\n"
        "ModuleMultiPointConstr;52201;12;37;3;5.4;14;6;;3.86;23;2;4;-1.73\n"
        "ModuleMultiPointConstr;52202;;5;3;1.;;6;3;-1.\n"
        "MultiPointConstr;52202;5;3;1.;6;3;-1.\n"
        "Node;5;;0.;0.;0.\n"
        "Node;6;;1.;0.;0.\n",
        true);
    const ModuleMultiPointConstrCard example = cardAt<ModuleMultiPointConstrTable>(model, 0);
    REQUIRE(example.id == 52201);
    REQUIRE(example.terms.size() == 3);
    REQUIRE((example.terms[0].point == ModuleId{12, 37} && example.terms[0].component == 3 &&
             example.terms[0].coefficient == 5.4));
    // A blank component is 0: the format reference leaves it blank for a scalar point.
    REQUIRE((example.terms[1].point == ModuleId{14, 6} && example.terms[1].component == 0 &&
             example.terms[1].coefficient == 3.86));
    REQUIRE((example.terms[2].point == ModuleId{23, 2} && example.terms[2].component == 4 &&
             example.terms[2].coefficient == -1.73));
    const ModuleMultiPointConstrCard main = cardAt<ModuleMultiPointConstrTable>(model, 1);
    REQUIRE((main.terms.size() == 2 && main.terms[0].point == ModuleId{0, 5} && main.terms[1].point == ModuleId{0, 6}));
    requireRoundTrip(model);

    // Every point of the example is in a module; the points of the main Bulk Data are grid or scalar points.
    REQUIRE(referencesOf<ModuleMultiPointConstrTable>(model, 0).fields.empty());
    const RecordingVisitor visitor = referencesOf<ModuleMultiPointConstrTable>(model, 1);
    REQUIRE(visitor.fields == std::vector<std::string>{"node", "node"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::Node));

    // The MDMPC is a set of the constraints that the Case Control MPC command selects, with the MPC cards of its
    // number.
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::ConstraintSet, Renumbering::offset(1000));
    REQUIRE((cardAt<ModuleMultiPointConstrTable>(model, 0).id == 53201 &&
             cardAt<ModuleMultiPointConstrTable>(model, 1).id == 53202));
    REQUIRE(cardAt<MultiPointConstrTable>(model).id == 53202); // the MPC of the same set moves with it
    REQUIRE(globalCommand(model, "MultiPointConstrSelection") == 53201);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<ModuleMultiPointConstrTable>(model, 1).terms[0].point == ModuleId{0, 105} &&
             cardAt<ModuleMultiPointConstrTable>(model, 1).terms[1].point == ModuleId{0, 106}));
    REQUIRE(cardAt<ModuleMultiPointConstrTable>(model, 0).terms[0].point ==
            ModuleId{12, 37}); // a point of another module
    REQUIRE(unresolvedIn(model) == 0);

    Model lacking =
        readModelText("[case]\nMultiPointConstrSelection;9\n[model]\nModuleMultiPointConstr;8;;5;3;1.\n", true);
    REQUIRE(unresolvedIn(lacking) == 2); // the command and the grid point
}

TEST_CASE("the terms of an MDMPC follow each other from field 9 and its first line holds one term",
          "[model][cards][modules]")
{
    const Model model = readBulk("ModuleMultiPointConstr;3;;5;1;1.;;6;1;-1.;;7;1;-1.;2;8;2;0.5");
    const ModuleMultiPointConstrCard card = cardAt<ModuleMultiPointConstrTable>(model);
    REQUIRE(card.terms.size() == 4);
    REQUIRE((card.terms[1].point.id == 6 && card.terms[2].point.id == 7 && card.terms[3].point == ModuleId{2, 8}));
    REQUIRE((card.terms[3].component == 2 && card.terms[3].coefficient == 0.5));
    requireRoundTrip(model);

    // A coefficient other than A1 is 0.0 when blank; A1 is required and not zero.
    const Model blank = readBulk("ModuleMultiPointConstr;3;;5;;1.;;8");
    REQUIRE((cardAt<ModuleMultiPointConstrTable>(blank).terms.size() == 2 &&
             cardAt<ModuleMultiPointConstrTable>(blank).terms[1].coefficient == 0.0));
    REQUIRE(cardAt<ModuleMultiPointConstrTable>(blank).terms[1].component == 0);

    expectInvalid("ModuleMultiPointConstr;0;;5;1;1.", "id");
    expectInvalid("ModuleMultiPointConstr;3", "node");
    expectInvalid("ModuleMultiPointConstr;3;;;1;1.", "node");
    expectInvalid("ModuleMultiPointConstr;3;-1;5;1;1.", "moduleId");
    expectInvalid("ModuleMultiPointConstr;3;;5;7;1.", "components");
    expectInvalid("ModuleMultiPointConstr;3;;5;1", "coefficient1");
    expectInvalid("ModuleMultiPointConstr;3;;5;1;0.", "coefficient1");
    expectInvalid("ModuleMultiPointConstr;3;;5;1;1.;;6;1;x", "coefficient");
}

// ------------------------------------------------------------------------------------------------------------- MDRBE2

TEST_CASE("an MDRBE2 pairs each dependent grid point with its module and ends with ALPHA and TREF",
          "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleRigidBodyStar;9;301;4;13;401;20;402;22;402;24;403;25;403;26;7.5E-6\n"
        "ModuleRigidBodyStar;10;;5;3;1;6;2;7;1.2E-5;20.\n"
        "ModuleRigidBodyStar;11;7;50;321;8;60;;70;;300.\n"
        "ModuleRigidBodyStar;12;;5;1;;6");
    const ModuleRigidBodyStarCard example = cardAt<ModuleRigidBodyStarTable>(model, 0);
    REQUIRE((example.id == 9 && example.independentModuleId == 301 && example.independentNode == 4 &&
             example.components == 13));
    REQUIRE(example.dependents == Pairs{{401, 20}, {402, 22}, {402, 24}, {403, 25}, {403, 26}});
    REQUIRE((example.alphaCoefficient == 7.5e-6 && example.referenceTemperature == 0.0));
    const ModuleRigidBodyStarCard both = cardAt<ModuleRigidBodyStarTable>(model, 1);
    REQUIRE((both.alphaCoefficient == 1.2e-5 && both.referenceTemperature == 20.0 &&
             both.dependents == Pairs{{1, 6}, {2, 7}}));
    // A blank ALPHA before a TREF; a blank module is the main Bulk Data; the digits of CM are put in order.
    const ModuleRigidBodyStarCard tref = cardAt<ModuleRigidBodyStarTable>(model, 2);
    REQUIRE((tref.alphaCoefficient == 0.0 && tref.referenceTemperature == 300.0 && tref.components == 123 &&
             tref.independentModuleId == 7));
    REQUIRE(tref.dependents == Pairs{{8, 60}, {0, 70}});
    const ModuleRigidBodyStarCard bare = cardAt<ModuleRigidBodyStarTable>(model, 3);
    REQUIRE((bare.independentModuleId == 0 && bare.dependents == Pairs{{0, 6}} && bare.alphaCoefficient == 0.0 &&
             bare.referenceTemperature == 0.0));
    requireRoundTrip(model);

    // The independent point and the dependent points are looked up only in the main Bulk Data.
    REQUIRE(referencesOf<ModuleRigidBodyStarTable>(model, 0).fields.empty());
    REQUIRE(referencesOf<ModuleRigidBodyStarTable>(model, 2).fields == std::vector<std::string>{"dependentNode"});
    const RecordingVisitor visitor = referencesOf<ModuleRigidBodyStarTable>(model, 3);
    REQUIRE(visitor.fields == std::vector<std::string>{"independentNode", "dependentNode"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::Node));

    Model lacking = readBulk("ModuleRigidBodyStar;12;;5;1;;6");
    REQUIRE(unresolvedFields<ModuleRigidBodyStarTable>(lacking) ==
            std::vector<std::string>{"independentNode", "dependentNode"});
    Model resolved = readBulk(
        "Node;5;;0.;0.;0.\nNode;6;;1.;0.;0.\nModuleRigidBodyStar;12;;5;1;;6\nModuleRigidBodyStar;13;3;5;1;3;6");
    REQUIRE(unresolvedFields<ModuleRigidBodyStarTable>(resolved, 0).empty());
    REQUIRE(unresolvedFields<ModuleRigidBodyStarTable>(resolved, 1)
                .empty()); // GN is in module 3, which the model does not hold
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<ModuleRigidBodyStarTable>(resolved, 0).independentNode == 105 &&
             cardAt<ModuleRigidBodyStarTable>(resolved, 0).dependents == Pairs{{0, 106}}));
    REQUIRE((cardAt<ModuleRigidBodyStarTable>(resolved, 1).independentNode == 5 &&
             cardAt<ModuleRigidBodyStarTable>(resolved, 1).dependents == Pairs{{3, 6}}));
}

TEST_CASE("an MDRBE2 refuses an unpaired module, a bad component set and a missing dependent point",
          "[model][cards][modules]")
{
    expectInvalid("ModuleRigidBodyStar;0;;5;1;;6", "id");
    expectInvalid("ModuleRigidBodyStar;12;-1;5;1;;6", "independentModuleId");
    expectInvalid("ModuleRigidBodyStar;12;;;1;;6", "independentNode");
    expectInvalid("ModuleRigidBodyStar;12;;5;0;;6", "dependentComponents");
    expectInvalid("ModuleRigidBodyStar;12;;5;7;;6", "dependentComponents");
    expectInvalid("ModuleRigidBodyStar;12;;5;1", "dependentNode");
    expectInvalid("ModuleRigidBodyStar;12;;5;1;;-6", "dependentNode");
    expectInvalid("ModuleRigidBodyStar;12;;5;1;;6;9", "dependentNode"); // a module without its grid point
    REQUIRE_THROWS_AS(readBulk("ModuleRigidBodyStar;12;;5;1;;6\nModuleRigidBodyStar;12;;5;1;;7"), base::DeckError);
}

// ------------------------------------------------------------------------------------------------------------- MDRBE3

TEST_CASE("an MDRBE3 has weighted groups of pairs, a UM part of triples and ALPHA, each on a line of its own",
          "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleRigidInterpolation;24;2000;200;1236;30;1.;123;301;1;302;3;303;5;3.6;1;301;2;302;4;303;6;4.4;2;301;7;302;"
        "8;303;9;6.3;1;304;15;305;16;DependentDofs;3;301;200;15;302;5;3;303;7;2;ThermalExpansion;7.5E-6\n"
        "ModuleRigidInterpolation;15;;100;123456;4;1.;123;;5\n"
        "ModuleRigidInterpolation;16;;300;123456;12;1.;123;6;1;6;2;;3;2.;3;6;4;DependentDofs;2;6;5;12;;6;3;"
        "ThermalExpansion;1.5E-6;22.");
    const ModuleRigidInterpolationCard example = cardAt<ModuleRigidInterpolationTable>(model, 0);
    REQUIRE((example.id == 24 && example.referenceModuleId == 2000 && example.referenceNode == 200 &&
             example.referenceComponents == 1236));
    REQUIRE(example.groups.size() == 4);
    REQUIRE(example.groups[0] == ModuleRigidInterpolationGroup{1.0, 123, Pairs{{301, 1}, {302, 3}, {303, 5}}});
    REQUIRE(example.groups[1] == ModuleRigidInterpolationGroup{3.6, 1, Pairs{{301, 2}, {302, 4}, {303, 6}}});
    REQUIRE(example.groups[2] == ModuleRigidInterpolationGroup{4.4, 2, Pairs{{301, 7}, {302, 8}, {303, 9}}});
    REQUIRE(example.groups[3] == ModuleRigidInterpolationGroup{6.3, 1, Pairs{{304, 15}, {305, 16}}});
    // UM: two triples on a line, the first field of every line but the first blank.
    REQUIRE(example.dependents == std::vector<ModuleRigidInterpolationDependent>{
                                      {ModuleId{301, 200}, 15}, {ModuleId{302, 5}, 3}, {ModuleId{303, 7}, 2}});
    REQUIRE((example.alphaCoefficient == 7.5e-6 && example.referenceTemperature == 0.0));

    // A blank module before a grid point is the main Bulk Data, in a group and in a triple.
    const ModuleRigidInterpolationCard minimal = cardAt<ModuleRigidInterpolationTable>(model, 1);
    REQUIRE((minimal.referenceModuleId == 0 && minimal.groups.size() == 1 && minimal.dependents.empty() &&
             minimal.alphaCoefficient == 0.0));
    REQUIRE(minimal.groups[0] == ModuleRigidInterpolationGroup{1.0, 123, Pairs{{0, 5}}});
    const ModuleRigidInterpolationCard mixed = cardAt<ModuleRigidInterpolationTable>(model, 2);
    REQUIRE(mixed.groups.size() == 2);
    REQUIRE(mixed.groups[0] == ModuleRigidInterpolationGroup{1.0, 123, Pairs{{6, 1}, {6, 2}, {0, 3}}});
    REQUIRE(mixed.groups[1] == ModuleRigidInterpolationGroup{2.0, 3, Pairs{{6, 4}}});
    REQUIRE(mixed.dependents ==
            std::vector<ModuleRigidInterpolationDependent>{{ModuleId{6, 5}, 12}, {ModuleId{0, 6}, 3}});
    REQUIRE((mixed.alphaCoefficient == 1.5e-6 && mixed.referenceTemperature == 22.0));
    requireRoundTrip(model);

    // The reference point, the points of the groups and the points after UM of the main Bulk Data are grid points.
    REQUIRE(referencesOf<ModuleRigidInterpolationTable>(model, 0).fields.empty());
    const RecordingVisitor main = referencesOf<ModuleRigidInterpolationTable>(model, 1);
    REQUIRE(main.fields == std::vector<std::string>{"referenceNode", "node"});
    const RecordingVisitor mixedReferences = referencesOf<ModuleRigidInterpolationTable>(model, 2);
    REQUIRE(mixedReferences.fields == std::vector<std::string>{"referenceNode", "node", "dependentNode"});
    REQUIRE(mixedReferences.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::Node));

    Model lacking = readBulk("ModuleRigidInterpolation;15;;100;123456;4;1.;123;;5");
    REQUIRE(unresolvedFields<ModuleRigidInterpolationTable>(lacking) ==
            std::vector<std::string>{"referenceNode", "node"});
    Model resolved =
        readBulk("Node;100;;0.;0.;0.\nNode;5;;1.;0.;0.\nModuleRigidInterpolation;15;;100;123456;4;1.;123;;5");
    REQUIRE(unresolvedFields<ModuleRigidInterpolationTable>(resolved).empty());
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<ModuleRigidInterpolationTable>(resolved).referenceNode == 1100);
    REQUIRE(cardAt<ModuleRigidInterpolationTable>(resolved).groups[0].points == Pairs{{0, 1005}});
}

TEST_CASE("an MDRBE3 is written with its UM and ALPHA parts at the start of a line, as the format draws them",
          "[model][cards][modules]")
{
    // The groups end in field 34: UM goes to the next line (field 41), its three triples fill fields 42 to 47 and 50 to
    // 52, ALPHA goes to field 57. A card read from that text is written back to the same fields.
    const Model model = readBulk(
        "ModuleRigidInterpolation;24;2000;200;1236;30;1.;123;301;1;302;3;303;5;3.6;1;301;2;302;4;303;6;4.4;2;301;7;302;"
        "8;303;9;6.3;1;304;15;305;16;DependentDofs;3;301;200;15;302;5;3;303;7;2;ThermalExpansion;7.5E-6");

    oor::testing::requireModelFileRoundTrip(model);

    // A card that gives the groups only is written packed, with every module.
    const Model packed = readBulk("ModuleRigidInterpolation;15;;100;123456;4;1.;123;;5");
    oor::testing::requireModelFileRoundTrip(packed);

    // A UM with a fourth and a fifth triple goes on over the next lines.
    const Model many = readBulk(
        "ModuleRigidInterpolation;16;1;400;123;4;1.;1;1;10;DependentDofs;5;1;20;1;1;21;2;1;22;3;1;23;4;1;24;5");
    const ModuleRigidInterpolationCard card = cardAt<ModuleRigidInterpolationTable>(many);
    REQUIRE(card.dependents.size() == 5);
    REQUIRE(card.dependents[4] == ModuleRigidInterpolationDependent{ModuleId{1, 24}, 5});
    requireRoundTrip(many);
}

TEST_CASE("an MDRBE3 refuses a group without a point, a UM that does not start a line and fields after TREF",
          "[model][cards][modules]")
{
    expectInvalid("ModuleRigidInterpolation;0;;100;1;4;1.;1;;5", "id");
    expectInvalid("ModuleRigidInterpolation;14;-1;100;1;4;1.;1;;5", "referenceModuleId");
    expectInvalid("ModuleRigidInterpolation;14;;;1;4;1.;1;;5", "referenceNode");
    expectInvalid("ModuleRigidInterpolation;14;;100;7;4;1.;1;;5", "referenceComponents");
    expectInvalid("ModuleRigidInterpolation;14;;100;1", "weight1");
    expectInvalid("ModuleRigidInterpolation;14;;100;1;4;1.;7;;5", "components");
    expectInvalid("ModuleRigidInterpolation;14;;100;1;2;1.;1", "node");
    expectInvalid("ModuleRigidInterpolation;14;;100;1;3;1.;1;3", "node"); // a module without its grid point
    expectInvalid("ModuleRigidInterpolation;14;;100;1;4;1.;1;-2;5", "moduleId");
    expectInvalid("ModuleRigidInterpolation;14;;100;1;4;1.;1;;-5", "node");
    expectInvalid("ModuleRigidInterpolation;14;;100;1;9;1.;1;;5;;DependentDofs;1;2;3",
                  "dependentList"); // UM is the first field of a line
    expectInvalid("ModuleRigidInterpolation;14;;100;1;4;1.;1;;5;DependentDofs;1;1;5;7", "dependentComponents");
    expectInvalid("ModuleRigidInterpolation;14;;100;1;4;1.;1;;5;DependentDofs;1;;5",
                  "dependentComponents"); // a triple needs its components
    expectInvalid("ModuleRigidInterpolation;14;;100;1;4;1.;1;;5;ThermalExpansion;1.;;DependentDofs;1;1;5;3",
                  "dependentList"); // UM comes before ALPHA
}

// ------------------------------------------------------------------------------------------------------------- MDRJNT

TEST_CASE("an MDRJNT joins a point of each of two modules, with the components of the second that move with the first",
          "[model][cards][modules]")
{
    Model model =
        readBulk("ModuleRigidJoint;5;13;1;24;2;1356\nModuleRigidJoint;6;;1;;2\nModuleRigidJoint;7;3;7;4;7;123");
    const ModuleRigidJointCard example = cardAt<ModuleRigidJointTable>(model, 0);
    REQUIRE((example.id == 5 && example.moduleIdA == 13 && example.nodeA == 1 && example.moduleIdB == 24 &&
             example.nodeB == 2));
    REQUIRE(example.componentsB == 1356);
    // A blank CB is all six components, a blank module the main Bulk Data.
    const ModuleRigidJointCard blank = cardAt<ModuleRigidJointTable>(model, 1);
    REQUIRE((blank.componentsB == 123456 && blank.moduleIdA == 0 && blank.moduleIdB == 0));
    // The same number of two modules is two points.
    REQUIRE((cardAt<ModuleRigidJointTable>(model, 2).nodeA == 7 && cardAt<ModuleRigidJointTable>(model, 2).nodeB == 7));
    requireRoundTrip(model);

    REQUIRE(referencesOf<ModuleRigidJointTable>(model, 0).fields.empty());
    const RecordingVisitor visitor = referencesOf<ModuleRigidJointTable>(model, 1);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodeA", "nodeB"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::Node));
    REQUIRE(unresolvedFields<ModuleRigidJointTable>(model, 1) == std::vector<std::string>{"nodeA", "nodeB"});
    REQUIRE(unresolvedFields<ModuleRigidJointTable>(model, 0).empty());

    Model grids =
        readBulk("Node;1;;0.;0.;0.\nNode;2;;1.;0.;0.\nModuleRigidJoint;6;;1;;2\nModuleRigidJoint;5;13;1;24;2;1356");
    Renumbering::apply(grids, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE(
        (cardAt<ModuleRigidJointTable>(grids, 0).nodeA == 11 && cardAt<ModuleRigidJointTable>(grids, 0).nodeB == 12));
    REQUIRE((cardAt<ModuleRigidJointTable>(grids, 1).nodeA == 1 &&
             cardAt<ModuleRigidJointTable>(grids, 1).nodeB == 2)); // in modules 13 and 24
}

// ------------------------------------------------------------------------------------------------------------- MDRROD

TEST_CASE("an MDRROD has ALPHA after its components and TREF on the continuation line", "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleRigidRod;14;13;1;24;2;2;;7.5E-6\nModuleRigidRod;15;;5;;6;;3;1.2E-5;20.\nModuleRigidRod;16;1;10;2;"
        "20\nModuleRigidRod;17;;5;;6;1;;;300.");
    const ModuleRigidRodCard example = cardAt<ModuleRigidRodTable>(model, 0);
    REQUIRE((example.id == 14 && example.moduleIdA == 13 && example.nodeA == 1 && example.moduleIdB == 24 &&
             example.nodeB == 2));
    REQUIRE((example.componentsAtA == 2 && example.componentsAtB == 0 && example.alphaCoefficient == 7.5e-6 &&
             example.referenceTemperature == 0.0));
    const ModuleRigidRodCard every = cardAt<ModuleRigidRodTable>(model, 1);
    REQUIRE((every.componentsAtA == 0 && every.componentsAtB == 3 && every.alphaCoefficient == 1.2e-5 &&
             every.referenceTemperature == 20.0));
    // Both components blank is the Lagrange method (the format reference allows it).
    const ModuleRigidRodCard lagrange = cardAt<ModuleRigidRodTable>(model, 2);
    REQUIRE((lagrange.componentsAtA == 0 && lagrange.componentsAtB == 0 && lagrange.alphaCoefficient == 0.0 &&
             lagrange.referenceTemperature == 0.0));
    const ModuleRigidRodCard tref = cardAt<ModuleRigidRodTable>(model, 3);
    REQUIRE((tref.componentsAtA == 1 && tref.alphaCoefficient == 0.0 && tref.referenceTemperature == 300.0));
    requireRoundTrip(model);

    REQUIRE(referencesOf<ModuleRigidRodTable>(model, 0).fields.empty());
    Model main = readBulk("Node;5;;0.;0.;0.\nNode;6;;1.;0.;0.\nModuleRigidRod;18;;5;;6;1");
    const RecordingVisitor visitor = referencesOf<ModuleRigidRodTable>(main);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodeA", "nodeB"});
    REQUIRE(unresolvedFields<ModuleRigidRodTable>(main).empty());
    Renumbering::apply(main, RenumberSpace::Node, Renumbering::offset(50));
    REQUIRE((cardAt<ModuleRigidRodTable>(main).nodeA == 55 && cardAt<ModuleRigidRodTable>(main).nodeB == 56));
}

TEST_CASE("an MDRROD refuses two dependent components, a translation above 3 and fields after TREF",
          "[model][cards][modules]")
{
    expectInvalid("ModuleRigidRod;0;11;1;21;2", "id");
    expectInvalid("ModuleRigidRod;14;-1;1;21;2", "moduleIdA");
    expectInvalid("ModuleRigidRod;14;11;1;21", "nodeB");
    expectInvalid("ModuleRigidRod;14;11;1;11;1", "nodeB");
    expectInvalid("ModuleRigidRod;14;11;1;21;2;4", "componentsAtA");
    expectInvalid("ModuleRigidRod;14;11;1;21;2;;4", "componentsAtB");
    expectInvalid("ModuleRigidRod;14;11;1;21;2;1;2", "componentsAtB");
    REQUIRE_NOTHROW(readBulk("ModuleRigidRod;14;11;1;12;1;1"));
}

// ------------------------------------------------------------------------------------------------------------- MDSEAM

TEST_CASE("an MDSEAM has the grid points of its ends or their coordinates, and a module for each patch",
          "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleSeam;452;197;;;33;38;;;20511;;64422;;;;;;17;29\n"
        "ModuleSeam;553;298;LINE1;Elements;50;51;52;53;;1.;;2.;3.;4.;5.;6.;7;8\n"
        "ModuleSeam;554;299;;;70;71;;;;1.5;;;;2.5\n"
        "ModuleSeam;555;300;;ShellProp;33;;;;20512;;64423");
    const ModuleSeamCard example = cardAt<ModuleSeamTable>(model, 0);
    REQUIRE((example.id == 452 && example.propId == 197 && example.seamLineName.empty() &&
             example.patchKind == ModuleSeamPatchType::Property));
    REQUIRE((example.startPatchIdA == 33 && example.startPatchIdB == 38 && example.endPatchIdA == 0 &&
             example.endPatchIdB == 0));
    REQUIRE((example.ends == ModuleSeamEnds::GridPoints && example.startNode == 20511 && example.endNode == 64422));
    REQUIRE((example.moduleIdA == 17 && example.moduleIdB == 29));
    const ModuleSeamCard alternate = cardAt<ModuleSeamTable>(model, 1);
    REQUIRE((alternate.seamLineName == "LINE1" && alternate.patchKind == ModuleSeamPatchType::Element &&
             alternate.hasCoordinates()));
    REQUIRE((alternate.endPatchIdA == 52 && alternate.endPatchIdB == 53 && alternate.startNode == 0 &&
             alternate.endNode == 0));
    REQUIRE(alternate.startXyz == Xyz{1.0, 2.0, 3.0});
    REQUIRE(alternate.endXyz == Xyz{4.0, 5.0, 6.0});
    REQUIRE((alternate.moduleIdA == 7 && alternate.moduleIdB == 8));
    // A coordinate may be blank: one of the six is enough to be the alternate format.
    const ModuleSeamCard some = cardAt<ModuleSeamTable>(model, 2);
    REQUIRE(some.hasCoordinates());
    REQUIRE(some.startXyz == Xyz{1.5, std::nullopt, std::nullopt});
    REQUIRE(some.endXyz == Xyz{2.5, std::nullopt, std::nullopt});
    // An IDBS that is blank is a two-sided patch of PSHELL properties.
    REQUIRE((cardAt<ModuleSeamTable>(model, 3).startPatchIdB == 0 &&
             cardAt<ModuleSeamTable>(model, 3).moduleIdA == 0));
    requireRoundTrip(model);
}

TEST_CASE("an MDSEAM looks a patch up in the main Bulk Data only, as a property or an element",
          "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleSeam;452;197;;;33;38;;;20511;;64422;;;;;;17;29\n"
        "ModuleSeam;555;300;;Elements;1;2;;;3;;4\n"
        "ModuleSeam;556;300;;;1;2;3;4;5;;6\n"
        "ModuleSeam;557;300;;;1;2;3;4;5;;6;;;;;;7");
    // The PID and the grid points are always looked up; a patch only when its module is the main Bulk Data.
    const RecordingVisitor modules = referencesOf<ModuleSeamTable>(model, 0);
    REQUIRE(modules.fields == std::vector<std::string>{"propId", "startNode", "endNode"});
    REQUIRE(modules.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node, ReferenceTarget::Node});
    const RecordingVisitor elements = referencesOf<ModuleSeamTable>(model, 1);
    REQUIRE(elements.fields ==
            std::vector<std::string>{"propId", "startPatchIdA", "startPatchIdB", "startNode", "endNode"});
    REQUIRE(elements.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Element,
                                                             ReferenceTarget::Element, ReferenceTarget::Node,
                                                             ReferenceTarget::Node});
    const RecordingVisitor properties = referencesOf<ModuleSeamTable>(model, 2);
    REQUIRE(properties.fields == std::vector<std::string>{"propId", "startPatchIdA", "startPatchIdB", "endPatchIdA",
                                                          "endPatchIdB", "startNode", "endNode"});
    std::vector<ReferenceTarget> propertyTargets(5, ReferenceTarget::Property);
    propertyTargets.insert(propertyTargets.end(), 2, ReferenceTarget::Node);
    REQUIRE(properties.targets == propertyTargets);
    // MODA holds IDAS and IDAE, MODB holds IDBS and IDBE.
    const RecordingVisitor split = referencesOf<ModuleSeamTable>(model, 3);
    REQUIRE(split.fields == std::vector<std::string>{"propId", "startPatchIdB", "endPatchIdB", "startNode", "endNode"});

    Model resolved = readBulk(
        "Node;3;;0.;0.;0.\n"
        "Node;4;;1.;0.;0.\n"
        "SeamProp;300;1;;16.\n"
        "ShellProp;1;1;0.01\n"
        "ShellProp;2;1;0.01\n"
        "ModuleSeam;556;300;;;1;2;;;3;;4");
    REQUIRE(unresolvedFields<ModuleSeamTable>(resolved).empty());
    Renumbering::apply(resolved, RenumberSpace::Property, Renumbering::offset(100));
    REQUIRE((cardAt<ModuleSeamTable>(resolved).startPatchIdA == 101 &&
             cardAt<ModuleSeamTable>(resolved).startPatchIdB == 102));
    REQUIRE(cardAt<ModuleSeamTable>(resolved).propId == 400); // the PSEAM is a property
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<ModuleSeamTable>(resolved).startNode == 1003 &&
             cardAt<ModuleSeamTable>(resolved).endNode == 1004));
}

TEST_CASE("an MDSEAM refuses what neither of its two formats has", "[model][cards][modules]")
{
    expectInvalid("ModuleSeam;0;5;;;1;2;;;3;;4", "id");
    expectInvalid("ModuleSeam;452;0;;;1;2;;;3;;4", "propId");
    oor::testing::requireModelFileRoundTrip(readModelText("ModuleSeam;452;5;TOOLONGNAME;;1;2;;;3;;4", true));
    expectInvalid("ModuleSeam;452;5;;BOGUS;1;2;;;3;;4", "patchKind");
    expectInvalid("ModuleSeam;452;5;;;;2;;;3;;4", "startPatchIdA");
    expectInvalid("ModuleSeam;452;5;;;1;-2;;;3;;4", "startPatchIdB");
    expectInvalid("ModuleSeam;452;5;;Elements;1;;;;3;;4", "startPatchIdB"); // two elements are needed
    expectInvalid("ModuleSeam;452;5;;Elements;1;1;;;3;;4", "startPatchIdB");
    expectInvalid("ModuleSeam;452;5;;Elements;1;2;7;7;3;;4", "endPatchIdB");
    expectInvalid("ModuleSeam;452;5;;;1;2", "startNode");
    expectInvalid("ModuleSeam;452;5;;;1;2;;;3", "endNode");
    expectInvalid("ModuleSeam;452;5;;;1;2;;;3;;4;;;;;;-1", "moduleIdA");
    // The same number is the same patch only in one module.
    REQUIRE_NOTHROW(readBulk("ModuleSeam;452;5;;Elements;1;1;;;3;;4;;;;;;7;8"));
    expectInvalid("ModuleSeam;452;5;;Elements;1;1;;;3;;4;;;;;;7;7", "startPatchIdB");
}

// ------------------------------------------------------------------------------------------------------------- MDWELD

TEST_CASE("an MDWELD has the layout of its type: patches, elements, grid point lists or vertex points",
          "[model][cards][modules]")
{
    Model model = readBulk(
        "ModuleWeld;101;8;207;PartPatch;;;;;24;36;;13;23\n"
        "ModuleWeld;102;5;407;ElemPatch;;;;;314;527;;13;23;;1.5;;2.5;;3.5\n"
        "ModuleWeld;103;5;407;ElemIdPatch;;;;;314;527;;13;12\n"
        "ModuleWeld;104;5;407;ElemIdPatch;;;;;314;;3;13\n"
        "ModuleWeld;105;26;241;NodeIdPatch;;;QuadTriaPatch;;;;;13;23;12;;24;;33;;37;42;53;57;64;4;6;9\n"
        "ModuleWeld;106;26;;AlignedNodes;113;262;;;;;;13;23");
    const ModuleWeldCard partpat = cardAt<ModuleWeldTable>(model, 0);
    REQUIRE((partpat.id == 101 && partpat.propId == 8 && partpat.spotNode == 207 &&
             partpat.type == ModuleWeldType::PartPatch));
    REQUIRE((partpat.patchIdA == 24 && partpat.patchIdB == 36 && partpat.moduleIdA == 13 && partpat.moduleIdB == 23));
    REQUIRE((partpat.matFrameId == ModuleWeldCard::NO_MAT_FRAME_ID && !partpat.location.has_value() &&
             !partpat.patchTypes.has_value()));
    const ModuleWeldCard elpat = cardAt<ModuleWeldTable>(model, 1);
    REQUIRE((elpat.type == ModuleWeldType::ElementPatch && elpat.patchIdA == 314 && elpat.patchIdB == 527));
    REQUIRE(elpat.location.has_value());
    REQUIRE(*elpat.location == std::array<double, 3>{1.5, 2.5, 3.5});
    const ModuleWeldCard elemid = cardAt<ModuleWeldTable>(model, 2);
    REQUIRE((elemid.type == ModuleWeldType::ElementId && elemid.patchIdB == 527 && elemid.spotModuleId == 0 &&
             elemid.moduleIdB == 12));
    // A blank SHIDB connects the point GS, in the module MODS, to the patch A.
    const ModuleWeldCard point = cardAt<ModuleWeldTable>(model, 3);
    REQUIRE((point.patchIdB == 0 && point.spotModuleId == 3 && point.moduleIdA == 13 && point.moduleIdB == 0));
    const ModuleWeldCard grids = cardAt<ModuleWeldTable>(model, 4);
    REQUIRE((grids.type == ModuleWeldType::GridId && grids.patchTypes == ModuleWeldPatchTypes::QuadTriangle));
    REQUIRE(grids.nodesA == std::array<base::EntityId, 8>{12, 24, 33, 37, 42, 53, 57, 64});
    REQUIRE(grids.nodesB == std::array<base::EntityId, 8>{4, 6, 9, 0, 0, 0, 0, 0});
    REQUIRE((grids.patchIdA == 0 && grids.patchIdB == 0 && grids.moduleIdA == 13 && grids.moduleIdB == 23));
    const ModuleWeldCard align = cardAt<ModuleWeldTable>(model, 5);
    REQUIRE(
        (align.type == ModuleWeldType::Align && align.spotNode == 0 && align.nodeA == 113 && align.nodeB == 262));
    requireRoundTrip(model);

    // The system, the piercing points and the location of the first line and the continuations.
    const Model detailed = readBulk(
        "ModuleWeld;107;8;;PartPatch;;;;5;24;36;;13;23;;1.5;;2.5;;3.5\n"
        "ModuleWeld;108;26;;NodeIdPatch;501;502;TriaTriaPatch;7;;;;2;3;1;;2;;3;;4;;;;;5;6;7;8");
    REQUIRE((cardAt<ModuleWeldTable>(detailed, 0).matFrameId == 5 &&
             cardAt<ModuleWeldTable>(detailed, 0).spotNode == 0));
    const ModuleWeldCard second = cardAt<ModuleWeldTable>(detailed, 1);
    REQUIRE((second.nodeA == 501 && second.nodeB == 502 &&
             second.patchTypes == ModuleWeldPatchTypes::TriangleTriangle));
    REQUIRE((second.matFrameId == 7 && second.moduleIdA == 2 && second.moduleIdB == 3));
    REQUIRE(second.nodesA == std::array<base::EntityId, 8>{1, 2, 3, 4, 0, 0, 0, 0});
    REQUIRE(second.nodesB == std::array<base::EntityId, 8>{5, 6, 7, 8, 0, 0, 0, 0});
    requireRoundTrip(detailed);
}

TEST_CASE("an MDWELD looks up the points and patches of the main Bulk Data and those of the module its type names",
          "[model][cards][modules]")
{
    Model modules = readBulk(
        "ModuleWeld;101;8;207;PartPatch;;;;;24;36;;13;23\n"
        "ModuleWeld;103;5;407;ElemIdPatch;;;;;314;527;;13;12\n"
        "ModuleWeld;104;5;407;ElemIdPatch;;;;;314;;3;13\n"
        "ModuleWeld;105;26;241;NodeIdPatch;;;QuadTriaPatch;;;;;13;23;12;;24;;33;;37;42;53;57;64;4;6;9\n"
        "ModuleWeld;106;26;;AlignedNodes;113;262;;;;;;13;23");
    // The PWELD is always looked up. GS is in the main Bulk Data, except for an ELEMID, where it is in the module MODS.
    REQUIRE(referencesOf<ModuleWeldTable>(modules, 0).fields == std::vector<std::string>{"propId", "spotNode"});
    REQUIRE(referencesOf<ModuleWeldTable>(modules, 1).fields == std::vector<std::string>{"propId", "spotNode"});
    REQUIRE(referencesOf<ModuleWeldTable>(modules, 2).fields == std::vector<std::string>{"propId"});
    REQUIRE(referencesOf<ModuleWeldTable>(modules, 3).fields == std::vector<std::string>{"propId", "spotNode"});
    REQUIRE(referencesOf<ModuleWeldTable>(modules, 4).fields == std::vector<std::string>{"propId"});

    Model main = readBulk(
        "ModuleWeld;201;8;207;PartPatch;501;502;;;24;36\n"
        "ModuleWeld;202;8;207;ElemPatch;;;;5;31;34\n"
        "ModuleWeld;203;8;207;ElemIdPatch;;;;;31\n"
        "ModuleWeld;204;8;;NodeIdPatch;;;TriaTriaPatch;;;;;;;1;;2;;3;;;;;;;4;5;6\n"
        "ModuleWeld;205;8;;AlignedNodes;113;262");
    const RecordingVisitor partpat = referencesOf<ModuleWeldTable>(main, 0);
    REQUIRE(partpat.fields == std::vector<std::string>{"propId", "spotNode", "nodeA", "nodeB", "patchIdA", "patchIdB"});
    REQUIRE(partpat.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node,
                                                            ReferenceTarget::Node, ReferenceTarget::Node,
                                                            ReferenceTarget::Property, ReferenceTarget::Property});
    const RecordingVisitor elpat = referencesOf<ModuleWeldTable>(main, 1);
    REQUIRE(elpat.fields == std::vector<std::string>{"propId", "spotNode", "matFrameId", "patchIdA", "patchIdB"});
    REQUIRE(elpat.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node,
                                                          ReferenceTarget::Coordinate, ReferenceTarget::Element,
                                                          ReferenceTarget::Element});
    const RecordingVisitor elemid = referencesOf<ModuleWeldTable>(main, 2);
    REQUIRE(elemid.fields == std::vector<std::string>{"propId", "spotNode", "patchIdA"});
    REQUIRE(elemid.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node, ReferenceTarget::Element});
    const RecordingVisitor gridid = referencesOf<ModuleWeldTable>(main, 3);
    REQUIRE(gridid.fields ==
            std::vector<std::string>{"propId", "nodeA1", "nodeA1", "nodeA1", "nodeB1", "nodeB1", "nodeB1"});
    REQUIRE(gridid.targets.size() == 7);
    REQUIRE(gridid.targets[0] == ReferenceTarget::Property);
    REQUIRE(gridid.targets[6] == ReferenceTarget::Node);
    const RecordingVisitor align = referencesOf<ModuleWeldTable>(main, 4);
    REQUIRE(align.fields == std::vector<std::string>{"propId", "nodeA", "nodeB"});

    Model resolved = readBulk(
        "Node;207;;0.;0.;0.\n"
        "Node;501;;0.;0.;0.\n"
        "Node;502;;0.;0.;0.\n"
        "WeldProp;8;3;1.\n"
        "ShellProp;24;1;0.01\n"
        "ShellProp;36;1;0.01\n"
        "ModuleWeld;201;8;207;PartPatch;501;502;;;24;36");
    REQUIRE(unresolvedFields<ModuleWeldTable>(resolved).empty());
    Renumbering::apply(resolved, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<ModuleWeldTable>(resolved).patchIdA == 1024 &&
             cardAt<ModuleWeldTable>(resolved).patchIdB == 1036));
    REQUIRE(cardAt<ModuleWeldTable>(resolved).propId == 1008); // the PWELD is a property
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(1));
    REQUIRE((cardAt<ModuleWeldTable>(resolved).spotNode == 208 &&
             cardAt<ModuleWeldTable>(resolved).nodeA == 502 && cardAt<ModuleWeldTable>(resolved).nodeB == 503));
}

TEST_CASE("an MDWELD refuses a field its type does not use and the values its type requires", "[model][cards][modules]")
{
    expectInvalid("ModuleWeld;0;8;207;PartPatch;;;;;24;36", "id");
    expectInvalid("ModuleWeld;101;0;207;PartPatch;;;;;24;36", "propId");
    expectInvalid("ModuleWeld;101;8;207;SPOT;;;;;24;36", "kind");
    expectInvalid("ModuleWeld;101;8;207;;;;;;24;36", "kind");
    expectInvalid("ModuleWeld;101;8;207;PartPatch;;;;-2;24;36", "matFrameId");
    expectInvalid("ModuleWeld;101;8;207;PartPatch;;;;;;36", "propIdA");
    expectInvalid("ModuleWeld;101;8;207;PartPatch;;;;;24", "propIdB");
    expectInvalid("ModuleWeld;101;8;207;PartPatch;;;;;24;24", "patchIdB");
    expectInvalid("ModuleWeld;101;8;;PartPatch;;;;;24;36", "spotNode"); // the location is GS, GA or XS, YS and ZS
    expectInvalid("ModuleWeld;101;8;207;PartPatch;;;;;24;36;;;;;1.;;2.", "xStation");
    // The cells of a layout that a type does not draw must be blank.
    expectInvalid("ModuleWeld;101;8;207;PartPatch;;;7;;24;36");
    expectInvalid("ModuleWeld;101;8;207;PartPatch;;;;;24;36;5"); // MODS is for the ELEMID
    expectInvalid("ModuleWeld;102;5;407;ElemPatch;;;;;314;527;5");
    expectInvalid(
        "ModuleWeld;103;5;407;ElemIdPatch;;;;;314;527;;;;;1.;;2.;;3."); // an ELEMID has no line of coordinates
    expectInvalid("ModuleWeld;106;26;5;AlignedNodes;113;262");          // an ALIGN has no GS
    expectInvalid("ModuleWeld;106;26;;AlignedNodes;113;262;NodeIdPatch");
    // ELEMID: a point to a patch has a blank SHIDB, a patch to a patch two different elements.
    REQUIRE_NOTHROW(readBulk("ModuleWeld;103;5;407;ElemIdPatch;;;;;314"));
    expectInvalid("ModuleWeld;103;5;407;ElemIdPatch;;;;;;527", "shellIdA");
    expectInvalid("ModuleWeld;103;5;407;ElemIdPatch;;;;;314;314", "patchIdB");
    REQUIRE_NOTHROW(readBulk("ModuleWeld;103;5;407;ElemIdPatch;;;;;314;314;;11;12"));
    // GRIDID: the type of the patches, and three points each (none for the second patch of a point to a patch).
    expectInvalid("ModuleWeld;105;26;241;NodeIdPatch;;;;;;;;13;23;12;;24;;33", "patchTypes");
    expectInvalid("ModuleWeld;105;26;241;NodeIdPatch;;;XX;;;;;13;23;12;;24;;33", "patchTypes");
    expectInvalid("ModuleWeld;105;26;241;NodeIdPatch;;;QuadTriaPatch;;;;;13;23;;;24;;33", "nodeA1");
    expectInvalid("ModuleWeld;105;26;241;NodeIdPatch;;;QuadTriaPatch;;;;;13;23;12;;24;;33", "nodeB1");
    expectInvalid("ModuleWeld;105;26;241;NodeIdPatch;;;QuadPatch;;;;;13;23;12;;24;;33;;;;;;;3", "nodeB1");
    REQUIRE_NOTHROW(readBulk("ModuleWeld;105;26;241;NodeIdPatch;;;QuadPatch;;;;;13;;12;;24;;33"));
    // ALIGN: the two vertex points, which differ when they are in one module.
    expectInvalid("ModuleWeld;106;26;;AlignedNodes;113", "nodeA");
    expectInvalid("ModuleWeld;106;26;;AlignedNodes;113;113", "nodeB");
    REQUIRE_NOTHROW(readBulk("ModuleWeld;106;26;;AlignedNodes;113;113;;;;;;13;23"));
    REQUIRE_THROWS_AS(
        readBulk("ModuleWeld;101;8;207;PartPatch;;;;;24;36\nModuleWeld;101;8;207;PartPatch;;;;;25;37"),
        base::DeckError);
}
