// Checks of the cards of an FRF based assembly: FRFCOMP, FRFCONN, FRFFLEX, FRFRELS, FRFSPC1,
// FRFXIT, FRFXIT1, FBADLAY, FBALOAD and FBAPHAS. Per card: the fields in their format reference positions, the
// defaults, what a component named by ID or by name means, the references it lists and what each one points to, the
// renumbering of the spaces it uses, the refused values and the round trip through a model file. The generic tests
// of CardRoundTripTest.cpp read, write and copy every sample; this file covers what each card means.

#include "oor/model/cards/dynamics/acousticSourceLoad/AcousticSourceLoadTable.hpp"
#include "oor/model/cards/dynamics/freqResponseLoad1/FreqResponseLoad1Table.hpp"
#include "oor/model/cards/dynamics/freqResponseLoad2/FreqResponseLoad2Table.hpp"
#include "oor/model/cards/dynamics/frfAssemblyLoadDelay/FrfAssemblyLoadDelayTable.hpp"
#include "oor/model/cards/dynamics/frfAssemblyLoadPhase/FrfAssemblyLoadPhaseTable.hpp"
#include "oor/model/cards/dynamics/frfAssemblyLoadScale/FrfAssemblyLoadScaleTable.hpp"
#include "oor/model/cards/dynamics/frfComponent/FrfComponentTable.hpp"
#include "oor/model/cards/dynamics/frfConnection/FrfConnectionTable.hpp"
#include "oor/model/cards/dynamics/frfExcitationDof/FrfExcitationDofTable.hpp"
#include "oor/model/cards/dynamics/frfExcitationDofList/FrfExcitationDofListTable.hpp"
#include "oor/model/cards/dynamics/frfFlexibleConnection/FrfFlexibleConnectionTable.hpp"
#include "oor/model/cards/dynamics/frfReleasedDofs/FrfReleasedDofsTable.hpp"
#include "oor/model/cards/dynamics/frfSinglePointConstr/FrfSinglePointConstrTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <span>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// Four FRF components: the first two are named by their numbers in the decks below, the other two by their names.
const std::string COMPONENTS_TEXT =
    "FrfComponent;5;ONE;Database\nFrfComponent;15;TWO;Database\nFrfComponent;30;BODY;Database\nFrfComponent;40;WING;"
    "Database\n";

// A TABLED1 of this number.
std::string tabled(int id)
{
    return "TableDynamic1;" + std::to_string(id) + ";;;0.;0.;1.;1.;EndTable\n";
}

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

template <typename TableT>
std::size_t aliveCards(const Model& model)
{
    const TableT* table = model.findTable<TableT>();
    return table == nullptr ? 0 : table->rows().aliveCount();
}

// The problem a check finds, as the name of its field ("" when there is none).
std::string problemField(const std::optional<CardProblem>& problem)
{
    return problem ? std::string(problem->field) : std::string();
}

} // namespace

TEST_CASE("an FRFCOMP has a unique ID, a name that may be blank, a medium and the unit of its file",
          "[model][cards][frf]")
{
    REQUIRE((FrfComponentTable::INFO.category == CardCategory::FrfComponent &&
             FrfComponentTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "FrfComponent;10;FINBOX;BinaryResultsFile;27\nFrfComponent;20;CHASSIS;UniversalFile;31;25.4\nFrfComponent;30;;"
        "Database\nFrfComponent;40;Body;"
        "UniversalFile;26;0.5;2.5");
    const FrfComponentCard binary = cardAt<FrfComponentTable>(model, 0);
    REQUIRE((binary.id == 10 && binary.name == "FINBOX" && binary.medium == FrfMedium::BinaryResultsFile &&
             binary.unitNumber == 27));
    REQUIRE((binary.lengthScaleFactor == 1.0 && binary.forceScaleFactor == 1.0)); // the scale factors default to 1.0
    const FrfComponentCard universal = cardAt<FrfComponentTable>(model, 1);
    REQUIRE((universal.medium == FrfMedium::UniversalFile && universal.unitNumber == 31));
    REQUIRE((universal.lengthScaleFactor == 25.4 && universal.forceScaleFactor == 1.0));
    const FrfComponentCard database = cardAt<FrfComponentTable>(model, 2);
    REQUIRE((database.name.empty() && database.medium == FrfMedium::Database && database.unitNumber == 0));
    const FrfComponentCard lower = cardAt<FrfComponentTable>(model, 3);
    REQUIRE((lower.name == "Body" &&
             lower.medium == FrfMedium::UniversalFile)); // the name as written, the word in any case
    REQUIRE((lower.lengthScaleFactor == 0.5 && lower.forceScaleFactor == 2.5));
    // A database has no unit: a number given with DB is kept.
    REQUIRE(cardAt<FrfComponentTable>(readBulk("FrfComponent;50;X;Database;7")).unitNumber == 7);
    REQUIRE(visitedBy<FrfComponentTable>(model, 0).fields.empty()); // the card names no other card
    requireRoundTrip(model);

    // The defaults are left out of the written card; a blank name is a blank field.

    // The values that an edit may not give a card.
    FrfComponentCard edited = cardAt<FrfComponentTable>(model, 0);
    REQUIRE_FALSE(FrfComponentFormat::check(edited).has_value());
    edited.unitNumber = 0; // a binary result file needs its unit
    REQUIRE(problemField(FrfComponentFormat::check(edited)) == "unitNumber");
    edited = cardAt<FrfComponentTable>(model, 0);
    edited.name = "ConnectionInfo";
    REQUIRE(problemField(FrfComponentFormat::check(edited)) == "componentName");
    edited.name = "two words";
    REQUIRE_FALSE(FrfComponentFormat::check(edited).has_value());

    // The ID is unique among the FRFCOMP cards.
    REQUIRE_THROWS_AS(readBulk("FrfComponent;10;A;Database\nFrfComponent;10;B;Database"), base::DeckError);

    expectInvalid("FrfComponent;0;A;Database", "id");
    expectInvalid("FrfComponent;-3;A;Database", "id");
    expectInvalid("FrfComponent;10;A", "medium"); // required
    expectInvalid("FrfComponent;10;A;TAPE", "medium");
    expectInvalid("FrfComponent;10;A;BinaryResultsFile", "unitNumber"); // a binary or universal file needs a unit
    expectInvalid("FrfComponent;10;A;UniversalFile", "unitNumber");
    expectInvalid("FrfComponent;10;A;UniversalFile;-3", "unitNumber");
    expectInvalid("FrfComponent;10;A;UniversalFile;30;0.", "lengthScaleFactor");
    expectInvalid("FrfComponent;10;A;UniversalFile;30;-1.", "lengthScaleFactor");
    expectInvalid("FrfComponent;10;A;UniversalFile;30;1.;0.", "forceScaleFactor");
    expectInvalid("FrfComponent;10;All;Database", "componentName"); // the words of the ASMOUT keyword
    expectInvalid("FrfComponent;10;ConnectionInfo;Database", "componentName");
    expectInvalid("FrfComponent;10;Component;Database", "componentName");
    expectInvalid("FrfComponent;10;Assembly;Database", "componentName");
    oor::testing::requireModelFileRoundTrip(readModelText("FrfComponent;10;TOOLONGNAME;Database", true));
    expectInvalid("FrfComponent;10;123;Database", "componentName"); // a whole number would be read as an ID
}

TEST_CASE("an FRFCONN joins a point of one FRF component to a point of another, each given by ID or by name",
          "[model][cards][frf]")
{
    REQUIRE((FrfConnectionTable::INFO.category == CardCategory::Other &&
             FrfConnectionTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model =
        readBulk(COMPONENTS_TEXT +
                 "FrfConnection;10;5;100;15;200\nFrfConnection;20;BODY;25;WING;35\nFrfConnection;30;Body;25;15;35");
    const FrfConnectionCard byId = cardAt<FrfConnectionTable>(model, 0);
    REQUIRE((byId.id == 10 && byId.points[0].component == FrfComponentRef::byId(5) && byId.points[0].point == 100));
    REQUIRE((byId.points[1].component == FrfComponentRef::byId(15) && byId.points[1].point == 200));
    const FrfConnectionCard byName = cardAt<FrfConnectionTable>(model, 1);
    REQUIRE((byName.points[0].component == FrfComponentRef::byName("BODY") && byName.points[0].point == 25));
    REQUIRE((byName.points[1].component == FrfComponentRef::byName("WING") && byName.points[1].point == 35));
    const FrfConnectionCard mixed = cardAt<FrfConnectionTable>(model, 2);
    // The name is kept as written; the match with the FRFCOMP does not look at the case.
    REQUIRE((mixed.points[0].component.name == "Body" && mixed.points[0].component.id == 0));
    REQUIRE((mixed.points[1].component.id == 15 && mixed.points[1].component.name.empty()));
    requireRoundTrip(model);

    // Only a component given by its ID is a reference; a name is not an ID, and a point belongs to the component.
    const Visited visitedIds = visitedBy<FrfConnectionTable>(model, 0);
    REQUIRE(visitedIds.fields == std::vector<std::string>{"frfComponent1", "frfComponent2"});
    REQUIRE(visitedIds.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::FrfComponent));
    REQUIRE(visitedBy<FrfConnectionTable>(model, 1).fields.empty());
    REQUIRE(visitedBy<FrfConnectionTable>(model, 2).fields == std::vector<std::string>{"frfComponent2"});
    REQUIRE(unresolvedIn(model) == 0);

    // A component that no FRFCOMP has is an unresolved reference when it is given by ID, and nothing when it is a name.
    Model missing = readBulk("FrfComponent;5;ONE;Database\nFrfConnection;10;5;100;16;200");
    REQUIRE(unresolvedIn(missing) == 1);
    Model namesOnly = readBulk("FrfConnection;10;ELSE;100;OTHER;200");
    REQUIRE(unresolvedIn(namesOnly) == 0);

    // The IDs of the components follow a renumbering of the FRF components, the names and the points do not move.
    Renumbering::apply(model, RenumberSpace::FrfComponent, Renumbering::offset(1000));
    REQUIRE((cardAt<FrfComponentTable>(model, 0).id == 1005 && cardAt<FrfComponentTable>(model, 3).id == 1040));
    REQUIRE((cardAt<FrfConnectionTable>(model, 0).points[0].component.id == 1005 &&
             cardAt<FrfConnectionTable>(model, 0).points[1].component.id == 1015));
    REQUIRE((cardAt<FrfConnectionTable>(model, 1).points[0].component.name == "BODY" &&
             cardAt<FrfConnectionTable>(model, 1).points[0].component.id == 0));
    REQUIRE(cardAt<FrfConnectionTable>(model, 2).points[1].component.id == 1015);
    REQUIRE(
        (cardAt<FrfConnectionTable>(model, 0).points[0].point == 100 && cardAt<FrfConnectionTable>(model, 0).id == 10));
    REQUIRE(unresolvedIn(model) == 0);
    // A renumbering of the nodes leaves the points of the components alone.
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<FrfConnectionTable>(model, 0).points[0].point == 100 &&
             cardAt<FrfConnectionTable>(model, 1).points[1].point == 35));

    // A deleted component leaves the references to it unresolved.
    model.remove(*model.locateInType(typeIndexOf<FrfComponentTable>(), 1005));
    REQUIRE(unresolvedIn(model) == 1);

    // The CONNID is unique among the cards.
    REQUIRE_THROWS_AS(readBulk("FrfConnection;10;5;100;15;200\nFrfConnection;10;6;100;15;200"), base::DeckError);

    expectInvalid("FrfConnection;0;5;100;15;200", "id");
    expectInvalid("FrfConnection;10;;100;15;200", "frfComponent1");
    expectInvalid("FrfConnection;10;-5;100;15;200", "frfComponent1");
    oor::testing::requireModelFileRoundTrip(
        readModelText("FrfConnection;10;TOOLONGNAME;100;15;200", true)); // the field holds the ID or the name
    expectInvalid("FrfConnection;10;5;;15;200", "point1");
    expectInvalid("FrfConnection;10;5;0;15;200", "point1");
    expectInvalid("FrfConnection;10;5;-4;15;200", "point1");
    expectInvalid("FrfConnection;10;5;abc;15;200", "point1");
    expectInvalid("FrfConnection;10;5;100;;200", "frfComponent2");
    expectInvalid("FrfConnection;10;5;100;15", "point2");
}

TEST_CASE(
    "an FRFFLEX gives the stiffness, damping and damping coefficient of a flexible connection as numbers or tables",
    "[model][cards][frf]")
{
    REQUIRE((FrfFlexibleConnectionTable::INFO.category == CardCategory::Other &&
             FrfFlexibleConnectionTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(COMPONENTS_TEXT + tabled(10) + tabled(11) + tabled(12) + tabled(13) + tabled(4150) +
                           "FrfFlexibleConnection;10;2;5;1207;15;2311;;62.5;4150;;;0.035\n"
                           "FrfFlexibleConnection;20;5;BODY;1340;WING;2460;10;;;0.375\n"
                           "FrfFlexibleConnection;30;;5;1;15;2;;;;0.5\n"
                           "FrfFlexibleConnection;40;3;BODY;1;WING;2;11;;12;;13\n"
                           "FrfFlexibleConnection;50;;5;1;15;2;;5.5;;;;0.125");
    // A stiffness value, a damping table and a coefficient value (an integer in field 8 is the table 4150).
    const FrfFlexibleConnectionCard first = cardAt<FrfFlexibleConnectionTable>(model, 0);
    REQUIRE((first.id == 10 && first.components == 2 && first.points[0].component.id == 5 &&
             first.points[0].point == 1207));
    REQUIRE((first.points[1].component.id == 15 && first.points[1].point == 2311));
    REQUIRE((first.stiffness.value == 62.5 && first.stiffness.table == 0));
    REQUIRE((first.damping.value == 0.0 && first.damping.table == 4150));
    REQUIRE((first.structuralDamping.value == 0.035 && first.structuralDamping.table == 0));
    // A table for the stiffness and a number for the damping, no continuation line.
    const FrfFlexibleConnectionCard second = cardAt<FrfFlexibleConnectionTable>(model, 1);
    REQUIRE((second.components == 5 && second.points[0].component.name == "BODY" &&
             second.points[1].component.name == "WING"));
    REQUIRE((second.stiffness.table == 10 && second.stiffness.value == 0.0 && second.damping.value == 0.375 &&
             second.structuralDamping.isBlank()));
    const FrfFlexibleConnectionCard scalar =
        cardAt<FrfFlexibleConnectionTable>(model, 2); // C blank is a scalar point, the stiffness may be blank
    REQUIRE((scalar.components == 0 && scalar.stiffness.isBlank() && scalar.damping.value == 0.5));
    const FrfFlexibleConnectionCard tables = cardAt<FrfFlexibleConnectionTable>(model, 3);
    REQUIRE((tables.stiffness.table == 11 && tables.damping.table == 12 && tables.structuralDamping.table == 13));
    const FrfFlexibleConnectionCard coefficient = cardAt<FrfFlexibleConnectionTable>(model, 4);
    REQUIRE((coefficient.stiffness.value == 5.5 && coefficient.damping.isBlank() &&
             coefficient.structuralDamping.value == 0.125));
    requireRoundTrip(model);

    // The references: the components given by ID and the TABLEDi.
    const Visited first1 = visitedBy<FrfFlexibleConnectionTable>(model, 0);
    REQUIRE(first1.fields == std::vector<std::string>{"frfComponent1", "frfComponent2", "dampingTableId"});
    REQUIRE(first1.targets == std::vector<ReferenceTarget>{ReferenceTarget::FrfComponent, ReferenceTarget::FrfComponent,
                                                           ReferenceTarget::DynamicTable});
    const Visited second1 = visitedBy<FrfFlexibleConnectionTable>(model, 1);
    REQUIRE(second1.fields == std::vector<std::string>{"stiffnessTableId"});
    REQUIRE(second1.targets == std::vector<ReferenceTarget>{ReferenceTarget::DynamicTable});
    const Visited tables1 = visitedBy<FrfFlexibleConnectionTable>(model, 3);
    REQUIRE(tables1.fields ==
            std::vector<std::string>{"stiffnessTableId", "dampingTableId", "structuralDampingTableId"});
    REQUIRE(tables1.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::DynamicTable));
    REQUIRE(unresolvedIn(model) == 0);

    // A table that no card has is reported; a number is not a reference.
    Model missing = readBulk(COMPONENTS_TEXT + "FrfFlexibleConnection;10;2;5;1207;15;2311;;62.5;4150");
    REQUIRE(unresolvedIn(missing) == 1);

    // The numbers of the tables follow a renumbering of the dynamic tables and the numbers of the properties do not.
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(1000));
    REQUIRE((cardAt<FrfFlexibleConnectionTable>(model, 0).damping.table == 5150 &&
             cardAt<FrfFlexibleConnectionTable>(model, 0).stiffness.value == 62.5));
    REQUIRE((cardAt<FrfFlexibleConnectionTable>(model, 1).stiffness.table == 1010 &&
             cardAt<FrfFlexibleConnectionTable>(model, 1).damping.value == 0.375));
    REQUIRE((cardAt<FrfFlexibleConnectionTable>(model, 3).structuralDamping.table == 1013 &&
             cardAt<FrfFlexibleConnectionTable>(model, 3).id == 40));
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::FrfComponent, Renumbering::offset(100));
    REQUIRE((cardAt<FrfFlexibleConnectionTable>(model, 0).points[0].component.id == 105 &&
             cardAt<FrfFlexibleConnectionTable>(model, 0).points[0].point == 1207));
    REQUIRE(cardAt<FrfFlexibleConnectionTable>(model, 1).points[0].component.name == "BODY");
    REQUIRE(unresolvedIn(model) == 0);

    // The rules of the card: stiffness or damping, and a damping coefficient only with a stiffness.
    FrfFlexibleConnectionCard edited = cardAt<FrfFlexibleConnectionTable>(model, 0);
    REQUIRE_FALSE(FrfFlexibleConnectionFormat::check(edited).has_value());
    edited.stiffness = FrfConnectionValue{};
    REQUIRE(problemField(FrfFlexibleConnectionFormat::check(edited)) ==
            "structuralDamping"); // the damping coefficient needs a stiffness
    edited.structuralDamping = FrfConnectionValue{};
    edited.damping = FrfConnectionValue{};
    REQUIRE(problemField(FrfFlexibleConnectionFormat::check(edited)) == "stiffness"); // nothing is given
    edited = cardAt<FrfFlexibleConnectionTable>(model, 0);
    edited.stiffness.table = 7; // a number and a table together
    REQUIRE(problemField(FrfFlexibleConnectionFormat::check(edited)) == "stiffness");

    const std::string prefix = "FrfFlexibleConnection;10;1;100;15;200;25";
    expectInvalid("FrfFlexibleConnection;0;1;100;15;200;25;;1.", "id");
    expectInvalid("FrfFlexibleConnection;10;7;100;15;200;25;;1.", "components");
    expectInvalid("FrfFlexibleConnection;10;-1;100;15;200;25;;1.", "components");
    expectInvalid("FrfFlexibleConnection;10;1;;15;200;25;;1.", "frfComponent1");
    expectInvalid("FrfFlexibleConnection;10;1;100;;200;25;;1.", "point1");
    expectInvalid("FrfFlexibleConnection;10;1;100;15;;25;;1.", "frfComponent2");
    expectInvalid("FrfFlexibleConnection;10;1;100;15;200;;;1.", "point2");
    expectInvalid(prefix, "stiffness"); // the stiffness and the damping are not both blank
    expectInvalid(prefix + ";;;;0.5;;0.1", "structuralDamping");
    expectInvalid(prefix + ";;0.", "stiffness"); // a real greater than zero
    expectInvalid(prefix + ";;-2.5", "stiffness");
    expectInvalid(prefix + ";;1.;;0.", "damping");
    expectInvalid(prefix + ";;1.;;1.;;-0.5", "structuralDamping");
    expectInvalid(prefix + ";;abc", "stiffness");
    expectInvalid(prefix + ";0", "stiffnessTableId"); // a table ID greater than zero
    expectInvalid(prefix + ";-3", "stiffnessTableId");
    expectInvalid(prefix + ";;1.;0", "dampingTableId");
    REQUIRE_THROWS_AS(
        readBulk("FrfFlexibleConnection;10;1;100;15;200;25;;1.\nFrfFlexibleConnection;10;1;100;15;200;25;;1."),
        base::DeckError);
}

TEST_CASE("an FRFRELS releases components of up to three points and several cards may share its number",
          "[model][cards][frf]")
{
    REQUIRE((FrfReleasedDofsTable::INFO.category == CardCategory::Other &&
             FrfReleasedDofsTable::INFO.idSpace == base::IdSpace::Group));
    Model model = readBulk(COMPONENTS_TEXT +
                           "FrfReleasedDofs;10;45;5;15;BODY;20;WING;30\n"
                           "FrfReleasedDofs;20;1;BODY;25;WING;35\n"
                           "FrfReleasedDofs;30;321;ONE;1\n"
                           "FrfReleasedDofs;40;2;5;6;;;15;10\n"
                           "FrfReleasedDofs;10;6;40;50");
    REQUIRE(aliveCards<FrfReleasedDofsTable>(model) == 5);
    const FrfReleasedDofsCard three = cardAt<FrfReleasedDofsTable>(model, 0);
    REQUIRE((three.id == 10 && three.components == 45));
    REQUIRE((three.points[0].component == FrfComponentRef::byId(5) && three.points[0].point == 15));
    REQUIRE((three.points[1].component == FrfComponentRef::byName("BODY") && three.points[1].point == 20));
    REQUIRE((three.points[2].component == FrfComponentRef::byName("WING") && three.points[2].point == 30));
    REQUIRE(cardAt<FrfReleasedDofsTable>(model, 2).components == 123); // the digits in ascending order
    // A group that is left out stays blank, even when a later one is there.
    const FrfReleasedDofsCard gap = cardAt<FrfReleasedDofsTable>(model, 3);
    REQUIRE((gap.points[1].isBlank() && gap.points[2].component.id == 15 && gap.points[2].point == 10));
    REQUIRE(cardAt<FrfReleasedDofsTable>(model, 1).points[2].isBlank());
    requireRoundTrip(model);
    // Cards of one number are one set of releases (the first and the last card have the number 10).
    REQUIRE((cardAt<FrfReleasedDofsTable>(model, 4).id == 10 && cardAt<FrfReleasedDofsTable>(model, 0).id == 10));
    REQUIRE(cardsOfSet(model, CardCategory::Other, 10).size() == 2);

    const Visited visited = visitedBy<FrfReleasedDofsTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"frfComponent1"}); // the others are names
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::FrfComponent});
    REQUIRE(visitedBy<FrfReleasedDofsTable>(model, 3).fields ==
            std::vector<std::string>{"frfComponent1", "frfComponent3"});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::FrfComponent, Renumbering::offset(1000));
    REQUIRE((cardAt<FrfReleasedDofsTable>(model, 0).points[0].component.id == 1005 &&
             cardAt<FrfReleasedDofsTable>(model, 3).points[2].component.id == 1015));
    REQUIRE(cardAt<FrfReleasedDofsTable>(model, 0).points[1].component.name == "BODY");
    REQUIRE((cardAt<FrfReleasedDofsTable>(model, 0).id == 10 &&
             cardAt<FrfReleasedDofsTable>(model, 0).points[0].point == 15));
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("FrfReleasedDofs;0;1;5;6", "id");
    expectInvalid("FrfReleasedDofs;10;;5;6", "components");  // required
    expectInvalid("FrfReleasedDofs;10;0;5;6", "components"); // a scalar point has nothing to release
    expectInvalid("FrfReleasedDofs;10;7;5;6", "components");
    expectInvalid("FrfReleasedDofs;10;1;;6", "frfComponent1");
    expectInvalid("FrfReleasedDofs;10;1;5", "node1");
    expectInvalid("FrfReleasedDofs;10;1;5;6;7", "node2"); // half a group
    expectInvalid("FrfReleasedDofs;10;1;5;6;;8", "frfComponent2");
    expectInvalid("FrfReleasedDofs;10;1;5;6;7;8;9", "node3");
}

TEST_CASE("an FRFSPC1 is a constraint set that the Case Control SPC selects", "[model][cards][frf]")
{
    REQUIRE((FrfSinglePointConstrTable::INFO.category == CardCategory::Constraint &&
             FrfSinglePointConstrTable::INFO.idSpace == base::IdSpace::Group));
    Model model = readBulk(COMPONENTS_TEXT +
                           "FrfSinglePointConstr;10;4;5;12;BODY;23;WING;31\n"
                           "FrfSinglePointConstr;20;1;BODY;25;WING;35\n"
                           "FrfSinglePointConstr;30;;5;8\n"
                           "FrfSinglePointConstr;40;6532;5;4;ONE;5\n"
                           "FrfSinglePointConstr;10;3;15;9");
    REQUIRE(aliveCards<FrfSinglePointConstrTable>(model) == 5);
    const FrfSinglePointConstrCard three = cardAt<FrfSinglePointConstrTable>(model, 0);
    REQUIRE((three.id == 10 && three.components == 4));
    REQUIRE((three.points[0].component == FrfComponentRef::byId(5) && three.points[0].point == 12));
    REQUIRE((three.points[1].component == FrfComponentRef::byName("BODY") && three.points[1].point == 23));
    REQUIRE((three.points[2].component == FrfComponentRef::byName("WING") && three.points[2].point == 31));
    REQUIRE(cardAt<FrfSinglePointConstrTable>(model, 2).components == 0);    // a blank C is a scalar point
    REQUIRE(cardAt<FrfSinglePointConstrTable>(model, 3).components == 2356); // the digits in ascending order
    REQUIRE(cardAt<FrfSinglePointConstrTable>(model, 1).points[2].isBlank());
    requireRoundTrip(model);

    // the blank C of a scalar point

    // Cards of one number are one constraint set, found by the targets of the constraint sets, as the SPC1 cards are.
    const std::span<const CardCategory> categories = categoriesOfSet(ReferenceTarget::ConstraintSet);
    REQUIRE((categories.size() == 1 && categories.front() == CardCategory::Constraint));
    const std::optional<CardLocation> found = resolveReference(model, ReferenceTarget::ConstraintSet, 10);
    REQUIRE((found.has_value() && found->table == typeIndexOf<FrfSinglePointConstrTable>()));
    REQUIRE(cardsOfSet(model, CardCategory::Constraint, 10).size() == 2);

    // The Case Control SPC names it, and follows its renumbering.
    Model selected = readModelText(
        "[executive]\nSolutionSequence;101\n[case]\nSubcaseStart;1\nSinglePointConstrSelection;10\n[model]"
        "\nFrfComponent;5;ONE;Database\nFrfSinglePointConstr;10;4;5;12\n",
        true);
    REQUIRE(unresolvedIn(selected) == 0);
    Renumbering::apply(selected, RenumberSpace::ConstraintSet, Renumbering::offset(1000));
    REQUIRE(cardAt<FrfSinglePointConstrTable>(selected).id == 1010);
    REQUIRE(unresolvedIn(selected) == 0); // the Case Control command followed
    Model unselected = readModelText(
        "[executive]\nSolutionSequence;101\n[case]\nSubcaseStart;1\nSinglePointConstrSelection;11\n[model]"
        "\nFrfComponent;5;ONE;Database\nFrfSinglePointConstr;10;4;5;12\n",
        true);
    REQUIRE(unresolvedIn(unselected) == 1); // no constraint set 11

    // The components of the cards follow the FRF components, the points do not move with the nodes.
    REQUIRE(visitedBy<FrfSinglePointConstrTable>(model, 0).fields == std::vector<std::string>{"frfComponent1"});
    REQUIRE(visitedBy<FrfSinglePointConstrTable>(model, 0).targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::FrfComponent});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::FrfComponent, Renumbering::offset(1000));
    REQUIRE((cardAt<FrfSinglePointConstrTable>(model, 0).points[0].component.id == 1005 &&
             cardAt<FrfSinglePointConstrTable>(model, 4).points[0].component.id == 1015));
    REQUIRE(cardAt<FrfSinglePointConstrTable>(model, 0).points[1].component.name == "BODY");
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<FrfSinglePointConstrTable>(model, 0).points[0].point == 12);

    expectInvalid("FrfSinglePointConstr;0;1;5;6", "id");
    expectInvalid("FrfSinglePointConstr;10;7;5;6", "components");
    expectInvalid("FrfSinglePointConstr;10;17;5;6", "components");
    expectInvalid("FrfSinglePointConstr;10;1;;6", "frfComponent1");
    expectInvalid("FrfSinglePointConstr;10;1;5", "point1");
    oor::testing::requireModelFileRoundTrip(readModelText("FrfSinglePointConstr;10;1;TOOLONGNAME;6", true));
    expectInvalid("FrfSinglePointConstr;10;1;5;6;7", "point2");
    expectInvalid("FrfSinglePointConstr;10;1;5;6;;8", "frfComponent2");
    expectInvalid("FrfSinglePointConstr;10;1;5;6;7;8;9", "point3");
}

TEST_CASE("an FRFXIT names a point of the model, a component and a label over six fields", "[model][cards][frf]")
{
    REQUIRE((FrfExcitationDofTable::INFO.category == CardCategory::Other &&
             FrfExcitationDofTable::INFO.idSpace == base::IdSpace::None));
    Model model = readBulk(
        "Node;10;;0.;0.;0.\n"
        "ScalarPoint;20\n"
        "FrfExcitationDof;10;3;SHAKER INPUT AT REAR MOUNT\n"
        "FrfExcitationDof;20;;DAMPER END\n"
        "FrfExcitationDof;30;6;THE SHAKER FORCE ON THE REAR MOUNT OF GEARBOX B7\n"
        "FrfExcitationDof;10;2;Shaker pull Y\n"
        "FrfExcitationDof;10;3;SHAKER INPUT AT REAR MOUNT");
    REQUIRE(aliveCards<FrfExcitationDofTable>(model) == 5); // no ID: a deck may repeat a load, the second is ignored
    const FrfExcitationDofCard first = cardAt<FrfExcitationDofTable>(model, 0);
    REQUIRE((first.pointId == 10 && first.components == 3 && first.entryLabel == "SHAKER INPUT AT REAR MOUNT"));
    const FrfExcitationDofCard scalar = cardAt<FrfExcitationDofTable>(model, 1);
    REQUIRE((scalar.pointId == 20 && scalar.components == 0 &&
             scalar.entryLabel == "DAMPER END")); // a blank C is a scalar point
    const FrfExcitationDofCard longest = cardAt<FrfExcitationDofTable>(model, 2);
    REQUIRE((longest.components == 6 && longest.entryLabel == "THE SHAKER FORCE ON THE REAR MOUNT OF GEARBOX B7"));
    REQUIRE(longest.entryLabel.size() == FRF_EXCITATION_LABEL_LENGTH);
    REQUIRE(cardAt<FrfExcitationDofTable>(model, 3).entryLabel ==
            "Shaker pull Y"); // the case is kept, blanks inside the label too
    requireRoundTrip(model);

    // The small format cuts the label in pieces of eight characters, the free format writes it whole.

    // The point is a grid point or a scalar point of the model; a scalar point that no card defines is not reported.
    const Visited visited = visitedBy<FrfExcitationDofTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"pointId"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::ScalarOrGridPoint});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<FrfExcitationDofTable>(model, 0).pointId == 1010 &&
             cardAt<FrfExcitationDofTable>(model, 1).pointId == 1020));
    REQUIRE(cardAt<FrfExcitationDofTable>(model, 0).entryLabel == "SHAKER INPUT AT REAR MOUNT");
    requireRoundTrip(model);

    // The label is not blank and begins in its first field.
    FrfExcitationDofCard edited = cardAt<FrfExcitationDofTable>(model, 0);
    REQUIRE_FALSE(FrfExcitationDofFormat::check(edited).has_value());
    edited.entryLabel = " LEADING BLANK";
    REQUIRE_FALSE(FrfExcitationDofFormat::check(edited).has_value());
    edited.entryLabel = "TRAILING BLANK ";
    REQUIRE_FALSE(FrfExcitationDofFormat::check(edited).has_value());
    edited.entryLabel = "A,B";
    REQUIRE_FALSE(FrfExcitationDofFormat::check(edited).has_value());
    edited.entryLabel = std::string(149, 'X');
    REQUIRE_FALSE(FrfExcitationDofFormat::check(edited).has_value());

    expectInvalid("FrfExcitationDof;0;3;A", "pointId");
    expectInvalid("FrfExcitationDof;-4;3;A", "pointId");
    expectInvalid("FrfExcitationDof;10;7;A", "components");
    expectInvalid("FrfExcitationDof;10;12;A", "components"); // a single component
    expectInvalid("FrfExcitationDof;10;3", "entryLabel");
    oor::testing::requireModelFileRoundTrip(readModelText(
        "FrfExcitationDof;10;3;THE SHAKER FORCE ON THE REAR MOUNT OF GEARBOX B72", true)); // 49 characters
}

TEST_CASE("an FRFXIT1 lists points with THRU ranges and the components they share", "[model][cards][frf]")
{
    REQUIRE((FrfExcitationDofListTable::INFO.category == CardCategory::Other &&
             FrfExcitationDofListTable::INFO.idSpace == base::IdSpace::None));
    Model model = readBulk(
        "Node;5;;0.;0.;0.\n"
        "Node;6;;1.;0.;0.\n"
        "ScalarPoint;7\n"
        "FrfExcitationDofList;123;10;20;30;40\n"
        "FrfExcitationDofList;123;5;Through;15\n"
        "FrfExcitationDofList;;7\n"
        "FrfExcitationDofList;321;1;2;Through;4;9\n"
        "FrfExcitationDofList;246;1301;1302;1303;1304;1305;1306;1307");
    REQUIRE(aliveCards<FrfExcitationDofListTable>(model) == 5);
    REQUIRE(cardAt<FrfExcitationDofListTable>(model, 0).components == 123);
    REQUIRE(cardAt<FrfExcitationDofListTable>(model, 0).pointIds ==
            std::vector<IdRun>{{10, 10}, {20, 20}, {30, 30}, {40, 40}});
    REQUIRE(cardAt<FrfExcitationDofListTable>(model, 1).pointIds == std::vector<IdRun>{{5, 15}});
    REQUIRE((cardAt<FrfExcitationDofListTable>(model, 2).components == 0 &&
             cardAt<FrfExcitationDofListTable>(model, 2).pointIds.size() == 1)); // scalar
    REQUIRE(cardAt<FrfExcitationDofListTable>(model, 3).components == 123);      // the digits in ascending order
    REQUIRE(cardAt<FrfExcitationDofListTable>(model, 3).pointIds == std::vector<IdRun>{{1, 1}, {2, 4}, {9, 9}});
    REQUIRE(cardAt<FrfExcitationDofListTable>(model, 4).pointIds.size() == 7); // seven points fill the first line
    requireRoundTrip(model);

    // Consecutive points become a range only on request.
    const Model listed = readBulk("FrfExcitationDofList;123;1;2;3;4;5\nFrfExcitationDofList;123;7;8");

    // A single point is a grid point or a scalar point; the ends of a range are nodes that may be missing.
    const Visited singles = visitedBy<FrfExcitationDofListTable>(model, 0);
    REQUIRE(singles.fields == std::vector<std::string>(4, "pointIds"));
    REQUIRE(singles.targets == std::vector<ReferenceTarget>(4, ReferenceTarget::ScalarOrGridPoint));
    const Visited range = visitedBy<FrfExcitationDofListTable>(model, 1);
    REQUIRE(range.fields == std::vector<std::string>(2, "pointIds"));
    REQUIRE(range.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::NodeInRange));
    const Visited mixed = visitedBy<FrfExcitationDofListTable>(model, 3);
    REQUIRE(mixed.targets == std::vector<ReferenceTarget>{ReferenceTarget::ScalarOrGridPoint,
                                                          ReferenceTarget::NodeInRange, ReferenceTarget::NodeInRange,
                                                          ReferenceTarget::ScalarOrGridPoint});
    REQUIRE(unresolvedIn(model) == 0); // none of them is reported when it does not exist
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<FrfExcitationDofListTable>(model, 1).pointIds == std::vector<IdRun>{{105, 115}});
    REQUIRE(cardAt<FrfExcitationDofListTable>(model, 2).pointIds == std::vector<IdRun>{{107, 107}}); // the SPOINT 7
    REQUIRE(cardAt<FrfExcitationDofListTable>(model, 3).pointIds ==
            std::vector<IdRun>{{101, 101}, {102, 104}, {109, 109}});
    REQUIRE(cardAt<FrfExcitationDofListTable>(model, 0).components == 123);

    expectInvalid("FrfExcitationDofList;7;10", "components");
    expectInvalid("FrfExcitationDofList;17;10", "components");
    expectInvalid("FrfExcitationDofList;123", "pointIds"); // at least one point
    expectInvalid("FrfExcitationDofList;123;0", "pointIds");
    expectInvalid("FrfExcitationDofList;123;10;Through", "Through");
    expectInvalid("FrfExcitationDofList;123;Through;10", "Through");
    expectInvalid("FrfExcitationDofList;123;10;Through;5", "pointIds");
    expectInvalid("FrfExcitationDofList;123;10;abc", "pointIds");
    oor::testing::requireModelFileRoundTrip(listed);
}

TEST_CASE(
    "an FBADLAY, an FBALOAD and an FBAPHAS are the delay, load and phase sets that RLOAD1, RLOAD2 and ACSRCE name",
    "[model][cards][frf]")
{
    // The categories and ID spaces of the cards they stand for (DELAY, DAREA, DPHASE), so that the references resolve.
    REQUIRE((FrfAssemblyLoadDelayTable::INFO.category == CardCategory::Delay &&
             FrfAssemblyLoadDelayTable::INFO.idSpace == base::IdSpace::Group));
    REQUIRE((FrfAssemblyLoadScaleTable::INFO.category == CardCategory::DynamicLoad &&
             FrfAssemblyLoadScaleTable::INFO.idSpace == base::IdSpace::Group));
    REQUIRE((FrfAssemblyLoadPhaseTable::INFO.category == CardCategory::PhaseLead &&
             FrfAssemblyLoadPhaseTable::INFO.idSpace == base::IdSpace::Group));

    Model model = readBulk(COMPONENTS_TEXT +
                           "FrfAssemblyLoadDelay;15;BODY;10;3;0.5\n"
                           "FrfAssemblyLoadDelay;25;30;5;2;0.1\n"
                           "FrfAssemblyLoadDelay;15;WING;7;;-0.25\n"
                           "FrfAssemblyLoadScale;7;WING;20;3;2.5\n"
                           "FrfAssemblyLoadScale;8;30;25;1;1.5\n"
                           "FrfAssemblyLoadScale;7;TWO;7;0;-1.\n"
                           "FrfAssemblyLoadPhase;16;5;40;1;10.\n"
                           "FrfAssemblyLoadPhase;26;FRAME;10;2;5.\n"
                           "FrfAssemblyLoadPhase;16;TWO;7;;-45.\n"
                           "FreqResponseLoad1;100;7;15;;16;;;;;3.\n"
                           "FreqResponseLoad2;101;8;25;;26;;;3.;;4.\n"
                           "AcousticSourceLoad;300;7;15;;16;;;2.;1.5;0.5");
    const FrfAssemblyLoadDelayCard delay = cardAt<FrfAssemblyLoadDelayTable>(model, 0);
    REQUIRE((delay.id == 15 && delay.frfComponent == FrfComponentRef::byName("BODY") && delay.pointId == 10));
    REQUIRE((delay.components == 3 && delay.delay == 0.5));
    const FrfAssemblyLoadDelayCard byId = cardAt<FrfAssemblyLoadDelayTable>(model, 1);
    REQUIRE((byId.frfComponent == FrfComponentRef::byId(30) && byId.pointId == 5 && byId.components == 2 &&
             byId.delay == 0.1));
    const FrfAssemblyLoadDelayCard scalarDelay = cardAt<FrfAssemblyLoadDelayTable>(model, 2);
    REQUIRE((scalarDelay.components == 0 &&
             scalarDelay.delay == -0.25)); // a blank C is a scalar point, a delay may be negative
    const FrfAssemblyLoadScaleCard load = cardAt<FrfAssemblyLoadScaleTable>(model, 0);
    REQUIRE((load.id == 7 && load.frfComponent == FrfComponentRef::byName("WING") && load.pointId == 20));
    REQUIRE((load.components == 3 && load.scaleFactor == 2.5));
    REQUIRE((cardAt<FrfAssemblyLoadScaleTable>(model, 1).frfComponent.id == 30 &&
             cardAt<FrfAssemblyLoadScaleTable>(model, 1).scaleFactor == 1.5));
    REQUIRE((cardAt<FrfAssemblyLoadScaleTable>(model, 2).components == 0 &&
             cardAt<FrfAssemblyLoadScaleTable>(model, 2).scaleFactor == -1.0));
    const FrfAssemblyLoadPhaseCard phase = cardAt<FrfAssemblyLoadPhaseTable>(model, 0);
    REQUIRE((phase.id == 16 && phase.frfComponent == FrfComponentRef::byId(5) && phase.pointId == 40));
    REQUIRE((phase.components == 1 && phase.phase == 10.0));
    REQUIRE((cardAt<FrfAssemblyLoadPhaseTable>(model, 1).frfComponent.name == "FRAME" &&
             cardAt<FrfAssemblyLoadPhaseTable>(model, 1).phase == 5.0));
    REQUIRE((cardAt<FrfAssemblyLoadPhaseTable>(model, 2).components == 0 &&
             cardAt<FrfAssemblyLoadPhaseTable>(model, 2).phase == -45.0));
    requireRoundTrip(model);

    // Only a component given by its ID is a reference; the point belongs to the component and is not looked up.
    REQUIRE(visitedBy<FrfAssemblyLoadDelayTable>(model, 0).fields.empty());
    const Visited delayVisited = visitedBy<FrfAssemblyLoadDelayTable>(model, 1);
    REQUIRE(delayVisited.fields == std::vector<std::string>{"frfComponent"});
    REQUIRE(delayVisited.targets == std::vector<ReferenceTarget>{ReferenceTarget::FrfComponent});
    REQUIRE(visitedBy<FrfAssemblyLoadScaleTable>(model, 1).targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::FrfComponent});
    REQUIRE(visitedBy<FrfAssemblyLoadPhaseTable>(model, 0).fields == std::vector<std::string>{"frfComponent"});
    REQUIRE(visitedBy<FrfAssemblyLoadPhaseTable>(model, 1).fields.empty());

    // The set IDs are found by the dynamic loads: the DELAY, the DPHASE and the EXCITEID of an RLOAD1, an RLOAD2 and an
    // ACSRCE name them as they name a DELAY, a DPHASE and a DAREA.
    REQUIRE(unresolvedIn(model) == 0);
    const std::optional<CardLocation> delaySet = resolveReference(model, ReferenceTarget::DelaySet, 15);
    REQUIRE((delaySet.has_value() && delaySet->table == typeIndexOf<FrfAssemblyLoadDelayTable>()));
    const std::optional<CardLocation> phaseSet = resolveReference(model, ReferenceTarget::PhaseSet, 26);
    REQUIRE((phaseSet.has_value() && phaseSet->table == typeIndexOf<FrfAssemblyLoadPhaseTable>()));
    const std::optional<CardLocation> excitation = resolveReference(model, ReferenceTarget::ExcitationSet, 8);
    REQUIRE((excitation.has_value() && excitation->table == typeIndexOf<FrfAssemblyLoadScaleTable>()));
    REQUIRE(cardsOfSet(model, CardCategory::Delay, 15).size() == 2);
    REQUIRE(cardsOfSet(model, CardCategory::DynamicLoad, 7).size() == 2);
    REQUIRE(cardsOfSet(model, CardCategory::PhaseLead, 16).size() == 2);
    // Without the cards the same dynamic loads name nothing: the EXCITEID, the DELAY and the DPHASE.
    Model alone = readBulk("FreqResponseLoad1;100;7;15;;16;;;;;3.");
    REQUIRE(unresolvedIn(alone) == 3);

    // A renumbering of the delays, the phases and the dynamic loads moves the sets and what names them.
    Renumbering::apply(model, RenumberSpace::Delay, Renumbering::offset(1000));
    REQUIRE((cardAt<FrfAssemblyLoadDelayTable>(model, 0).id == 1015 &&
             cardAt<FrfAssemblyLoadDelayTable>(model, 1).id == 1025));
    REQUIRE((cardAt<FreqResponseLoad1Table>(model).delayId == 1015 &&
             cardAt<FreqResponseLoad2Table>(model).delayId == 1025));
    REQUIRE(cardAt<AcousticSourceLoadTable>(model).delayId == 1015);
    Renumbering::apply(model, RenumberSpace::Phase, Renumbering::offset(2000));
    REQUIRE((cardAt<FrfAssemblyLoadPhaseTable>(model, 0).id == 2016 &&
             cardAt<FrfAssemblyLoadPhaseTable>(model, 1).id == 2026));
    REQUIRE((cardAt<FreqResponseLoad1Table>(model).phaseId == 2016 &&
             cardAt<FreqResponseLoad2Table>(model).phaseId == 2026));
    REQUIRE(cardAt<AcousticSourceLoadTable>(model).phaseId == 2016);
    Renumbering::apply(model, RenumberSpace::DynamicLoadSet, Renumbering::offset(3000));
    REQUIRE((cardAt<FrfAssemblyLoadScaleTable>(model, 0).id == 3007 &&
             cardAt<FrfAssemblyLoadScaleTable>(model, 1).id == 3008));
    REQUIRE((cardAt<FreqResponseLoad1Table>(model).excitationId == 3007 &&
             cardAt<FreqResponseLoad2Table>(model).excitationId == 3008));
    REQUIRE(cardAt<AcousticSourceLoadTable>(model).excitationId == 3007);
    REQUIRE(unresolvedIn(model) == 0);
    // The FRF components follow their own renumbering; the points are left to the components.
    Renumbering::apply(model, RenumberSpace::FrfComponent, Renumbering::offset(100));
    REQUIRE((cardAt<FrfAssemblyLoadDelayTable>(model, 1).frfComponent.id == 130 &&
             cardAt<FrfAssemblyLoadPhaseTable>(model, 0).frfComponent.id == 105));
    REQUIRE((cardAt<FrfAssemblyLoadScaleTable>(model, 1).frfComponent.id == 130 &&
             cardAt<FrfAssemblyLoadScaleTable>(model, 0).pointId == 20));
    REQUIRE(cardAt<FrfAssemblyLoadDelayTable>(model, 0).frfComponent.name == "BODY");
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("FrfAssemblyLoadDelay;0;BODY;10;3;0.5", "id");
    expectInvalid("FrfAssemblyLoadDelay;15;;10;3;0.5", "frfComponent");
    expectInvalid("FrfAssemblyLoadDelay;15;-2;10;3;0.5", "frfComponent");
    oor::testing::requireModelFileRoundTrip(readModelText("FrfAssemblyLoadDelay;15;TOOLONGNAME;10;3;0.5", true));
    expectInvalid("FrfAssemblyLoadDelay;15;BODY;;3;0.5", "pointId");
    expectInvalid("FrfAssemblyLoadDelay;15;BODY;0;3;0.5", "pointId");
    expectInvalid("FrfAssemblyLoadDelay;15;BODY;10;7;0.5", "components");
    expectInvalid("FrfAssemblyLoadDelay;15;BODY;10;-1;0.5", "components");
    expectInvalid("FrfAssemblyLoadDelay;15;BODY;10;3", "delay"); // required
    expectInvalid("FrfAssemblyLoadDelay;15;BODY;10;3;abc", "delay");
    expectInvalid("FrfAssemblyLoadScale;0;BODY;10;3;2.5", "id");
    expectInvalid("FrfAssemblyLoadScale;7;;10;3;2.5", "frfComponent");
    expectInvalid("FrfAssemblyLoadScale;7;BODY;;3;2.5", "pointId");
    expectInvalid("FrfAssemblyLoadScale;7;BODY;10;7;2.5", "components");
    expectInvalid("FrfAssemblyLoadScale;7;BODY;10;3", "scaleFactor");
    expectInvalid("FrfAssemblyLoadPhase;0;BODY;10;3;5.", "id");
    expectInvalid("FrfAssemblyLoadPhase;16;;10;3;5.", "frfComponent");
    expectInvalid("FrfAssemblyLoadPhase;16;BODY;0;3;5.", "pointId");
    expectInvalid("FrfAssemblyLoadPhase;16;BODY;10;9;5.", "components");
    expectInvalid("FrfAssemblyLoadPhase;16;BODY;10;3", "phase");
}
