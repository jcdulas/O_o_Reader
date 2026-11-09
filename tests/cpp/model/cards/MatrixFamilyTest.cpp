#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/matrices/MatrixCards.hpp"
#include "oor/model/cards/matrices/directMatrixAeroJ/DirectMatrixAeroJTable.hpp"
#include "oor/model/cards/matrices/directMatrixAxisym/DirectMatrixAxisymTable.hpp"
#include "oor/model/cards/matrices/monitorDisplacement/MonitorDisplacementTable.hpp"
#include "oor/model/cards/matrices/monitorElemLoad2/MonitorElemLoad2Table.hpp"
#include "oor/model/cards/matrices/monitorIntegratedLoad1/MonitorIntegratedLoad1Table.hpp"
#include "oor/model/cards/matrices/monitorNodeForceSum3/MonitorNodeForceSum3Table.hpp"
#include "oor/model/format/FieldWriter.hpp"
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

constexpr std::int32_t BLANK = DirectMatrixAxisymCard::NO_HARMONIC;

// The matrices of the aerodynamic mesh (DMIJ, DMIJI, DMIK): the layout of DMIG, a table each, so that one name can be
// a matrix of each kind.
const char* const AERO_MATRICES =
    "DirectMatrixAeroJ;AOAJ;0;9;2;0;;2\n"
    "DirectMatrixAeroJ;AOAJ;1;1;1;1;0.4;;2;1;0.6\n"
    "DirectMatrixAeroJ;AOAJ;2;1;1;1;0.7\n"
    "DirectMatrixAeroJInterference;WJI;0;9;1;0;;2\n"
    "DirectMatrixAeroJInterference;WJI;1;0;7;0;2.5;;8;0;3.5\n"
    "DirectMatrixAeroK;WTK;0;6;2;0\n"
    "DirectMatrixAeroK;WTK;3;1;3;1;6.;;4;2;7.\n"
    "DirectMatrixNodal;AOAJ;0;6;2;0\n"
    "DirectMatrixNodal;AOAJ;1;1;1;1;9.\n";

// An axisymmetric matrix: complex with harmonics, and a real rectangular one whose harmonic is zero (not blank).
const char* const AXISYMMETRIC_MATRICES =
    "DirectMatrixAxisym;AXK4;0;1;3;4\n"
    "DirectMatrixAxisym;AXK4;21;3;-3;614;3;5;3.75E+5;1250.;615;1;;1.5;0.5\n"
    "DirectMatrixAxisym;AXK4;22;0;;614;3;5;1.25;2.5\n"
    "DirectMatrixAxisym;REALM;0;2;1\n"
    "DirectMatrixAxisym;REALM;5;0;0;7;0;0;2.5\n";

} // namespace

TEST_CASE("DMIJ, DMIJI and DMIK are matrices of points like DMIG, each kind with its own names", "[model][matrices]")
{
    const Model model = readModelText(AERO_MATRICES, true);
    CHECK(pointMatrixNames(model, PointMatrixCard::AeroJ) == std::vector<std::string>{"AOAJ"});
    CHECK(pointMatrixNames(model, PointMatrixCard::AeroJInterference) == std::vector<std::string>{"WJI"});
    CHECK(pointMatrixNames(model, PointMatrixCard::AeroK) == std::vector<std::string>{"WTK"});
    // the DMIG of the same name is another matrix
    CHECK(pointMatrixNames(model, PointMatrixCard::Nodal) == std::vector<std::string>{"AOAJ"});
    CHECK(directMatrixNodalTerms(model, "AOAJ")->real == std::vector<double>{9.0});

    CHECK_FALSE(pointMatrixTerms(model, PointMatrixCard::AeroJ, "aoaj").has_value());
    const auto terms = pointMatrixTerms(model, PointMatrixCard::AeroJ, "AOAJ");
    REQUIRE(terms.has_value());
    CHECK(terms->form == 9);
    CHECK(terms->columnCount == 2);
    CHECK(terms->columnId == std::vector<base::EntityId>{1, 1, 2});
    CHECK(terms->rowId == std::vector<base::EntityId>{1, 2, 1});
    CHECK(terms->real == std::vector<double>{0.4, 0.6, 0.7});
    const CooMatrix coo = pointMatrixCoo(PointMatrixCard::AeroJ, "AOAJ", *terms);
    CHECK(coo.card == "DirectMatrixAeroJ");
    CHECK(coo.rowCount == 2);
    CHECK(coo.columnCount == 2); // IFO 9 with NCOL: the columns are numbered 1 to NCOL by GJ
    CHECK(coo.column == std::vector<std::int32_t>{0, 0, 1});
    CHECK(coo.row == std::vector<std::int32_t>{0, 1, 0});

    const auto symmetric = pointMatrixTerms(model, PointMatrixCard::AeroK, "WTK");
    REQUIRE(symmetric.has_value());
    const CooMatrix square = pointMatrixCoo(PointMatrixCard::AeroK, "WTK", *symmetric);
    CHECK(square.card == "DirectMatrixAeroK");
    CHECK(square.rowCount == 2);
    CHECK(square.columnCount == 2);
    CHECK_FALSE(pointMatrixTerms(model, PointMatrixCard::AeroJInterference, "AOAJ").has_value());
    CHECK(directMatrixNodalNames(model) == std::vector<std::string>{"AOAJ"});
}

TEST_CASE("a matrix stored as DMIK or DMIJI cards reads back from a model file; removing it removes its cards",
          "[model][matrices]")
{
    LabelledTerms terms;
    terms.form = 6;
    terms.inputType = 2;
    terms.columnId = {3, 3, 4};
    terms.columnComponent = {1, 1, 2};
    terms.rowId = {3, 4, 4};
    terms.rowComponent = {1, 2, 2};
    terms.real = {6.0, -1.5, 7.0};
    for (const PointMatrixCard kind : {PointMatrixCard::AeroJInterference, PointMatrixCard::AeroK})
    {
        Model model;
        (void)addPointMatrix(model, kind, "wkk", terms);
        CHECK(model.cardCount() == 3); // the header and a card for each of the two columns
        CHECK_THROWS_AS(addPointMatrix(model, kind, "wkk", terms), base::UsageError); // the name is taken
        const Model again = readModelText(writeModelText(model), true);
        const auto back = pointMatrixTerms(again, kind, "wkk");
        REQUIRE(back.has_value());
        CHECK(back->form == 6);
        CHECK(back->columnId == terms.columnId);
        CHECK(back->rowComponent == terms.rowComponent);
        CHECK(back->real == terms.real);
        // a DMIG of the same name does not stand in the way
        (void)addPointMatrix(model, PointMatrixCard::Nodal, "wkk", terms);
        CHECK(removePointMatrix(model, kind, "wkk") == 3);
        CHECK(pointMatrixNames(model, kind).empty());
        CHECK(model.cardCount() == 3);
    }
}

TEST_CASE("a DMIJ card is checked like a DMIG one", "[model][matrices]")
{
    const auto expectInvalid = [](const std::string& card)
    {
        INFO(card);
        CHECK_THROWS_AS(readModelText(card, true), base::DeckError);
    };
    expectInvalid("DirectMatrixAeroJ;AOAJ;0;3;2;0"); // IFO
    expectInvalid("DirectMatrixAeroJ;AOAJ;0;9;5;0"); // TIN
    oor::testing::requireModelFileRoundTrip(
        readModelText("DirectMatrixAeroJ;1AOAJ;0;9;2;0", true));    // arbitrary nonempty identifiers are preserved
    expectInvalid("DirectMatrixAeroK;WTK;1;7;1;1;2.");              // CJ above 6
    expectInvalid("DirectMatrixAeroJInterference;WJI;1;1;-4;1;2."); // a point is a positive integer
}

TEST_CASE("a DMIAX matrix keeps the harmonic number of every degree of freedom", "[model][matrices]")
{
    const Model model = readModelText(AXISYMMETRIC_MATRICES, true);
    CHECK(directMatrixAxisymNames(model) == std::vector<std::string>{"AXK4", "REALM"});
    const auto complex = directMatrixAxisymTerms(model, "AXK4");
    REQUIRE(complex.has_value());
    CHECK(complex->form == 1);
    CHECK(complex->inputType == 3);
    CHECK(complex->outputType == 4);
    CHECK(complex->complex);
    CHECK(complex->columnId == std::vector<base::EntityId>{21, 21, 22});
    CHECK(complex->columnComponent == std::vector<std::int32_t>{3, 3, 0});
    CHECK(complex->columnHarmonic == std::vector<std::int32_t>{-3, -3, BLANK});
    CHECK(complex->rowId == std::vector<base::EntityId>{614, 615, 614});
    CHECK(complex->rowHarmonic == std::vector<std::int32_t>{5, BLANK, 5});
    CHECK(complex->real == std::vector<double>{3.75e5, 1.5, 1.25});
    CHECK(complex->imag == std::vector<double>{1250.0, 0.5, 2.5});

    const CooMatrix coo = directMatrixAxisymCoo("AXK4", *complex);
    CHECK(coo.card == "DirectMatrixAxisym");
    // square: rows and columns on one list, sorted by point, component and harmonic (a blank harmonic first)
    CHECK(coo.rowCount == 4);
    CHECK(coo.columnCount == 4);
    CHECK(coo.rowId == std::vector<base::EntityId>{21, 22, 614, 615});
    CHECK(coo.rowHarmonic == std::vector<std::int32_t>{-3, BLANK, 5, BLANK});
    CHECK(coo.row == std::vector<std::int32_t>{2, 3, 2});
    CHECK(coo.column == std::vector<std::int32_t>{0, 0, 1});

    // a harmonic of zero is a harmonic: it is not the blank field
    const auto real = directMatrixAxisymTerms(model, "REALM");
    REQUIRE(real.has_value());
    CHECK_FALSE(real->complex);
    CHECK(real->columnHarmonic == std::vector<std::int32_t>{0});
    CHECK(real->rowHarmonic == std::vector<std::int32_t>{0});
    CHECK(real->imag.empty());
    CHECK(directMatrixAxisymCoo("REALM", *real).columnCount == 1);
}

TEST_CASE("a DMIAX card is written and read back, a blank harmonic staying blank", "[model][matrices]")
{
    Model model = readModelText(AXISYMMETRIC_MATRICES, true);
    const std::size_t cards = model.cardCount();
    const Model again = readModelText(writeModelText(model), true);
    CHECK(again.cardCount() == cards);
    for (const char* name : {"AXK4", "REALM"})
    {
        const auto before = directMatrixAxisymTerms(model, name);
        const auto after = directMatrixAxisymTerms(again, name);
        REQUIRE(after.has_value());
        CHECK(after->rowHarmonic == before->rowHarmonic);
        CHECK(after->columnHarmonic == before->columnHarmonic);
        CHECK(after->rowId == before->rowId);
        CHECK(after->real == before->real);
        CHECK(after->imag == before->imag);
    }
    CHECK(removeDirectMatrixAxisym(model, "AXK4") == 3);
    oor::testing::requireModelFileRoundTrip(model);
}

TEST_CASE("the values a DMIAX card refuses", "[model][matrices]")
{
    const auto problemOf = [](DirectMatrixAxisymCard card) { return DirectMatrixAxisymFormat::check(card); };
    DirectMatrixAxisymCard header;
    header.entryName = "K2PP";
    header.matrixShape = 1;
    header.inputType = 1;
    CHECK_FALSE(problemOf(header).has_value());
    DirectMatrixAxisymCard bad = header;
    bad.matrixShape = 3;
    CHECK(std::string(problemOf(bad)->field) == "matrixShape");
    bad = header;
    bad.inputType = 2; // the format reference lists single precision only
    CHECK(std::string(problemOf(bad)->field) == "inputType");
    bad = header;
    bad.outputType = 5;
    CHECK(std::string(problemOf(bad)->field) == "outputType");
    bad = header;
    bad.entryName = "9K2PP";
    CHECK_FALSE(problemOf(bad).has_value());
    Model named;
    named.add<DirectMatrixAxisymTable>(bad);
    oor::testing::requireModelFileRoundTrip(named);

    DirectMatrixAxisymCard column;
    column.entryName = "K2PP";
    column.nodeJ = 7;
    column.terms.push_back(DirectMatrixAxisymTerm{8, 1, BLANK, 2.0, 0.0});
    CHECK_FALSE(problemOf(column).has_value());
    bad = column;
    bad.componentsJ = 7;
    CHECK(std::string(problemOf(bad)->field) == "componentsJ");
    bad = column;
    bad.terms[0].component = 9;
    CHECK(std::string(problemOf(bad)->field) == "components");
    bad = column;
    bad.terms[0].imaginary = 1.0; // a real column has no B
    CHECK(std::string(problemOf(bad)->field) == "imaginaryValue");

    // a header with something after TOUT is refused by the reader
    CHECK_THROWS_AS(readModelText("DirectMatrixAxisym;K2PP;0;1;2", true), base::DeckError);
}

TEST_CASE("the points of a DMIAX matrix are renumbered with the nodes and may name points the model lacks",
          "[model][matrices]")
{
    Model model = readModelText(
        "Node;1;;0.;0.;0.\nNode;2;;1.;0.;0.\nDirectMatrixAxisym;K2PP;0;1;1\nDirectMatrixAxisym;K2PP;1;1;;2;1;;5.;;900;"
        "0;2;6.\n",
        true);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    const auto terms = directMatrixAxisymTerms(model, "K2PP");
    REQUIRE(terms.has_value());
    CHECK(terms->columnId == std::vector<base::EntityId>{101, 101});
    CHECK(terms->rowId.front() == 102);
    // the fluid point 900 is no card of the model: it is not reported as missing
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    CHECK(ReferenceChecker::check(model, sink) == 0);
}

namespace
{

const MonitorIntegratedLoad1Card& firstMonpnt1(const Model& model)
{
    static thread_local MonitorIntegratedLoad1Card card;
    card = model.findTable<MonitorIntegratedLoad1Table>()->card(0);
    return card;
}

} // namespace

TEST_CASE("a MONPNT1 label is the text of fields 3 to 9 with its blanks", "[model][monitor]")
{
    const Model model = readModelText(
        "MonitorIntegratedLoad1;FnRtTq1;Fin Root Torque About Hinge;4;FnRoot;;212.375;38.625;5.07125\n", true);
    const MonitorIntegratedLoad1Card& card = firstMonpnt1(model);
    CHECK(card.name == "FnRtTq1");
    CHECK(card.label == "Fin Root Torque About Hinge");
    CHECK(card.outputAxes == 4);
    CHECK(card.component == "FnRoot");
    CHECK(card.positionFrameId == 0);
    CHECK(card.xCoord == 212.375);
    CHECK(card.zCoord == 5.07125);
    CHECK(card.displacementFrameId == 0);

    // the fixed layout comes back as it was: the label in pieces of eight characters

    CHECK(firstMonpnt1(readModelText(writeModelText(model), true)) == card);

    // long labels remain one text value
    std::string longest(156, 'x');
    for (std::size_t index = 7; index < longest.size(); index += 9)
        longest[index] = ' ';
    Model made;
    made.add<MonitorIntegratedLoad1Table>(
        MonitorIntegratedLoad1Card{"LONGEST", longest, 123, "COMP", 0, 0.0, 0.0, 0.0, 0});
    CHECK(firstMonpnt1(readModelText(writeModelText(made), true)).label == longest);
    oor::testing::requireModelFileRoundTrip(model);
    oor::testing::requireModelFileRoundTrip(made);
}

TEST_CASE("a label is one field of the record, its blanks kept", "[model][monitor]")
{
    CHECK(firstMonpnt1(readModelText("MonitorIntegratedLoad1;A;Wing load;34;WING\n", true)).label == "Wing load");
    CHECK(firstMonpnt1(readModelText("MonitorIntegratedLoad1;A;;34;WING\n", true)).label.empty());
}

TEST_CASE("the blank CD of a monitor point is the system of CP, kept and written", "[model][monitor]")
{
    const Model model = readModelText("MonitorIntegratedLoad1;A;;34;WING;5;1.;2.;3.\n", true);
    const MonitorIntegratedLoad1Card& card = firstMonpnt1(model);
    CHECK(card.positionFrameId == 5);
    CHECK(card.displacementFrameId == 5);

    // the axes are a set of digits, kept in ascending order
    CHECK(firstMonpnt1(readModelText("MonitorIntegratedLoad1;A;;4321;WING\n", true)).outputAxes == 1234);
    oor::testing::requireModelFileRoundTrip(model);
}

TEST_CASE("the values a monitor point refuses", "[model][monitor]")
{
    const auto expectInvalid = [](const std::string& cards, const std::string& field)
    {
        INFO(cards);
        CHECK_THROWS_AS(readModelText(cards, true), base::DeckError);
        const Model tolerant = readModelText(cards, false);
        REQUIRE(tolerant.diagnostics().hasErrors());
        CHECK(tolerant.diagnostics().firstError()->location.fieldName == field);
    };
    expectInvalid("MonitorIntegratedLoad1;A;;7;WING\n", "outputAxes");
    expectInvalid("MonitorIntegratedLoad1;A;;1\n", "component");
    expectInvalid("MonitorIntegratedLoad1;A;;1;WING;-1\n", "positionFrameId");
    oor::testing::requireModelFileRoundTrip(readModelText("MonitorIntegratedLoad1;ABCDEFGHI;;1;WING\n", true));
    expectInvalid("MonitorElemLoad2;A;;TEMP;Bar2;SX;1\n", "resultTable");
    expectInvalid("MonitorElemLoad2;A;;Stress\n", "kind");
    expectInvalid("MonitorNodeForceSum3;A;;123;0\n", "nodeSetId");
    expectInvalid("MonitorNodeForceSum3;A;;123;1;2;0;1.;2.;3.;Z\n", "excludedForces");
    expectInvalid("MonitorDisplacement;A;;1;WING;0;1.;2.;3.;0;7\n", "sampledComponents");

    // Labels preserve punctuation, surrounding blanks and unrestricted length.
    MonitorIntegratedLoad1Card card{"A", "has, a comma", 1, "WING", 0, 0.0, 0.0, 0.0, 0};
    CHECK_FALSE(MonitorIntegratedLoad1Format::check(card).has_value());
    {
        Model labelled;
        labelled.add<MonitorIntegratedLoad1Table>(card);
        oor::testing::requireModelFileRoundTrip(labelled);
    }
    card.label = " leading blank";
    CHECK_FALSE(MonitorIntegratedLoad1Format::check(card).has_value());
    {
        Model labelled;
        labelled.add<MonitorIntegratedLoad1Table>(card);
        oor::testing::requireModelFileRoundTrip(labelled);
    }
    card.label = std::string(57, 'x');
    CHECK_FALSE(MonitorIntegratedLoad1Format::check(card).has_value());
    {
        Model labelled;
        labelled.add<MonitorIntegratedLoad1Table>(card);
        oor::testing::requireModelFileRoundTrip(labelled);
    }
    card.label = "$ comment";
    CHECK_FALSE(MonitorIntegratedLoad1Format::check(card).has_value());
    {
        Model labelled;
        labelled.add<MonitorIntegratedLoad1Table>(card);
        oor::testing::requireModelFileRoundTrip(labelled);
    }
}

TEST_CASE("MONPNT2, MONPNT3 and MONDSP1 read their fields", "[model][monitor]")
{
    const Model model = readModelText(
        "MonitorElemLoad2;BR42;Rear spar cap near rib 7;Stress;Bar2;SX1D;42\n"
        "MonitorNodeForceSum3;q7;Cabin floor beam cut;12345;3;4;0;45.;;;ASM\n"
        "MonitorDisplacement;TIP88;Flap tip deflection;3;FLAP88;1004;96.;88.;12.5;1006\n"
        "MonitorDisplacement;W2;LBL;123;C1\n",
        true);
    const MonitorElemLoad2Card two = model.findTable<MonitorElemLoad2Table>()->card(0);
    CHECK(two.name == "BR42");
    CHECK(two.label == "Rear spar cap near rib 7");
    CHECK(two.resultTable == "Stress");
    CHECK(two.kind == "Bar2");
    CHECK(two.resultItem == "SX1D");
    CHECK(two.elemId == 42);
    const MonitorNodeForceSum3Card three = model.findTable<MonitorNodeForceSum3Table>()->card(0);
    CHECK(three.name == "q7");
    CHECK(three.outputAxes == 12345);
    CHECK(three.nodeSetId == 3);
    CHECK(three.elementSet == 4);
    CHECK(three.xCoord == 45.0);
    CHECK(three.excluded == "ASM");
    CHECK(three.displacementFrameId == 0);
    const MonitorDisplacementTable& displacements = *model.findTable<MonitorDisplacementTable>();
    const MonitorDisplacementCard first = displacements.card(0);
    CHECK(first.positionFrameId == 1004);
    CHECK(first.displacementFrameId == 1006);
    CHECK(first.sampledComponents == 123); // the default
    CHECK(first.yCoord == 88.0);
    const MonitorDisplacementCard second = displacements.card(1);
    CHECK(second.outputAxes == 123);
    CHECK(second.displacementFrameId == 0);
    // the default INDDOF is not written, a different one is

    Model other;
    other.add<MonitorDisplacementTable>(MonitorDisplacementCard{"D", "", 1, "COMP", 0, 0.0, 0.0, 0.0, 0, 12});

    oor::testing::requireModelFileRoundTrip(model);
    oor::testing::requireModelFileRoundTrip(other);
}

TEST_CASE("a repeated MONPNT2 or MONPNT3 name is reported, a MONPNT1 one is not", "[model][monitor]")
{
    const Model two =
        readModelText("MonitorElemLoad2;A;;Stress;Bar2;SX;1\nMonitorElemLoad2;A;;Force;Bar2;FX;2\n");
    REQUIRE(two.diagnostics().hasErrors());
    CHECK(two.diagnostics().firstError()->code == "deck.duplicate-monitor-elem-load2");
    const Model three = readModelText("MonitorNodeForceSum3;A;;123;1\nMonitorNodeForceSum3;A;;123;2\n");
    REQUIRE(three.diagnostics().hasErrors());
    CHECK(three.diagnostics().firstError()->code == "deck.duplicate-monitor-node-force-sum3");
    // a structural and an aerodynamic MONPNT1 may share a NAME (told apart by their AECOMP)
    const Model one = readModelText("MonitorIntegratedLoad1;A;;1;STRUCT\nMonitorIntegratedLoad1;A;;1;AERO\n");
    CHECK_FALSE(one.diagnostics().hasErrors());
}

TEST_CASE("a monitor point refers to its systems, set and element", "[model][monitor]")
{
    Model model = readModelText(
        "RectFrame2;7;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "IdListSet1;5;1;2\n"
        "IdListSet1;6;10\n"
        "MonitorIntegratedLoad1;A;;1;WING;7\n"
        "MonitorNodeForceSum3;B;;123;5;6;7\n",
        true);
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    CHECK(ReferenceChecker::check(model, sink) == 0);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    CHECK(model.findTable<MonitorIntegratedLoad1Table>()->card(0).positionFrameId == 17);
    CHECK(model.findTable<MonitorIntegratedLoad1Table>()->card(0).displacementFrameId == 17);
    CHECK(model.findTable<MonitorNodeForceSum3Table>()->card(0).positionFrameId == 17);
    CHECK(model.findTable<MonitorNodeForceSum3Table>()->card(0).displacementFrameId == 17);
    Model missing = readModelText("MonitorNodeForceSum3;B;;123;5\n", false);
    sink = base::DiagnosticSink(base::ReadMode::Tolerant);
    CHECK(ReferenceChecker::check(missing, sink) == 1); // the SET1 of GRIDSET does not exist
    // a MONPNT2 may name an element the model lacks: the format reference ignores a fictitious one
    Model fictitious = readModelText("MonitorElemLoad2;A;;Stress;Bar2;SX;1\n", true);
    sink = base::DiagnosticSink(base::ReadMode::Tolerant);
    CHECK(ReferenceChecker::check(fictitious, sink) == 0);
}

TEST_CASE("a text value remains whole regardless of its internal slot span", "[model][monitor]")
{
    FieldWriter writer;
    writer.text(1, "NAME");
    writer.spanText(2, 7, "Fin Root Torque About Hinge");
    writer.integer(9, 34);
    CHECK(writer.fieldCount() == 9);
    const std::vector<std::string> fields = writer.fields();
    CHECK(fields[1] == "Fin Root Torque About Hinge");
    CHECK(fields[2].empty());
    CHECK(fields[8] == "34");
    FieldWriter blank;
    blank.spanText(2, 7, "");
    CHECK(blank.fields().size() == 8);
    CHECK(blank.fields()[1].empty());
}
