// Checks of the non-structural mass cards: NSM, NSM1, NSML, NSML1 and NSMADD: field positions, the lists
// with THRU, BY and ALL, the writing, the references to properties and elements (an ID that names nothing is let be)
// and their renumbering, and the rules by field.

#include "oor/model/cards/properties/nonStructMass/NonStructMassTable.hpp"
#include "oor/model/cards/properties/nonStructMass1/NonStructMass1Table.hpp"
#include "oor/model/cards/properties/nonStructMassLumped/NonStructMassLumpedTable.hpp"
#include "oor/model/cards/properties/nonStructMassLumped1/NonStructMassLumped1Table.hpp"
#include "oor/model/cards/properties/nonStructMassUnion/NonStructMassUnionTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
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

std::size_t unresolvedIn(Model& model)
{
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    return ReferenceChecker::check(model, sink);
}

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

// The model written as a model file and read again gives the same cards and the same text.
void requireRoundTrip(const Model& model)
{
    oor::testing::requireModelFileRoundTrip(model);
}

const char* const PROPERTIES =
    "IsoMat;1;2.1E+5;;0.3;0.001\nShellProp;10;1;1.\nShellProp;11;1;2.\nBarProp;20;1;1.;1.;1.;1.\n";

} // namespace

TEST_CASE("NSM and NSML read their (ID, VALUE) pairs over the continuation lines", "[model][cards][nsm]")
{
    const Model model = readBulk(
        "NonStructMass;3;ShellProp;17;0.031\n"
        "NonStructMass;4;Element;1;0.1;2;0.2;3;0.3;4;0.4;5;0.5\n"
        "NonStructMassLumped;5;ShellProp;17;0.73\n"
        "NonStructMassLumped;6;BarProp;2;1.5;3;2.5");
    const NonStructMassCard first = cardAt<NonStructMassTable>(model, 0);
    REQUIRE(first.id == 3);
    REQUIRE(first.type == "ShellProp"); // the neutral name of the card type, from a word in any case
    REQUIRE(first.pairs == std::vector<NonStructMassPair>{{17, 0.031}});
    const NonStructMassCard second = cardAt<NonStructMassTable>(model, 1);
    REQUIRE(second.pairs.size() == 5);
    REQUIRE(second.pairs.back() == NonStructMassPair{5, 0.5});
    REQUIRE(cardAt<NonStructMassLumpedTable>(model, 0).pairs == std::vector<NonStructMassPair>{{17, 0.73}});
    REQUIRE(cardAt<NonStructMassLumpedTable>(model, 1).pairs == std::vector<NonStructMassPair>{{2, 1.5}, {3, 2.5}});
    // A set ID is shared by many cards: the same set in NSM and in NSML.
    const Model shared = readBulk(
        "NonStructMass;7;ShellProp;10;0.1\nNonStructMass;7;ShellProp;11;0.2\nNonStructMassLumped;7;BarProp;20;3.");
    REQUIRE(shared.findTable<NonStructMassTable>()->rows().rowCount() == 2);
    requireRoundTrip(model);
}

TEST_CASE("NSM1 and NSML1 list IDs, ranges with and without BY, or ALL", "[model][cards][nsm]")
{
    const Model model = readBulk(
        "NonStructMass1;6;ShellProp;0.038;1;2;3\n"
        "NonStructMass1;7;Element;0.038;1;Through;20\n"
        "NonStructMass1;8;BarProp;0.5;5;10;Through;50;By;10;100\n"
        "NonStructMass1;9;CompProp;0.052;All\n"
        "NonStructMassLumped1;12;Element;0.038;1;2;3\n"
        "NonStructMassLumped1;13;TubeProp;0.0147;All\n"
        "NonStructMassLumped1;14;ShellProp;0.059;1;Through;5;10;Through;15\n"
        "NonStructMassLumped1;15;ShellProp;0.059;1;Through;5");
    REQUIRE(cardAt<NonStructMass1Table>(model, 0).ids.runs ==
            std::vector<NonStructMassRun>{{1, 1, 1}, {2, 2, 1}, {3, 3, 1}});
    REQUIRE(cardAt<NonStructMass1Table>(model, 0).nonStructMass == 0.038);
    REQUIRE(cardAt<NonStructMass1Table>(model, 1).ids.runs == std::vector<NonStructMassRun>{{1, 20, 1}});
    REQUIRE(cardAt<NonStructMass1Table>(model, 2).ids.runs ==
            std::vector<NonStructMassRun>{{5, 5, 1}, {10, 50, 10}, {100, 100, 1}});
    REQUIRE(cardAt<NonStructMass1Table>(model, 3).ids.all);
    REQUIRE(cardAt<NonStructMass1Table>(model, 3).type == "CompProp");
    REQUIRE(cardAt<NonStructMassLumped1Table>(model, 1).ids.all);
    REQUIRE(cardAt<NonStructMassLumped1Table>(model, 2).ids.runs ==
            std::vector<NonStructMassRun>{{1, 5, 1}, {10, 15, 1}});
    REQUIRE(cardAt<NonStructMassLumped1Table>(model, 3).ids.runs ==
            std::vector<NonStructMassRun>{{1, 5, 1}}); // TO is THRU
    // The IDs a list stands for: a range goes by its increment.
    REQUIRE(expandNonStructMassIds(cardAt<NonStructMass1Table>(model, 2).ids) ==
            std::vector<base::EntityId>{5, 10, 20, 30, 40, 50, 100});
    requireRoundTrip(model);
}

TEST_CASE("the writer keeps a THRU BY range on one line", "[model][cards][nsm]")
{
    // Ten single IDs end at field 13: a range of five fields would straddle the line, so it starts on the next one.
    const Model model = readBulk("NonStructMass1;1;ShellProp;0.5;1;2;3;4;5;6;7;8;9;10;Through;50;By;10");

    // The line of the range starts with the THRU form entire.
    requireRoundTrip(model);
    const Model again = readModelText(writeModelText(model), true);
    REQUIRE(cardAt<NonStructMass1Table>(again).ids == cardAt<NonStructMass1Table>(model).ids);
}

TEST_CASE("NSMADD lists the sets it adds", "[model][cards][nsm]")
{
    const Model model = readBulk("NonStructMassUnion;21;6;7;8\nNonStructMassUnion;22;6");
    REQUIRE(cardAt<NonStructMassUnionTable>(model, 0).sets == std::vector<base::EntityId>{6, 7, 8});
    REQUIRE(cardAt<NonStructMassUnionTable>(model, 1).sets == std::vector<base::EntityId>{6});
    requireRoundTrip(model);
}

TEST_CASE("the IDs of the cards are references that renumber with properties and elements, and are let be when missing",
          "[model][cards][nsm]")
{
    Model model = readBulk(std::string(PROPERTIES) +
                           "Node;1;;0.;0.;0.\n"
                           "Node;2;;1.;0.;0.\n"
                           "Rod2;100;20;1;2\n"
                           "Rod2;101;20;1;2\n"
                           "NonStructMass;1;ShellProp;10;0.1;99;0.2\n"
                           "NonStructMass;2;Element;100;0.1\n"
                           "NonStructMass1;3;BarProp;0.1;20\n"
                           "NonStructMass1;4;Element;0.1;100;Through;101\n"
                           "NonStructMassLumped;5;ShellProp;11;0.5\n"
                           "NonStructMassLumped1;6;ShellProp;0.5;10;Through;11\n"
                           "NonStructMass;7;CrackTipProp2D;5;0.1\n");
    // A missing property or element is not a broken reference (the format reference: undefined IDs are ignored).
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(cardAt<NonStructMassTable>(model, 0).pairs == std::vector<NonStructMassPair>{{1010, 0.1}, {1099, 0.2}});
    REQUIRE(cardAt<NonStructMass1Table>(model, 0).ids.runs == std::vector<NonStructMassRun>{{1020, 1020, 1}});
    REQUIRE(cardAt<NonStructMassLumpedTable>(model, 0).pairs == std::vector<NonStructMassPair>{{1011, 0.5}});
    REQUIRE(cardAt<NonStructMassLumped1Table>(model, 0).ids.runs == std::vector<NonStructMassRun>{{1010, 1011, 1}});
    // The elements of an ELEMENT card do not move with the properties.
    REQUIRE(cardAt<NonStructMassTable>(model, 1).pairs == std::vector<NonStructMassPair>{{100, 0.1}});
    // PRAC2D is a card the library does not have: its IDs stay.
    REQUIRE(cardAt<NonStructMassTable>(model, 2).pairs == std::vector<NonStructMassPair>{{5, 0.1}});
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(500));
    REQUIRE(cardAt<NonStructMassTable>(model, 1).pairs == std::vector<NonStructMassPair>{{600, 0.1}});
    REQUIRE(cardAt<NonStructMass1Table>(model, 1).ids.runs == std::vector<NonStructMassRun>{{600, 601, 1}});
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEST_CASE("the NSM cards are invalid in the fields the format reference names", "[model][cards][nsm]")
{
    expectInvalid("NonStructMass;0;ShellProp;1;0.1", "id");
    expectInvalid("NonStructMass;1;BushProp;1;0.1", "type");
    expectInvalid("NonStructMass;1;;1;0.1", "type");
    expectInvalid("NonStructMass;1;ShellProp", "targetId");
    expectInvalid("NonStructMass;1;ShellProp;0;0.1", "targetId");
    expectInvalid("NonStructMass;1;ShellProp;5", "targetValue");
    expectInvalid("NonStructMassLumped;1;ShellProp;5", "targetValue");
    expectInvalid("NonStructMassLumped;1;PUNCH;5;0.1", "type");
    expectInvalid("NonStructMass1;1;ShellProp;;1", "nonStructMass");
    expectInvalid("NonStructMass1;1;ShellProp;0.1", "targetId");
    expectInvalid("NonStructMass1;1;ShellProp;0.1;0", "targetId");
    expectInvalid("NonStructMass1;1;ShellProp;0.1;All;5", "targetId");
    expectInvalid("NonStructMass1;1;ShellProp;0.1;5;All", "targetId");
    expectInvalid("NonStructMass1;1;ShellProp;0.1;Through;5", "through");
    expectInvalid("NonStructMass1;1;ShellProp;0.1;5;Through", "through");
    expectInvalid("NonStructMass1;1;ShellProp;0.1;5;By;2", "increment");
    expectInvalid("NonStructMass1;1;ShellProp;0.1;8;Through;3", "targetId");
    expectInvalid("NonStructMass1;1;ShellProp;0.1;1;Through;9;By;0", "increment");
    expectInvalid("NonStructMassLumped1;1;ShellProp;0.1", "targetId");
    expectInvalid("NonStructMassUnion;1", "memberSetId1");
    expectInvalid("NonStructMassUnion;1;1", "memberSetId1");
    expectInvalid("NonStructMassUnion;1;2;0", "memberSetId1");
    expectInvalid("NonStructMassUnion;0;2", "id");
}
