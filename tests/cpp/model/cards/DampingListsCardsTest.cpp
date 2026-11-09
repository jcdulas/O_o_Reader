// Checks of the damping and list cards: DAMPING, HYBDAMP and ROTHYBD (the damping specifications, with the Case
// Control RSDAMP and SEDAMP), ELIST (the element lists of an MFLUID, with the sign of the wetted side), ERPPNL (the
// panels of an equivalent radiated power calculation), CYAX and CYJOIN (the points of a cyclic symmetry model), CONTRLT
// (the thermal controller), BDYOR (the defaults of the surface elements) and VCCT (a crack of the virtual crack closure
// technique). Per card: the fields in their format reference positions, the defaults, the references the card lists,
// the renumbering of the spaces it owns, the refused values and the round trip through a model file.

#include "oor/model/cards/constraints/cyclicAxisNodes/CyclicAxisNodesTable.hpp"
#include "oor/model/cards/constraints/cyclicBoundaryJoin/CyclicBoundaryJoinTable.hpp"
#include "oor/model/cards/dynamics/hybridDamping/HybridDampingTable.hpp"
#include "oor/model/cards/dynamics/hybridDampingRotorOptions/HybridDampingRotorOptionsTable.hpp"
#include "oor/model/cards/dynamics/rotorHybridDamping/RotorHybridDampingTable.hpp"
#include "oor/model/cards/nonlinear_contact/crackClosureCrack/CrackClosureCrackTable.hpp"
#include "oor/model/cards/sets/radiatedPowerPanelSet/RadiatedPowerPanelSetTable.hpp"
#include "oor/model/cards/thermal/fluidMassElemList/FluidMassElemListTable.hpp"
#include "oor/model/cards/thermal/fluidVirtualMass/FluidVirtualMassTable.hpp"
#include "oor/model/cards/thermal/heatController/HeatControllerTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceDefaults/HeatSurfaceDefaultsTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceNodes/HeatSurfaceNodesTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceSide/HeatSurfaceSideTable.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
#include "support/ExplicitTestSupport.hpp"
#include "support/SlotRecord.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The references that name nothing in a model that was just read (the support function takes a model that stays).
std::size_t unresolvedIn(Model&& model)
{
    return testing::unresolvedIn(model);
}

// The number a command of the global Case Control section holds.
std::int64_t globalCommand(const Model& model, const char* name)
{
    const CaseEntry* entry = CaseControl::ownCommand(model.control().caseControl.global(), name);
    REQUIRE(entry != nullptr);
    return entry->value.integer;
}

// Records what a card names: the target and the ID, in the order of the card.
struct IdVisitor final : ReferenceVisitor
{
    std::vector<std::pair<ReferenceTarget, base::EntityId>> seen;
    base::EntityId visit(std::string_view, ReferenceTarget target, base::EntityId id) override
    {
        seen.emplace_back(target, id);
        return id;
    }
};

using Seen = std::vector<std::pair<ReferenceTarget, base::EntityId>>;

// The fields of `text`, separated by commas.
std::vector<std::string> fieldsOf(const std::string& text)
{
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true)
    {
        const std::size_t comma = text.find(',', start);
        fields.push_back(text.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        if (comma == std::string::npos)
            return fields;
        start = comma + 1;
    }
}

// The record of a VCCT given by the slots its card reader reads, in four parts separated by commas: the head (slots 1
// to 8), the second and the third group of eight slots (never blank), and the crack front from slot 25.
std::string vcct(const std::string& head, const std::string& second, const std::string& third, const std::string& front)
{
    constexpr std::size_t GROUP = 8;
    std::vector<std::string> slots;
    for (const std::string* part : {&head, &second, &third})
    {
        const std::vector<std::string> fields = fieldsOf(*part);
        slots.insert(slots.end(), fields.begin(), fields.end());
        slots.resize((slots.size() + GROUP - 1) / GROUP * GROUP);
    }
    const std::vector<std::string> crackFront = fieldsOf(front);
    slots.insert(slots.end(), crackFront.begin(), crackFront.end());
    return oor::testing::slotRecord("CrackClosureCrack", slots);
}

// The four tables of a VCCT, TABLEM1 10 to 13, for the decks that name them.
const std::string TABLES =
    "TableMaterial1;10;;;0.;0.;1.;2.;EndTable\n"
    "TableMaterial1;11;;;0.;0.;1.;2.;EndTable\n"
    "TableMaterial1;12;;;0.;0.;1.;2.;EndTable\n"
    "TableMaterial1;13;;;0.;0.;1.;2.;EndTable\n";

// A VCCT with everything in it.
std::string fullCrackClosureCrack()
{
    return vcct("204,2,2,1,2,1", "0.4,1750.,,,,,1350.,1100.", "10,11,,,,,12,13", "41,42,Through,50,By,2,63");
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// DAMPING, HYBDAMP and ROTHYBD
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a DAMPING has the format reference defaults and the transient frequencies on its second line",
          "[model][cards][damping]")
{
    Model model = readBulk(
        "HybridDampingRotorOptions;2450;0.045;;;;;No\n"
        "HybridDampingRotorOptions;101\n"
        "HybridDampingRotorOptions;102;0.03;0.001;2.E-5;201;0.5;No;100.;200.;300.\n"
        "HybridDampingRotorOptions;103;0.03;;;;;;628.3");
    const HybridDampingRotorOptionsCard sampleCard = cardAt<HybridDampingRotorOptionsTable>(model, 0);
    REQUIRE((sampleCard.id == 2450 && sampleCard.structuralDamping == 0.045 && sampleCard.alphaCoefficient1 == 0.0 &&
             sampleCard.alphaCoefficient2 == 0.0 && sampleCard.hybridDampingId == 0));
    REQUIRE((sampleCard.structuralDampingFactor == 1.0 && sampleCard.rotorSeparation == RotorSeparation::No));
    REQUIRE((sampleCard.structuralDampingFrequency == 0.0 && sampleCard.materialDampingFrequency == 0.0 &&
             sampleCard.hybridDampingFrequency == 0.0));
    const HybridDampingRotorOptionsCard bare = cardAt<HybridDampingRotorOptionsTable>(model, 1);
    REQUIRE((bare.id == 101 && bare.structuralDamping == 0.0 && bare.hybridDampingId == 0 &&
             bare.structuralDampingFactor == 1.0));
    REQUIRE(bare.rotorSeparation == RotorSeparation::Yes); // a blank ROTSEP is YES
    const HybridDampingRotorOptionsCard full = cardAt<HybridDampingRotorOptionsTable>(model, 2);
    REQUIRE((full.structuralDamping == 0.03 && full.alphaCoefficient1 == 1.0e-3 && full.alphaCoefficient2 == 2.0e-5 &&
             full.hybridDampingId == 201));
    REQUIRE((full.structuralDampingFactor == 0.5 && full.rotorSeparation == RotorSeparation::No));
    REQUIRE((full.structuralDampingFrequency == 100.0 && full.materialDampingFrequency == 200.0 &&
             full.hybridDampingFrequency == 300.0));
    const HybridDampingRotorOptionsCard transient = cardAt<HybridDampingRotorOptionsTable>(model, 3);
    REQUIRE((transient.structuralDamping == 0.03 && transient.structuralDampingFrequency == 628.3 &&
             transient.materialDampingFrequency == 0.0 && transient.hybridDampingFrequency == 0.0));
    REQUIRE((HybridDampingRotorOptionsTable::INFO.category == CardCategory::HybridDampingRotorOptionsSpecification &&
             HybridDampingRotorOptionsTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The defaults are blank fields, so a card of defaults is its ID alone and the second line is left out.

    // The only reference is the HYBDAMP, and a blank field names nothing.
    IdVisitor visitor;
    model.table<HybridDampingRotorOptionsTable>().visitReferences(2, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::HybridDamping, 201}});
    visitor = IdVisitor{};
    model.table<HybridDampingRotorOptionsTable>().visitReferences(0, visitor);
    REQUIRE(visitor.seen.empty());

    // The ID is unique among the DAMPING cards and its own space: a HYBDAMP 5 and a DAMPING 5 coexist.
    REQUIRE_THROWS_AS(readBulk("HybridDampingRotorOptions;5\nHybridDampingRotorOptions;5;0.1"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("HybridDampingRotorOptions;5\nHybridDamping;5;1;2"));

    expectInvalid("HybridDampingRotorOptions", "id");
    expectInvalid("HybridDampingRotorOptions;0", "id");
    expectInvalid("HybridDampingRotorOptions;-3", "id");
    expectInvalid("HybridDampingRotorOptions;1;LOW", "structuralDamping");
    expectInvalid("HybridDampingRotorOptions;1;;LOW", "alphaCoefficient1");
    expectInvalid("HybridDampingRotorOptions;1;;;LOW", "alphaCoefficient2");
    expectInvalid("HybridDampingRotorOptions;1;;;;-2", "hybridDampingId");
    expectInvalid("HybridDampingRotorOptions;1;;;;;LOW", "structuralDampingFactor");
    expectInvalid("HybridDampingRotorOptions;1;;;;;;MAYBE", "rotorSeparation");
    expectInvalid("HybridDampingRotorOptions;1;;;;;;Yes;-1.", "structuralDampingFrequency");
    expectInvalid("HybridDampingRotorOptions;1;;;;;;Yes;;-1.", "materialDampingFrequency");
    expectInvalid("HybridDampingRotorOptions;1;;;;;;Yes;;;-1.", "hybridDampingFrequency");
}

TEST_CASE("a HYBDAMP names the method of its modes and the TABDMP1 of its damping", "[model][cards][damping]")
{
    Model model = readBulk(
        "HybridDamping;2610;2640;2641;No;Yes\nHybridDamping;102;5;6\nHybridDamping;103;5;6;Yes;No\nHybridDamping;104;5;"
        "6;Yes");
    const HybridDampingCard sampleCard = cardAt<HybridDampingTable>(model, 0);
    REQUIRE((sampleCard.id == 2610 && sampleCard.methodId == 2640 && sampleCard.modalDampingTableId == 2641));
    REQUIRE((sampleCard.dampingInStiffness == HybridDampingOption::No &&
             sampleCard.printEigenvalues == HybridDampingOption::Yes));
    const HybridDampingCard bare = cardAt<HybridDampingTable>(model, 1);
    REQUIRE((bare.dampingInStiffness == HybridDampingOption::No &&
             bare.printEigenvalues == HybridDampingOption::No)); // both default to NO
    const HybridDampingCard lower = cardAt<HybridDampingTable>(model, 2);
    REQUIRE((lower.dampingInStiffness == HybridDampingOption::Yes &&
             lower.printEigenvalues == HybridDampingOption::No)); // any case
    REQUIRE((cardAt<HybridDampingTable>(model, 3).dampingInStiffness == HybridDampingOption::Yes));
    REQUIRE((HybridDampingTable::INFO.category == CardCategory::HybridDamping &&
             HybridDampingTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // METHOD is an eigenvalue method and SDAMP a TABDMP1.
    RecordingVisitor visitor;
    model.table<HybridDampingTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"methodId", "modalDampingTableId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Method, ReferenceTarget::HybridDampingRotorOptionsTable});
    REQUIRE(unresolvedIn(model) == 8); // the four cards name a method and a table that this deck has not

    Model resolved = readBulk(
        "HybridDamping;101;102;1001\nLanczosEigenMethod;102;;;6\nTableModalDamping1;1001;CriticalDamping;0.;0.02;1.E+3;"
        "0.02;EndTable");
    REQUIRE(unresolvedIn(resolved) == 0);

    expectInvalid("HybridDamping;0;1;2", "id");
    expectInvalid("HybridDamping;1;0;2", "methodId");
    expectInvalid("HybridDamping;1;2;0", "modalDampingTableId");
    expectInvalid("HybridDamping;1;2", "modalDampingTableId"); // required
    expectInvalid("HybridDamping;1", "methodId");              // required
    expectInvalid("HybridDamping;1;2;3;MAYBE", "dampingInStiffness");
    expectInvalid("HybridDamping;1;2;3;No;MAYBE", "printEigenvalues");
}

TEST_CASE("the DAMPING, HYBDAMP and ROTHYBD cards name each other and the Case Control and follow a renumbering",
          "[model][cards][damping]")
{
    Model model = readModelText(
        "[case]\n"
        "RotorDampingSelection;100\n"
        "SuperelementDampingSelection;101\n"
        "[model]\n"
        "HybridDampingRotorOptions;100;0.03\n"
        "HybridDampingRotorOptions;101;;;;201\n"
        "HybridDamping;201;102;1001\n"
        "HybridDamping;202;102;1001;Yes;Yes\n"
        "LanczosEigenMethod;102;;;6\n"
        "TableModalDamping1;1001;CriticalDamping;0.;0.02;1.E+3;0.02;EndTable\n"
        "RotorHybridDamping;1;201;2;202\n"
        "Node;1\n"
        "Node;2\n"
        "RotorLineNodes;1;1;2\n"
        "RotorLineNodes;2;1;2\n",
        true);
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE((globalCommand(model, "RotorDampingSelection") == 100 &&
             globalCommand(model, "SuperelementDampingSelection") == 101));

    // The HYBDAMP numbers are rewritten in the DAMPING that names one and in the ROTHYBD; the rotors (ROTORG cards, a
    // space of their own) are left alone.
    REQUIRE(Renumbering::apply(model, RenumberSpace::HybridDamping, Renumbering::offset(10)) == 2);
    REQUIRE((cardAt<HybridDampingTable>(model, 0).id == 211 && cardAt<HybridDampingTable>(model, 1).id == 212));
    REQUIRE(cardAt<HybridDampingRotorOptionsTable>(model, 1).hybridDampingId == 211);
    REQUIRE(cardAt<HybridDampingRotorOptionsTable>(model, 0).hybridDampingId == 0);
    REQUIRE((cardAt<RotorHybridDampingTable>(model).pairs == std::vector<RotorHybridPair>{{1, 211}, {2, 212}}));

    // The Case Control RSDAMP and SEDAMP follow the DAMPING numbers.
    REQUIRE(Renumbering::apply(model, RenumberSpace::HybridDampingRotorOptionsSpecification,
                               Renumbering::offset(1000)) == 2);
    REQUIRE((cardAt<HybridDampingRotorOptionsTable>(model, 0).id == 1100 &&
             cardAt<HybridDampingRotorOptionsTable>(model, 1).id == 1101));
    REQUIRE((globalCommand(model, "RotorDampingSelection") == 1100 &&
             globalCommand(model, "SuperelementDampingSelection") == 1101));

    // The method and the table of a HYBDAMP follow their own spaces.
    Renumbering::apply(model, RenumberSpace::Method, Renumbering::offset(5));
    Renumbering::apply(model, RenumberSpace::HybridDampingRotorOptionsTable, Renumbering::offset(1));
    REQUIRE((cardAt<HybridDampingTable>(model, 0).methodId == 107 &&
             cardAt<HybridDampingTable>(model, 0).modalDampingTableId == 1002));
    REQUIRE(unresolvedIn(model) == 0);

    // What names nothing is reported: the command, the HYBRID, the method, the table, and the rotor and the HYBDAMP of
    // the ROTHYBD.
    Model dangling = readModelText(
        "[case]\nRotorDampingSelection;9\n[model]\nHybridDampingRotorOptions;100;;;;5\nHybridDamping;6;7;"
        "8\nRotorHybridDamping;1;9\n",
        true);
    REQUIRE(unresolvedIn(dangling) == 6);
    // The two rotors, the HYBDAMP 16, the method and the table.
    REQUIRE(unresolvedIn(readBulk("RotorHybridDamping;1;15;2;16\nHybridDamping;15;1;2")) == 5);
}

TEST_CASE("a ROTHYBD lists the pairs of a rotor and its HYBDAMP over any number of lines", "[model][cards][damping]")
{
    Model model = readBulk(
        "RotorHybridDamping;3;4205\n"
        "RotorHybridDamping;41;4101;42;4102;43;4103\n"
        "RotorHybridDamping;1;11;2;12;3;13;4;14;5;15;6;16\n"
        "RotorHybridDamping;7;70");
    REQUIRE((cardAt<RotorHybridDampingTable>(model, 0).pairs == std::vector<RotorHybridPair>{{3, 4205}}));
    REQUIRE((cardAt<RotorHybridDampingTable>(model, 1).pairs ==
             std::vector<RotorHybridPair>{{41, 4101}, {42, 4102}, {43, 4103}}));
    // The fifth pair is on the second line, in fields 9 and 10.
    REQUIRE((cardAt<RotorHybridDampingTable>(model, 2).pairs ==
             std::vector<RotorHybridPair>{{1, 11}, {2, 12}, {3, 13}, {4, 14}, {5, 15}, {6, 16}}));
    REQUIRE((RotorHybridDampingTable::INFO.category == CardCategory::Other &&
             RotorHybridDampingTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // A pair of blank fields in the middle is padding.
    REQUIRE((cardAt<RotorHybridDampingTable>(readBulk("RotorHybridDamping;1;11;;;3;13"), 0).pairs ==
             std::vector<RotorHybridPair>{{1, 11}, {3, 13}}));

    // The rotor of a pair is a rotor of the model (a ROTORG, ROTOR, ROTORAX or ROTORSE) and its HYBDAMP a hybrid
    // damping, in the order of the pairs.
    IdVisitor visitor;
    model.table<RotorHybridDampingTable>().visitReferences(1, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::Rotor, 41},
                                 {ReferenceTarget::HybridDamping, 4101},
                                 {ReferenceTarget::Rotor, 42},
                                 {ReferenceTarget::HybridDamping, 4102},
                                 {ReferenceTarget::Rotor, 43},
                                 {ReferenceTarget::HybridDamping, 4103}});
    // Cards with no ID of their own may repeat.
    REQUIRE_NOTHROW(readBulk("RotorHybridDamping;1;15\nRotorHybridDamping;1;15"));

    expectInvalid("RotorHybridDamping;1", "hybridDampingId"); // a pair has both numbers
    expectInvalid("RotorHybridDamping;1;15;2", "hybridDampingId");
    expectInvalid("RotorHybridDamping;;15", "rotorId");
    expectInvalid("RotorHybridDamping;0;15", "rotorId");
    expectInvalid("RotorHybridDamping;-1;15", "rotorId");
    expectInvalid("RotorHybridDamping;1;0", "hybridDampingId");
    expectInvalid("RotorHybridDamping;1;BAD", "hybridDampingId");
}

// ---------------------------------------------------------------------------------------------------------------------
// ELIST
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an ELIST keeps the sign of an element and the elements of a THRU range as written", "[model][cards][elist]")
{
    Model model = readBulk(
        "FluidMassElemList;2705;1305;-1318;1330;Through;1345;1372\n"
        "FluidMassElemList;4;-10;Through;-20;5\n"
        "FluidMassElemList;5;1;2;3;4;5;6;7;8;Through;20;30\n"
        "FluidMassElemList;6;-7");
    const FluidMassElemListCard sampleCard = cardAt<FluidMassElemListTable>(model, 0);
    REQUIRE(sampleCard.id == 2705);
    REQUIRE((sampleCard.elements == std::vector<IdRun>{{1305, 1305}, {-1318, -1318}, {1330, 1345}, {1372, 1372}}));
    // A range of the other side is written with the sign on both ends, the larger magnitude last.
    REQUIRE((cardAt<FluidMassElemListTable>(model, 1).elements == std::vector<IdRun>{{-10, -20}, {5, 5}}));
    REQUIRE((cardAt<FluidMassElemListTable>(model, 2).elements ==
             std::vector<IdRun>{{1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}, {7, 7}, {8, 20}, {30, 30}}));
    REQUIRE((cardAt<FluidMassElemListTable>(model, 3).elements == std::vector<IdRun>{{-7, -7}}));
    REQUIRE((FluidMassElemListTable::INFO.category == CardCategory::ElementList &&
             FluidMassElemListTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The lists are ElementInRange references with the absolute value: an element the model has not is not reported.
    IdVisitor visitor;
    model.table<FluidMassElemListTable>().visitReferences(0, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::ElementInRange, 1305},
                                 {ReferenceTarget::ElementInRange, 1318},
                                 {ReferenceTarget::ElementInRange, 1330},
                                 {ReferenceTarget::ElementInRange, 1345},
                                 {ReferenceTarget::ElementInRange, 1372}});
    visitor = IdVisitor{};
    model.table<FluidMassElemListTable>().visitReferences(1, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::ElementInRange, 10},
                                 {ReferenceTarget::ElementInRange, 20},
                                 {ReferenceTarget::ElementInRange, 5}});
    REQUIRE(unresolvedIn(model) == 0);

    // A renumbering of the elements moves the magnitude and keeps the side.
    Model quads = readBulk(
        "Node;1\n"
        "Node;2;;1.\n"
        "Node;3;;1.;1.\n"
        "Node;4;;0.;1.\n"
        "ShellQuad4;1305;1;1;2;3;4\n"
        "ShellQuad4;1318;1;1;2;3;4\n"
        "ShellProp;1;1;0.1\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "FluidMassElemList;2705;1305;-1318;1330;Through;1345;1372\n"
        "FluidMassElemList;4;-10;Through;-20");
    Renumbering::apply(quads, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE((cardAt<FluidMassElemListTable>(quads, 0).elements ==
             std::vector<IdRun>{{2305, 2305}, {-2318, -2318}, {2330, 2345}, {2372, 2372}}));
    REQUIRE((cardAt<FluidMassElemListTable>(quads, 1).elements == std::vector<IdRun>{{-1010, -1020}}));
    REQUIRE(cardAt<FluidMassElemListTable>(quads, 0).id == 2705); // the list number is not an element number
    REQUIRE(unresolvedIn(quads) == 0);

    // The text keeps the list as written; with the THRU option the runs of one side become ranges, a pair stays a pair.
    const Model listed = readBulk(
        "FluidMassElemList;6;1;2;3;4;-7;-8;-9\nFluidMassElemList;7;12;13\nFluidMassElemList;8;-4;-5;-"
        "6\nFluidMassElemList;9;1;-2;-3;4");
    oor::testing::requireModelFileRoundTrip(listed);
    // A range is never split over two lines and its THRU is never the first field of a line.
    const Model split = readBulk("FluidMassElemList;9;1;3;5;7;9;11;13;Through;20");

    oor::testing::requireModelFileRoundTrip(split);
    REQUIRE(cardAt<FluidMassElemListTable>(readModelText(oor::testing::writeModelText(split), true)).elements ==
            cardAt<FluidMassElemListTable>(split).elements);

    REQUIRE_THROWS_AS(readBulk("FluidMassElemList;5;1\nFluidMassElemList;5;2"), base::DeckError);

    expectInvalid("FluidMassElemList;0;1", "id");
    expectInvalid("FluidMassElemList;1", "elemId1");
    expectInvalid("FluidMassElemList;1;0", "elemId1");
    expectInvalid("FluidMassElemList;1;5;Through", "through");
    expectInvalid("FluidMassElemList;1;Through;5", "through");
    expectInvalid("FluidMassElemList;1;5;Through;6;Through;9", "through");
    expectInvalid("FluidMassElemList;1;5;Through;-8", "through"); // the ends of a range have the same sign
    expectInvalid("FluidMassElemList;1;-5;Through;8", "through");
    expectInvalid("FluidMassElemList;1;8;Through;5", "through");   // the end is not below the start
    expectInvalid("FluidMassElemList;1;-8;Through;-5", "through"); // in magnitude, for the other side too
    expectInvalid("FluidMassElemList;1;LOW", "elemId");
}

TEST_CASE("an MFLUID names the ELIST cards of its wetted elements and follows their renumbering",
          "[model][cards][elist]")
{
    Model model = readBulk(
        "FluidVirtualMass;4;;;1.E+3;3;4;NoSymmetry;NoSymmetry\nFluidMassElemList;3;1;2\nFluidMassElemList;4;-2");
    REQUIRE((cardAt<FluidVirtualMassTable>(model).oneSideElemListId == 3 &&
             cardAt<FluidVirtualMassTable>(model).bothSidesElemListId == 4));
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(readBulk("FluidVirtualMass;4;;;1.E+3;3;4;NoSymmetry;NoSymmetry\nFluidMassElemList;3;1;2")) ==
            1); // ELIST 4 is missing

    REQUIRE(Renumbering::apply(model, RenumberSpace::ElementList, Renumbering::offset(10)) == 2);
    REQUIRE((cardAt<FluidMassElemListTable>(model, 0).id == 13 && cardAt<FluidMassElemListTable>(model, 1).id == 14));
    REQUIRE((cardAt<FluidVirtualMassTable>(model).oneSideElemListId == 13 &&
             cardAt<FluidVirtualMassTable>(model).bothSidesElemListId == 14));
    REQUIRE(unresolvedIn(model) == 0);
    // The elements of the lists are not renumbered with the lists.
    REQUIRE((cardAt<FluidMassElemListTable>(model, 1).elements == std::vector<IdRun>{{-2, -2}}));
}

// ---------------------------------------------------------------------------------------------------------------------
// ERPPNL
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an ERPPNL lists the label and the SET3 of any number of panels", "[model][cards][erppnl]")
{
    Model model = readBulk(
        "RadiatedPowerPanelSet;CAP;1;PLATE;14\n"
        "RadiatedPowerPanelSet;CAP;1;PLATE;14;FLOOR;2;WALL;3;TAIL;4;Nose;5\n"
        "RadiatedPowerPanelSet;Door;7\n"
        "TypedListSet3;1;GeomPoint;1;2\n"
        "TypedListSet3;14;GeomPoint;3\n"
        "TypedListSet3;2;GeomPoint;1\n"
        "TypedListSet3;3;GeomPoint;4\n"
        "TypedListSet3;4;GeomPoint;5\n"
        "TypedListSet3;5;GeomPoint;6\n"
        "TypedListSet3;7;GeomPoint;7");
    const RadiatedPowerPanelSetCard sampleCard = cardAt<RadiatedPowerPanelSetTable>(model, 0);
    REQUIRE((sampleCard.panels == std::vector<RadiatedPowerPanel>{{"CAP", 1}, {"PLATE", 14}}));
    // The fifth panel is in fields 9 and 10; a label is kept as written.
    REQUIRE((cardAt<RadiatedPowerPanelSetTable>(model, 1).panels ==
             std::vector<RadiatedPowerPanel>{
                 {"CAP", 1}, {"PLATE", 14}, {"FLOOR", 2}, {"WALL", 3}, {"TAIL", 4}, {"Nose", 5}}));
    REQUIRE((cardAt<RadiatedPowerPanelSetTable>(model, 2).panels == std::vector<RadiatedPowerPanel>{{"Door", 7}}));
    REQUIRE((RadiatedPowerPanelSetTable::INFO.category == CardCategory::Other &&
             RadiatedPowerPanelSetTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // The sets are references; the labels are not. Every SET3 of the deck exists (the sets list points here, which
    // name no card, so that the deck has no other reference).
    IdVisitor visitor;
    model.table<RadiatedPowerPanelSetTable>().visitReferences(0, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::Set, 1}, {ReferenceTarget::Set, 14}});
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(readBulk("RadiatedPowerPanelSet;CAP;1;PLATE;14\nTypedListSet3;1;GeomPoint;1")) == 1);

    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(100));
    REQUIRE((cardAt<RadiatedPowerPanelSetTable>(model, 0).panels ==
             std::vector<RadiatedPowerPanel>{{"CAP", 101}, {"PLATE", 114}}));
    REQUIRE(unresolvedIn(model) == 0);

    // Cards with no ID of their own may repeat.
    REQUIRE_NOTHROW(readBulk("RadiatedPowerPanelSet;A;1\nRadiatedPowerPanelSet;A;1"));

    expectInvalid("RadiatedPowerPanelSet;CAP", "panelSetId");
    expectInvalid("RadiatedPowerPanelSet;CAP;0", "panelSetId");
    expectInvalid("RadiatedPowerPanelSet;CAP;-4", "panelSetId");
    expectInvalid("RadiatedPowerPanelSet;;1", "panelName");
    REQUIRE_NOTHROW(readBulk("RadiatedPowerPanelSet;TOOLONGNAME;1"));
    expectInvalid("RadiatedPowerPanelSet;A;1;B", "panelSetId");
    expectInvalid("RadiatedPowerPanelSet;A;LOW", "panelSetId");
}

// ---------------------------------------------------------------------------------------------------------------------
// CYAX and CYJOIN
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a CYAX lists points and THRU ranges, and its points are nodes", "[model][cards][cyclic]")
{
    Model model = readBulk(
        "CyclicAxisNodes;1406;1431;Through;1449;1463;1402\nCyclicAxisNodes;5\nNode;1406\nNode;1463\nNode;1402\nNode;5");
    REQUIRE((cardAt<CyclicAxisNodesTable>(model, 0).grids ==
             std::vector<IdRun>{{1406, 1406}, {1431, 1449}, {1463, 1463}, {1402, 1402}}));
    REQUIRE((cardAt<CyclicAxisNodesTable>(model, 1).grids == std::vector<IdRun>{{5, 5}}));
    REQUIRE((CyclicAxisNodesTable::INFO.category == CardCategory::Other &&
             CyclicAxisNodesTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // A single point must exist, the ends of a range may not (the gaps are skipped).
    IdVisitor visitor;
    model.table<CyclicAxisNodesTable>().visitReferences(0, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::Node, 1406},
                                 {ReferenceTarget::NodeInRange, 1431},
                                 {ReferenceTarget::NodeInRange, 1449},
                                 {ReferenceTarget::Node, 1463},
                                 {ReferenceTarget::Node, 1402}});
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(readBulk("CyclicAxisNodes;5;Through;8;9\nNode;5")) == 1); // 9
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<CyclicAxisNodesTable>(model, 0).grids ==
             std::vector<IdRun>{{1506, 1506}, {1531, 1549}, {1563, 1563}, {1502, 1502}}));

    // Several cards may repeat (the format reference does not limit the deck to one).
    REQUIRE_NOTHROW(readBulk("CyclicAxisNodes;1\nCyclicAxisNodes;2"));

    // The text keeps the list as written; with the THRU option consecutive points become a range. A range is never
    // split over two lines and its THRU is never the first field of a line.
    const Model listed = readBulk("CyclicAxisNodes;1;2;3;4;5;9");
    oor::testing::requireModelFileRoundTrip(listed);
    const Model split = readBulk("CyclicAxisNodes;1;3;5;7;9;11;13;15;Through;20");

    oor::testing::requireModelFileRoundTrip(split);
    REQUIRE(cardAt<CyclicAxisNodesTable>(readModelText(oor::testing::writeModelText(split), true)).grids ==
            cardAt<CyclicAxisNodesTable>(split).grids);

    expectInvalid("CyclicAxisNodes;0", "nodes");
    expectInvalid("CyclicAxisNodes;-3", "nodes");
    expectInvalid("CyclicAxisNodes;5;Through", "Through");
    expectInvalid("CyclicAxisNodes;Through;5", "Through");
    expectInvalid("CyclicAxisNodes;5;Through;3", "nodes");
    expectInvalid("CyclicAxisNodes;5;Through;6;Through;9", "Through");
    expectInvalid("CyclicAxisNodes;5;LOW", "nodes");
}

TEST_CASE("a CYJOIN has a side, the normal degree of freedom of dihedral problems and a list of points",
          "[model][cards][cyclic]")
{
    Model model = readBulk(
        "CyclicBoundaryJoin;1;Translation2;2204;2210;2215;Through;2235;2251;2260;Through;2272\n"
        "CyclicBoundaryJoin;2;;10;20\n"
        "CyclicBoundaryJoin;1;Rectangular;1\n"
        "CyclicBoundaryJoin;2;Translation1;3\n"
        "CyclicBoundaryJoin;2;Spherical;4\n"
        "CyclicBoundaryJoin;1;Translation3;5\n"
        "CyclicBoundaryJoin;1;Cylindrical;6");
    const CyclicBoundaryJoinCard sampleCard = cardAt<CyclicBoundaryJoinTable>(model, 0);
    REQUIRE(sampleCard.sideCode == 1);
    REQUIRE(sampleCard.system == CyclicBoundaryJoinSystem::T2);
    REQUIRE(
        (sampleCard.grids == std::vector<IdRun>{{2204, 2204}, {2210, 2210}, {2215, 2235}, {2251, 2251}, {2260, 2272}}));
    REQUIRE((cardAt<CyclicBoundaryJoinTable>(model, 1).sideCode == 2 &&
             !cardAt<CyclicBoundaryJoinTable>(model, 1).system)); // C may be blank
    REQUIRE(cardAt<CyclicBoundaryJoinTable>(model, 2).system == CyclicBoundaryJoinSystem::Rectangular); // any case
    REQUIRE(cardAt<CyclicBoundaryJoinTable>(model, 3).system == CyclicBoundaryJoinSystem::T1);
    REQUIRE(cardAt<CyclicBoundaryJoinTable>(model, 4).system == CyclicBoundaryJoinSystem::Spherical);
    REQUIRE(cardAt<CyclicBoundaryJoinTable>(model, 5).system == CyclicBoundaryJoinSystem::T3);
    REQUIRE(cardAt<CyclicBoundaryJoinTable>(model, 6).system == CyclicBoundaryJoinSystem::Cylindrical);
    REQUIRE((CyclicBoundaryJoinTable::INFO.category == CardCategory::Other &&
             CyclicBoundaryJoinTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // The words are written as the format reference spells them, and a blank C is a blank.

    // Every point of the list must be defined; the ends of a range may not be.
    IdVisitor visitor;
    model.table<CyclicBoundaryJoinTable>().visitReferences(0, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::Node, 2204},
                                 {ReferenceTarget::Node, 2210},
                                 {ReferenceTarget::NodeInRange, 2215},
                                 {ReferenceTarget::NodeInRange, 2235},
                                 {ReferenceTarget::Node, 2251},
                                 {ReferenceTarget::NodeInRange, 2260},
                                 {ReferenceTarget::NodeInRange, 2272}});
    // A scalar point is a point of the node space too.
    REQUIRE(unresolvedIn(readBulk("CyclicBoundaryJoin;1;Translation1;5;6\nScalarPoint;5\nNode;6")) == 0);
    REQUIRE(unresolvedIn(readBulk("CyclicBoundaryJoin;1;Translation1;5;6\nScalarPoint;5")) == 1);
    Model renumbered = readBulk("CyclicBoundaryJoin;1;;5;Through;8\nNode;5");
    Renumbering::apply(renumbered, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE((cardAt<CyclicBoundaryJoinTable>(renumbered).grids == std::vector<IdRun>{{15, 18}}));

    // A side may have several cards (the lists of a side are one logical entry).
    REQUIRE_NOTHROW(readBulk("CyclicBoundaryJoin;1;;1\nCyclicBoundaryJoin;1;;2"));

    expectInvalid("CyclicBoundaryJoin;0;Translation2;1", "sideCode");
    expectInvalid("CyclicBoundaryJoin;3;Translation2;1", "sideCode");
    expectInvalid("CyclicBoundaryJoin;;Translation2;1", "sideCode");
    expectInvalid("CyclicBoundaryJoin;1;X9;1", "system");
    expectInvalid("CyclicBoundaryJoin;1;T4;1", "system");
    expectInvalid("CyclicBoundaryJoin;1;Translation2", "nodes");
    expectInvalid("CyclicBoundaryJoin;1;Translation2;0", "nodes");
    expectInvalid("CyclicBoundaryJoin;1;Translation2;5;Through", "Through");
    expectInvalid("CyclicBoundaryJoin;1;Translation2;5;Through;3", "nodes");
}

// ---------------------------------------------------------------------------------------------------------------------
// CONTRLT
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a CONTRLT is a thermal controller with a sensor, a dead band and a process type", "[model][cards][contrlt]")
{
    Model model = readBulk(
        "HeatController;1702;1716;Temperature;Thermostat;16.5;21.5;6;1.;0.3;0.08;0.\n"
        "HeatController;1703;1716;;;16.5;21.5;1\n"
        "HeatController;1704;5;Temperature;Thermostat;0.;1.;11");
    const HeatControllerCard sampleCard = cardAt<HeatControllerTable>(model, 0);
    REQUIRE(
        (sampleCard.id == 1702 && sampleCard.sensorNode == 1716 && sampleCard.sensorForm == SensorForm::Temperature));
    REQUIRE((sampleCard.controllerKind == ControlType::Thermostat && sampleCard.lowerLimit == 16.5 &&
             sampleCard.upperLimit == 21.5 && sampleCard.controlLaw == 6));
    REQUIRE((sampleCard.initialState == 1.0 && sampleCard.timeStep == 0.3 && sampleCard.delayTime == 0.08 &&
             sampleCard.timeConstant == 0.0));
    const HeatControllerCard bare = cardAt<HeatControllerTable>(model, 1);
    REQUIRE(
        (bare.sensorForm == SensorForm::Temperature && bare.controllerKind == ControlType::Thermostat)); // T and TSTAT
    REQUIRE((bare.initialState == 0.0 && bare.timeStep == 0.0 && bare.delayTime == 0.0 && bare.timeConstant == 0.0));
    REQUIRE((cardAt<HeatControllerTable>(model, 2).controlLaw == 11 &&
             cardAt<HeatControllerTable>(model, 2).sensorNode == 5));
    REQUIRE((HeatControllerTable::INFO.category == CardCategory::Other &&
             HeatControllerTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The defaults are blanks; PL, PH and PTYPE are always written.

    // The sensor is a grid point or a scalar point that may be named without being defined; the ID is a plain number.
    IdVisitor visitor;
    model.table<HeatControllerTable>().visitReferences(0, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::ScalarOrGridPoint, 1716}});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE(
        (cardAt<HeatControllerTable>(model, 0).sensorNode == 1726 && cardAt<HeatControllerTable>(model, 0).id == 1702));

    // The controller number is its own space: a grid point or a scalar point may have it.
    REQUIRE_NOTHROW(readBulk("HeatController;1716;1716;;;16.5;21.5;1\nNode;1716"));
    REQUIRE_THROWS_AS(readBulk("HeatController;5;1716;;;16.5;21.5;1\nHeatController;5;1717;;;16.5;21.5;1"),
                      base::DeckError);

    expectInvalid("HeatController;0;1716;Temperature;Thermostat;16.5;21.5;6", "id");
    expectInvalid("HeatController;1702;0;Temperature;Thermostat;16.5;21.5;6", "sensorNode");
    expectInvalid("HeatController;1702;;Temperature;Thermostat;16.5;21.5;6", "sensorNode");
    expectInvalid("HeatController;1702;1716;K;Thermostat;16.5;21.5;6", "sensorForm");
    expectInvalid("HeatController;1702;1716;Temperature;PID;16.5;21.5;6", "controllerKind");
    expectInvalid("HeatController;1702;1716;Temperature;Thermostat;;21.5;6", "lowerLimit");
    expectInvalid("HeatController;1702;1716;Temperature;Thermostat;16.5;;6", "upperLimit");
    expectInvalid("HeatController;1702;1716;Temperature;Thermostat;16.5;21.5", "controlLaw");
    expectInvalid("HeatController;1702;1716;Temperature;Thermostat;16.5;21.5;0", "controlLaw");
    expectInvalid("HeatController;1702;1716;Temperature;Thermostat;16.5;21.5;12", "controlLaw");
    expectInvalid("HeatController;1702;1716;Temperature;Thermostat;16.5;21.5;6;0.5", "initialState"); // 0.0 or 1.0
    expectInvalid("HeatController;1702;1716;Temperature;Thermostat;16.5;21.5;6;1.;-0.1", "timeStep");
    expectInvalid("HeatController;1702;1716;Temperature;Thermostat;16.5;21.5;6;1.;;-0.1", "delayTime");
    expectInvalid("HeatController;1702;1716;Temperature;Thermostat;16.5;21.5;6;1.;;;-0.1", "timeConstant");
}

// ---------------------------------------------------------------------------------------------------------------------
// BDYOR
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a BDYOR holds the defaults of the surface elements and names the cards they would name",
          "[model][cards][bdyor]")
{
    // Three cards in one deck: the format reference allows one, so the deck is read in the tolerant mode and the second
    // card is reported.
    Model model = readModelText(
        "[model]\nHeatSurfaceDefaults;AreaQuad4;4;5;6;8;12\nHeatSurfaceDefaults;Line;1;2;3;4;5;6;7;0.5;1.5;2."
        "5\nHeatSurfaceDefaults;Tube\n",
        false);
    REQUIRE(model.diagnostics().firstError()->code == "deck.duplicate-heat-surface-defaults");
    const HeatSurfaceDefaultsCard sampleCard = cardAt<HeatSurfaceDefaultsTable>(model, 0);
    REQUIRE(sampleCard.kind == HeatSurfaceDefaultsSurfaceType::Area4);
    REQUIRE((sampleCard.frontViewId == 4 && sampleCard.backViewId == 5 && sampleCard.frontRadiationPropId == 6 &&
             sampleCard.backRadiationPropId == 8 && sampleCard.propId == 12));
    REQUIRE((sampleCard.orientationNode == 0 && sampleCard.directionFrameId == 0 && !sampleCard.direction1 &&
             !sampleCard.direction2 && !sampleCard.direction3));
    const HeatSurfaceDefaultsCard full = cardAt<HeatSurfaceDefaultsTable>(model, 1);
    REQUIRE(full.kind == HeatSurfaceDefaultsSurfaceType::Line);
    REQUIRE((full.frontViewId == 1 && full.backViewId == 2 && full.frontRadiationPropId == 3 &&
             full.backRadiationPropId == 4 && full.propId == 5));
    REQUIRE((full.orientationNode == 6 && full.directionFrameId == 7));
    REQUIRE((full.direction1 == 0.5 && full.direction2 == 1.5 && full.direction3 == 2.5));
    REQUIRE(cardAt<HeatSurfaceDefaultsTable>(model, 2).kind == HeatSurfaceDefaultsSurfaceType::Tube);
    const HeatSurfaceDefaultsCard empty = cardAt<HeatSurfaceDefaultsTable>(readBulk("HeatSurfaceDefaults"));
    REQUIRE((!empty.kind && empty.frontViewId == 0 && empty.propId == 0 && empty.orientationNode == 0 &&
             empty.directionFrameId == 0 && !empty.direction1));
    REQUIRE((HeatSurfaceDefaultsTable::INFO.category == CardCategory::Other &&
             HeatSurfaceDefaultsTable::INFO.idSpace == base::IdSpace::None));
    // (The round trip below reads a deck with one card: every sample of the generic test has one.)
    requireRoundTrip(readBulk("HeatSurfaceDefaults;Line;1;2;3;4;5;6;7;0.5;1.5;2.5"));

    // The ten words of the field.
    for (const std::string_view word : {"Point", "Line", "ReversedNormal", "AreaTria3", "AreaQuad4", "EllipticCylinder",
                                        "ForcedConvectionTube", "AreaTria6", "AreaQuad8", "Tube"})
        REQUIRE(
            cardAt<HeatSurfaceDefaultsTable>(readBulk("HeatSurfaceDefaults;" + std::string(word))).kind.has_value());

    // The references are the ones of a CHBDYP: the VIEW, RADM, PHBDY, orientation point and system.
    IdVisitor visitor;
    model.table<HeatSurfaceDefaultsTable>().visitReferences(1, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::ViewFactorFaceFactor, 1},
                                 {ReferenceTarget::ViewFactorFaceFactor, 2},
                                 {ReferenceTarget::RadiationMaterial, 3},
                                 {ReferenceTarget::RadiationMaterial, 4},
                                 {ReferenceTarget::HeatBoundaryProperty, 5},
                                 {ReferenceTarget::Node, 6},
                                 {ReferenceTarget::Coordinate, 7}});
    visitor = IdVisitor{};
    model.table<HeatSurfaceDefaultsTable>().visitReferences(2, visitor);
    REQUIRE(visitor.seen.empty());      // blank fields name nothing
    REQUIRE(unresolvedIn(model) == 12); // 5 of the first card and 7 of the second: the deck has none of them

    Model resolved = readBulk(
        "HeatSurfaceDefaults;Line;1;2;3;4;5;6;7;0.5;1.5;2.5\n"
        "Node;6\n"
        "RectFrame2;7\n"
        "HeatSurfaceProp;5\n"
        "RadiationProp;3;0.9\n"
        "RadiationProp;4;0.8\n"
        "ViewFactorFace;1;1;Both;2;3;0.25\n"
        "ViewFactorFace;2;1;Both;2;3;0.25\n"
        "RadiationCavity;1");
    REQUIRE(unresolvedIn(resolved) == 0);
    Renumbering::apply(resolved, RenumberSpace::ViewFactorFaceFactor, Renumbering::offset(10));
    Renumbering::apply(resolved, RenumberSpace::RadiationMaterial, Renumbering::offset(20));
    Renumbering::apply(resolved, RenumberSpace::HeatBoundaryProperty, Renumbering::offset(30));
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(40));
    Renumbering::apply(resolved, RenumberSpace::Coordinate, Renumbering::offset(50));
    const HeatSurfaceDefaultsCard moved = cardAt<HeatSurfaceDefaultsTable>(resolved);
    REQUIRE((moved.frontViewId == 11 && moved.backViewId == 12 && moved.frontRadiationPropId == 23 &&
             moved.backRadiationPropId == 24));
    REQUIRE((moved.propId == 35 && moved.orientationNode == 46 && moved.directionFrameId == 57));

    expectInvalid("HeatSurfaceDefaults;SQUARE", "kind");
    expectInvalid("HeatSurfaceDefaults;AreaQuad4;-1", "frontViewId");
    expectInvalid("HeatSurfaceDefaults;AreaQuad4;;-1", "backViewId");
    expectInvalid("HeatSurfaceDefaults;AreaQuad4;;;-1", "frontRadiationPropId");
    expectInvalid("HeatSurfaceDefaults;AreaQuad4;;;;-1", "backRadiationPropId");
    expectInvalid("HeatSurfaceDefaults;AreaQuad4;;;;;-1", "propId");
    expectInvalid("HeatSurfaceDefaults;AreaQuad4;;;;;;-1", "orientationNode");
    expectInvalid("HeatSurfaceDefaults;AreaQuad4;;;;;;;-1", "directionFrameId");
    expectInvalid("HeatSurfaceDefaults;AreaQuad4;;;;;;;;LOW", "direction1");
    expectInvalid("HeatSurfaceDefaults;AreaQuad4;;;;;;;;;LOW", "direction2");
    expectInvalid("HeatSurfaceDefaults;AreaQuad4;;;;;;;;;;LOW", "direction3");
}

TEST_CASE("the defaults of a BDYOR go to the blank fields of the CHBDYE, CHBDYG and CHBDYP of the deck",
          "[model][cards][bdyor]")
{
    // The BDYOR is the last card: its defaults reach the cards that come before it.
    const Model model = readBulk(
        "HeatSurfaceSide;1;10;1\n"
        "HeatSurfaceSide;2;11;2;7;8;9;6\n"
        "HeatSurfaceNodes;3;AreaQuad4;;;;;1;2;3;4\n"
        "HeatSurfaceProperty;4;5;Line;;;21;22\n"
        "HeatSurfaceProperty;5;5;Line;;;21;22;23\n"
        "HeatSurfaceProperty;6;5;Line;;;21;22;;;;;;1.;2.;3.\n"
        "HeatSurfaceProperty;7;5;Line;3;;21;22\n"
        "HeatSurfaceDefaults;AreaQuad4;2;3;4;5;0;30;40;0.5;0.6;0.7");
    // The blank fields take the values of the BDYOR, the others keep their own.
    const HeatSurfaceSideCard blankEdge = cardAt<HeatSurfaceSideTable>(model, 0);
    REQUIRE((blankEdge.frontViewId == 2 && blankEdge.backViewId == 3 && blankEdge.frontRadiationPropId == 4 &&
             blankEdge.backRadiationPropId == 5));
    const HeatSurfaceSideCard givenEdge = cardAt<HeatSurfaceSideTable>(model, 1);
    REQUIRE((givenEdge.frontViewId == 7 && givenEdge.backViewId == 8 && givenEdge.frontRadiationPropId == 9 &&
             givenEdge.backRadiationPropId == 6));
    const HeatSurfaceNodesCard area = cardAt<HeatSurfaceNodesTable>(model);
    REQUIRE((area.frontViewId == 2 && area.backViewId == 3 && area.frontRadiationPropId == 4 &&
             area.backRadiationPropId == 5));
    // CHBDYP: the orientation grid point of the BDYOR, else its vector (here GO is 30, so the vector is not
    // used); a grid point or a vector of the card is kept.
    const HeatSurfacePropertyCard plain = cardAt<HeatSurfacePropertyTable>(model, 0);
    REQUIRE((plain.frontViewId == 2 && plain.backRadiationPropId == 5 && plain.orientationNode == 30 &&
             !plain.direction1 && plain.directionFrameId == 0));
    REQUIRE(cardAt<HeatSurfacePropertyTable>(model, 1).orientationNode == 23);
    const HeatSurfacePropertyCard ownVector = cardAt<HeatSurfacePropertyTable>(model, 2);
    REQUIRE((ownVector.orientationNode == 0 && ownVector.direction1 == 1.0 &&
             ownVector.directionFrameId == 40)); // CE of the BDYOR goes with a vector
    REQUIRE(cardAt<HeatSurfacePropertyTable>(model, 3).frontViewId == 3);

    // Without a GO the BDYOR gives its vector, and CE with it.
    const Model vector =
        readBulk("HeatSurfaceProperty;4;5;Line;;;21;22\nHeatSurfaceDefaults;Line;;;;;;;40;0.5;0.6;0.7");
    const HeatSurfacePropertyCard fromVector = cardAt<HeatSurfacePropertyTable>(vector);
    REQUIRE((fromVector.orientationNode == 0 && fromVector.direction1 == 0.5 && fromVector.direction2 == 0.6 &&
             fromVector.direction3 == 0.7));
    REQUIRE(fromVector.directionFrameId == 40);

    // A deck with no BDYOR leaves the blank fields as they are, and a BDYOR added later gives its defaults to the cards
    // that are blank when the defaults are applied again.
    Model without = readBulk("HeatSurfaceSide;1;10;1");
    REQUIRE(cardAt<HeatSurfaceSideTable>(without).frontViewId == 0);
    HeatSurfaceDefaultsCard added;
    added.kind = HeatSurfaceDefaultsSurfaceType::Area4;
    added.frontViewId = 6;
    without.add<HeatSurfaceDefaultsTable>(added);
    without.applyDefaults();
    REQUIRE(cardAt<HeatSurfaceSideTable>(without).frontViewId == 6);
}

// ---------------------------------------------------------------------------------------------------------------------
// VCCT
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a negative G1 of a VCCT is the SET3 that lists the points of the crack front", "[model][cards][vcct]")
{
    Model model = readBulk(vcct("1", "0.", "0", "-5") + "\n" + vcct("1,2", "0.", "0", "7") +
                           "\nTypedListSet3;5;Node;7;8;9\nNode;7\nNode;8\nNode;9");
    const CrackClosureCrackCard bySet = cardAt<CrackClosureCrackTable>(model, 0);
    REQUIRE((bySet.nodeSetId == 5 && bySet.nodes.empty()));
    REQUIRE((cardAt<CrackClosureCrackTable>(model, 1).nodeSetId == 0 &&
             cardAt<CrackClosureCrackTable>(model, 1).nodes.size() == 1));
    requireRoundTrip(model);

    // The set is a reference to the sets; the points of a list are nodes (the ends of a range may not exist).
    IdVisitor visitor;
    model.table<CrackClosureCrackTable>().visitReferences(0, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::Set, 5}});
    visitor = IdVisitor{};
    model.table<CrackClosureCrackTable>().visitReferences(1, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::Node, 7}});
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(readBulk(vcct("1", "0.", "0", "-5"))) == 1);
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(100));
    REQUIRE(cardAt<CrackClosureCrackTable>(model, 0).nodeSetId == 105);

    // Only one SET3 ID is allowed, and nothing follows it.
    expectInvalid(vcct("1", "0.", "0", "-5,6"), "node2");
    expectInvalid(vcct("1", "0.", "0", "-5,Through,9"), "node2");
}

TEST_CASE("a VCCT names its four tables, its set and its points, and the crack set is an ID shared by cards",
          "[model][cards][vcct]")
{
    Model model = readBulk(fullCrackClosureCrack() + "\n" + TABLES + "Node;41\nNode;63\nNode;42");
    IdVisitor visitor;
    model.table<CrackClosureCrackTable>().visitReferences(0, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::MaterialTable, 10},
                                 {ReferenceTarget::MaterialTable, 11},
                                 {ReferenceTarget::MaterialTable, 12},
                                 {ReferenceTarget::MaterialTable, 13},
                                 {ReferenceTarget::Node, 41},
                                 {ReferenceTarget::NodeInRange, 42},
                                 {ReferenceTarget::NodeInRange, 50},
                                 {ReferenceTarget::Node, 63}});
    REQUIRE(unresolvedIn(model) == 0);
    // A table and a single point that nothing defines are reported, the ends of a range are not.
    REQUIRE(unresolvedIn(readBulk(vcct("1", "0.", "5", "7,Through,9,10"))) == 2);

    Renumbering::apply(model, RenumberSpace::MaterialTable, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    const CrackClosureCrackCard moved = cardAt<CrackClosureCrackTable>(model);
    REQUIRE((moved.growthIncrementTableId == 110 && moved.growthResistanceTableId == 111 &&
             moved.growthResistanceIITableId == 112 && moved.growthResistanceIIITableId == 113));
    REQUIRE((moved.nodes == std::vector<ContactRun>{{1041, 1041, 1}, {1042, 1050, 2}, {1063, 1063, 1}}));
    REQUIRE(moved.setId == 204); // the crack set is not a node
    REQUIRE(unresolvedIn(model) == 0);

    // Many cracks share a set ID (IDCR numbers them); the set space is the one of the Case Control VCCT.
    REQUIRE_NOTHROW(readBulk(vcct("204,1", "0.", "0", "1") + "\n" + vcct("204,2", "0.", "0", "2")));

    expectInvalid(vcct("0", "0.", "0", "1"), "setId");
    expectInvalid(vcct("-4", "0.", "0", "1"), "setId");
    expectInvalid(vcct("1,0", "0.", "0", "1"), "crackId");
    expectInvalid(vcct("1,-2", "0.", "0", "1"), "crackId");
    expectInvalid(vcct("1,1,-1", "0.", "0", "1"), "propagationKind");
    expectInvalid(vcct("1,1,0,-1", "0.", "0", "1"), "growthMethod");
    expectInvalid(vcct("1,1,0,2,-1", "0.", "0", "1"), "incrementMethod");
    expectInvalid(vcct("1,1,0,2,1,-1", "0.", "0", "1"), "growthCriterion");
    expectInvalid(vcct("1,1,LOW", "0.", "0", "1"), "propagationKind");
    expectInvalid(vcct("1", "LOW", "0", "1"), "growthIncrement");
    expectInvalid(vcct("1", "0.,LOW", "0", "1"), "growthResistance");
    expectInvalid(vcct("1", "0.,0.,,,,,LOW", "0", "1"), "growthResistanceII");
    expectInvalid(vcct("1", "0.,0.,,,,,0.,LOW", "0", "1"), "growthResistanceIII");
    expectInvalid(vcct("1", "0.", "-1", "1"), "growthIncrementTableId");
    expectInvalid(vcct("1", "0.", "0,-1", "1"), "growthResistanceTableId");
    expectInvalid(vcct("1", "0.", "0,0,,,,,-1", "1"), "growthResistanceIITableId");
    expectInvalid(vcct("1", "0.", "0,0,,,,,0,-1", "1"), "growthResistanceIIITableId");
    expectInvalid("CrackClosureCrack;1;;;;;;0.;;;;0", "node1");      // a crack front is required
    expectInvalid("CrackClosureCrack;1;;;;;;0.;;;;0;;;;0", "node1"); // a grid point is positive
    expectInvalid(vcct("1", "0.", "0", "5,Through"), "Through");
    expectInvalid(vcct("1", "0.", "0", "5,By,2"), "By");
    expectInvalid(vcct("1", "0.", "0", "5,Through,9,By,0"), "By");
    expectInvalid(vcct("1", "0.", "0", "5,Through,9,By,-1"),
                  "By"); // the increment goes from the first to the last point
}
