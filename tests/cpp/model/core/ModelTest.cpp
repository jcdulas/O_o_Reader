#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"
#include "oor/model/control/case_control/CommandCatalogue.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <iterator>
#include <map>
#include <random>
#include <utility>

using namespace oor;
using namespace oor::model;
using oor::testing::readModelText;
using oor::testing::writeModelText;

namespace
{

NodeCard gridAt(base::EntityId id, double x)
{
    NodeCard card;
    card.id = id;
    card.xyz = base::Vec3{x, 0.0, 0.0};
    return card;
}

} // namespace

TEST_CASE("the registry knows the card types and index 0 is the unknown table", "[model][core]")
{
    const CardRegistry& registry = CardRegistry::instance();
    REQUIRE(registry.entry(CardRegistry::UNKNOWN_TYPE).info.category == CardCategory::Unknown);
    REQUIRE(registry.typeIndexOf("Node").has_value());
    REQUIRE_FALSE(registry.typeIndexOf("NoSuchCard").has_value());
    REQUIRE(registry.entry(*registry.typeIndexOf("ShellQuad4")).info.idSpace == base::IdSpace::Element);
    REQUIRE(registry.entry(*registry.typeIndexOf("PointForceLoad")).info.idSpace == base::IdSpace::Group);
}

TEST_CASE("added cards are found by ID in their ID space", "[model][core]")
{
    Model model;
    const CardLocation first = model.add<NodeTable>(gridAt(10, 1.0));
    model.add<NodeTable>(gridAt(20, 2.0));
    REQUIRE(model.cardCount() == 2);
    REQUIRE(model.locate(base::IdSpace::Node, 10) == first);
    REQUIRE_FALSE(model.locate(base::IdSpace::Node, 30).has_value());
    REQUIRE(model.table<NodeTable>().xyz(first.row).x == 1.0);
}

TEST_CASE("elements of different types share one ID space", "[model][core]")
{
    Model model;
    Rod2Card rod;
    rod.id = 5;
    rod.propId = 1;
    rod.nodes = {1, 2};
    model.add<Rod2Table>(rod);
    ShellQuad4Card quad;
    quad.id = 5; // another element type, the same element ID
    quad.propId = 1;
    quad.nodes = {1, 2, 3, 4};
    REQUIRE_THROWS_AS(model.add<ShellQuad4Table>(quad), base::DuplicateIdError);
    REQUIRE(model.cardCount() == 1); // the failed add left no trace
}

TEST_CASE("a duplicate ID is an error in strict mode and a warning in tolerant mode", "[model][core]")
{
    const std::string deck = "[model]\nNode;1;;0.;0.;0.\nNode;1;;5.;0.;0.\n";
    REQUIRE_THROWS_AS(readModelText(deck, true), base::DeckError);

    Model model = readModelText(deck, false);
    REQUIRE(model.diagnostics().diagnostics().size() >= 1);
    const auto location = model.locate(base::IdSpace::Node, 1);
    REQUIRE(location.has_value());
    REQUIRE(model.table<NodeTable>().xyz(location->row).x == 5.0); // the last card wins
}

TEST_CASE("setId renumbers one card and checks for a clash", "[model][core]")
{
    Model model;
    const CardLocation first = model.add<NodeTable>(gridAt(1, 0.0));
    model.add<NodeTable>(gridAt(2, 0.0));
    REQUIRE_THROWS_AS(model.setId(first, 2), base::DuplicateIdError);
    model.setId(first, 7);
    REQUIRE_FALSE(model.locate(base::IdSpace::Node, 1).has_value());
    REQUIRE(model.locate(base::IdSpace::Node, 7) == first);
}

TEST_CASE("remove leaves a tombstone and compact drops it", "[model][core]")
{
    Model model;
    model.add<NodeTable>(gridAt(1, 1.0));
    const CardLocation second = model.add<NodeTable>(gridAt(2, 2.0));
    model.add<NodeTable>(gridAt(3, 3.0));
    model.remove(second);
    REQUIRE(model.cardCount() == 2);
    REQUIRE_FALSE(model.locate(base::IdSpace::Node, 2).has_value());
    REQUIRE(model.table<NodeTable>().rows().rowCount() == 3);

    model.compact();
    REQUIRE(model.table<NodeTable>().rows().rowCount() == 2);
    const auto third = model.locate(base::IdSpace::Node, 3);
    REQUIRE(third.has_value());
    REQUIRE(model.table<NodeTable>().xyz(third->row).x == 3.0);
}

TEST_CASE("random adds, deletes, edits and compactions keep a table equal to a reference map",
          "[model][core][property]")
{
    // The node table against a map of ID to position; the card keys are never reused and survive the
    // compactions.
    Model model;
    std::map<base::EntityId, double> reference;
    std::map<base::EntityId, std::uint32_t> keys;
    std::uint32_t highestKey = 0;
    bool anyKey = false;
    std::mt19937 generator(2026);
    const auto anyId = [&]()
    {
        auto at = reference.begin();
        std::advance(at, static_cast<std::ptrdiff_t>(generator() % reference.size()));
        return at->first;
    };
    // Every card of the reference is found by its ID, with its value and the key it was given.
    const auto requireEqual = [&]()
    {
        const NodeTable& grids = model.table<NodeTable>();
        for (const auto& [id, x] : reference)
        {
            const auto location = model.locate(base::IdSpace::Node, id);
            REQUIRE(location.has_value());
            REQUIRE(grids.xyz(location->row).x == x);
            REQUIRE(grids.rows().keyAt(location->row).value == keys.at(id));
        }
    };
    for (int step = 0; step < 3000; ++step)
    {
        const unsigned operation = generator() % 10;
        if (operation < 4 || reference.empty())
        {
            const auto id = static_cast<base::EntityId>(1 + generator() % 500);
            if (reference.contains(id))
                continue;
            const double x = static_cast<double>(generator() % 1000) * 0.5;
            const CardLocation added = model.add<NodeTable>(gridAt(id, x));
            const std::uint32_t key = model.table<NodeTable>().rows().keyAt(added.row).value;
            REQUIRE((!anyKey || key > highestKey)); // a new card never takes the key of a card deleted before
            highestKey = key;
            anyKey = true;
            reference[id] = x;
            keys[id] = key;
        }
        else if (operation < 6)
        {
            const base::EntityId id = anyId();
            model.remove(*model.locate(base::IdSpace::Node, id));
            reference.erase(id);
            keys.erase(id);
        }
        else if (operation < 9)
        {
            const base::EntityId id = anyId();
            const double x = static_cast<double>(generator() % 1000) * 0.25;
            model.table<NodeTable>().setXyzComponent(model.locate(base::IdSpace::Node, id)->row, 0, x);
            reference[id] = x;
        }
        else
        {
            model.compact();
        }
        REQUIRE(model.cardCount() == reference.size());
        if (step % 250 == 0)
            requireEqual();
    }
    model.compact();
    REQUIRE(model.table<NodeTable>().rows().rowCount() == reference.size());
    requireEqual();
}

TEST_CASE("a copy is independent from the original", "[model][core]")
{
    Model original;
    const CardLocation grid = original.add<NodeTable>(gridAt(1, 1.0));
    Model copy = original.copy();
    copy.table<NodeTable>().setXyzComponent(grid.row, 0, 9.0);
    REQUIRE(original.table<NodeTable>().xyz(grid.row).x == 1.0);
    REQUIRE(copy.table<NodeTable>().xyz(grid.row).x == 9.0);
    original.add<NodeTable>(gridAt(2, 2.0));
    REQUIRE(copy.cardCount() == 1);
}

TEST_CASE("a copy removes and adds cards without changing the indexes of the original", "[model][core]")
{
    Model original;
    const CardLocation first = original.add<NodeTable>(gridAt(1, 1.0));
    original.add<NodeTable>(gridAt(2, 2.0));
    Model copy = original.copy(); // shares the ID indexes and the row books until one side changes them
    copy.remove(first);
    copy.add<NodeTable>(gridAt(3, 3.0));
    REQUIRE(original.cardCount() == 2);
    REQUIRE(original.locate(base::IdSpace::Node, 1) == first);
    REQUIRE(original.table<NodeTable>().rows().isAlive(first.row));
    REQUIRE_FALSE(original.locate(base::IdSpace::Node, 3).has_value());
    REQUIRE(copy.cardCount() == 2);
    REQUIRE_FALSE(copy.locate(base::IdSpace::Node, 1).has_value());
    REQUIRE(copy.locate(base::IdSpace::Node, 3).has_value());
    original.remove(*original.locate(base::IdSpace::Node, 2));
    REQUIRE(copy.locate(base::IdSpace::Node, 2).has_value());
}

TEST_CASE("a copy shares the control sections until one side changes them", "[model][core]")
{
    const CommandDefinition* load = CommandCatalogue::instance().findByName("StaticLoadSelection");
    REQUIRE(load != nullptr);
    Model original;
    CaseControl& control = original.control().caseControl;
    control.setCommand(control.global(), *load, "", {}, CaseValue::ofInteger(1));
    Model copy = original.copy();
    const Model& reading = original;
    REQUIRE(&reading.control() == &std::as_const(copy).control()); // shared: reading changes nothing
    CaseControl& edited = copy.control().caseControl;              // asked to change: the copy gets its own
    REQUIRE(&reading.control() != &std::as_const(copy).control());
    edited.setCommand(edited.global(), *load, "", {}, CaseValue::ofInteger(2));
    REQUIRE(reading.control()
                .caseControl.effectiveCommand(reading.control().caseControl.global().key, "StaticLoadSelection")
                ->value.integer == 1);
    REQUIRE(edited.effectiveCommand(edited.global().key, "StaticLoadSelection")->value.integer == 2);
}

TEST_CASE("cards are written in file order whatever their type", "[model][core]")
{
    const std::string deck = "[model]\nNode;2;;1.;0.;0.\nIsoMat;1;2.1E+11;;0.3\nNode;1;;0.;0.;0.\nRod2;1;1;1;2\n";
    const Model model = readModelText(deck, true);
    const std::string written = writeModelText(model);
    INFO(written);
    const auto grid2 = written.find("Node;2;");
    const auto mat1 = written.find("IsoMat;");
    const auto grid1 = written.find("Node;1;");
    const auto crod = written.find("Rod2;");
    REQUIRE(grid2 != std::string::npos);
    REQUIRE(grid2 < mat1);
    REQUIRE(mat1 < grid1);
    REQUIRE(grid1 < crod);
}

TEST_CASE("comments travel with their card", "[model][core]")
{
    const std::string deck = "[model]\n# the first point\nNode;1;;0.;0.;0.\nNode;2;;1.;0.;0.\n";
    const Model model = readModelText(deck, true);
    const std::string written = writeModelText(model);
    REQUIRE(written.find("# the first point\nNode;") != std::string::npos);
}

TEST_CASE("an unknown card is kept and written back unchanged", "[model][core]")
{
    const std::string deck = "[model]\nNode;1;;0.;0.;0.\nFutureVeryLongCard;1;2;THREE\n";
    const Model model = readModelText(deck, true);
    REQUIRE(model.cardCount() == 2);
    const std::string written = oor::testing::writeModelText(model);
    INFO(written);
    REQUIRE(written.find("\nFutureVeryLongCard;1;2;THREE\n") != std::string::npos);
}

TEST_CASE("a card that fails to parse is an error in strict mode and kept as text in tolerant mode", "[model][core]")
{
    const std::string deck = "[model]\nNode;1;;0.;0.;0.\nNode;abc;;0.;0.;0.\n";
    REQUIRE_THROWS_AS(readModelText(deck, true), base::DeckError);
    const Model model = readModelText(deck, false);
    REQUIRE(model.diagnostics().hasErrors());
    REQUIRE(model.cardCount() == 2);
    REQUIRE(writeModelText(model).find("Node;abc") != std::string::npos);
}

TEST_CASE("a part is a model of its own with its own ID spaces", "[model][core]")
{
    const std::string deck = "[model]\nNode;1;;0.;0.;0.\n[model;SuperelementPart;5]\nNode;1;;7.;0.;0.\n";
    const Model model = readModelText(deck, true);
    REQUIRE(model.parts().size() == 1);
    const Model& part = *model.parts().front();
    REQUIRE(part.isPart());
    REQUIRE(part.partInfo()->id == 5);
    const auto inPart = part.locate(base::IdSpace::Node, 1);
    REQUIRE(inPart.has_value());
    REQUIRE(part.findTable<NodeTable>()->xyz(inPart->row).x == 7.0);
    REQUIRE(model.findTable<NodeTable>()->xyz(model.locate(base::IdSpace::Node, 1)->row).x == 0.0);
}

TEST_CASE("the reference visitor sees each reference field and can renumber it", "[model][core]")
{
    Model model;
    RodPropCard prod;
    prod.id = 3;
    prod.matId = 4;
    prod.area = 1.0;
    const CardLocation location = model.add<RodPropTable>(prod);

    struct Doubler final : ReferenceVisitor
    {
        int visits = 0;
        base::EntityId visit(std::string_view, ReferenceTarget target, base::EntityId id) override
        {
            ++visits;
            REQUIRE(target == ReferenceTarget::Material);
            return id * 2;
        }
    } visitor;
    model.table<RodPropTable>().visitReferences(location.row, visitor);
    REQUIRE(visitor.visits == 1);
    REQUIRE(model.table<RodPropTable>().card(location.row).matId == 8);
}

TEST_CASE("the control version changes when the control sections may be edited, not when they are read",
          "[model][core]")
{
    Model model = readModelText(
        "[executive]\nSolutionSequence;101\n[case]\nNonStructMassSelection;5\nSubcaseStart;1\n[model]\nNode;1;;0.;0.;0."
        "\n");
    const std::uint64_t read = model.controlVersion();
    REQUIRE(std::as_const(model).control().caseControl.segments().size() == 2); // a read through the const sections
    REQUIRE(model.controlVersion() == read);
    const std::uint64_t content = model.contentVersion();
    (void)model.control(); // the non-const sections: an edit may follow
    REQUIRE(model.controlVersion() != read);
    REQUIRE(model.contentVersion() == content); // the Case Control is not the content of the cards

    // A copy shares the sections and their version; an edit of one side changes that side only, to a number neither
    // side had (one counter for all models).
    Model copy(model);
    const std::uint64_t shared = model.controlVersion();
    REQUIRE(copy.controlVersion() == shared);
    copy.control().caseControl.segments().front().entries.clear();
    REQUIRE(copy.controlVersion() != shared);
    REQUIRE(model.controlVersion() == shared);
    REQUIRE(CaseControl::ownCommand(std::as_const(model).control().caseControl.segments().front(),
                                    "NonStructMassSelection") != nullptr);
    REQUIRE(CaseControl::ownCommand(std::as_const(copy).control().caseControl.segments().front(),
                                    "NonStructMassSelection") == nullptr);
    const std::uint64_t edited = copy.controlVersion();
    model.noteControlChange();
    REQUIRE(model.controlVersion() != shared);
    REQUIRE(model.controlVersion() != edited);
}
