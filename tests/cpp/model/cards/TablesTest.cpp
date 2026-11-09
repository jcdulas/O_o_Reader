// Checks of the table cards: how the points and the coefficients are read and written, the ID
// spaces, and the evaluation of every table type against values worked out by hand: inside the table, at the points,
// at a jump and outside it.

#include "oor/model/cards/tables/tableDynamic1/TableDynamic1Table.hpp"
#include "oor/model/cards/tables/tableDynamic2/TableDynamic2Table.hpp"
#include "oor/model/cards/tables/tableDynamic3/TableDynamic3Table.hpp"
#include "oor/model/cards/tables/tableDynamic4/TableDynamic4Table.hpp"
#include "oor/model/cards/tables/tableMaterial1/TableMaterial1Table.hpp"
#include "oor/model/cards/tables/tableMaterial2/TableMaterial2Table.hpp"
#include "oor/model/cards/tables/tableMaterial3/TableMaterial3Table.hpp"
#include "oor/model/cards/tables/tableMaterial4/TableMaterial4Table.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using Catch::Approx;
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

// The value of the first TableDynamic1 of a model at x, by the three ways to ask for it.
double tableDynamic1At(const std::string& card, double x)
{
    const Model model = readBulk(card);
    const TableDynamic1Table& table = *model.findTable<TableDynamic1Table>();
    const double byRow = table.evaluateRow(0, x);
    const double byCard = TableDynamic1Table::evaluate(table.card(0), x);
    const std::vector<double> in = {x};
    std::vector<double> out(1);
    table.evaluateRow(0, in, out);
    if (std::isnan(byRow))
    {
        REQUIRE(std::isnan(byCard));
        REQUIRE(std::isnan(out[0]));
    }
    else
    {
        REQUIRE(byCard == byRow);
        REQUIRE(out[0] == byRow);
    }
    return byRow;
}

// y(x) = 2x on [0, 1], x + 1 on [1, 3], 4 at 3 and 5 at 4 (slope 1): a table with two different end segments.
const char* const LINEAR = "TableDynamic1;1;;;0.;0.;1.;2.;3.;4.;EndTable";

} // namespace

TEST_CASE("the points of a table are read up to ENDT and SKIP pairs are dropped", "[model][cards]")
{
    const Model model = readBulk(
        "TableDynamic1;1;Logarithmic;;1.;1.;Skip;Skip;10.;10.;;;100.;100.;EndTable\n"
        "TableDynamic1;2;;;0.;0.;1.;Skip;2.;4.;EndTable\n"
        "TableDynamic1;3;;;0.;0.;1.;1.;2.;2.;3.;3.;4.;4.;5.;5.;6.;6.;EndTable");
    const TableDynamic1Card log = cardAt<TableDynamic1Table>(model, 0);
    REQUIRE(log.xAxisKind == TableAxis::Log);
    REQUIRE(log.yAxisKind == TableAxis::Linear);
    REQUIRE(log.x == std::vector<double>{1.0, 10.0, 100.0});
    REQUIRE(log.y == std::vector<double>{1.0, 10.0, 100.0});
    REQUIRE(cardAt<TableDynamic1Table>(model, 1).x == std::vector<double>{0.0, 2.0}); // SKIP in the y field too
    REQUIRE(cardAt<TableDynamic1Table>(model, 2).x.size() == 7);                      // over two continuation lines
}

TEST_CASE("a table writes its points two fields each and ends with ENDT in the next field", "[model][cards]")
{
    const Model model = readBulk(
        "TableDynamic1;1;;;0.;0.;1.;2.;2.;1.;3.;5.;4.;6.;EndTable\n"
        "TableDynamic4;2;0.;1.;0.;10.;1.;2.;3.;4.;5.;6.;7.;8.;EndTable");

    // The points start at field 9: five blank fields after TID, XAXIS, YAXIS; ENDT follows the last pair.

    // ENDT alone on the line that follows a full line.

    const Model again = readModelText(writeModelText(model), true);
    REQUIRE(cardAt<TableDynamic1Table>(again).x == cardAt<TableDynamic1Table>(model).x);
    REQUIRE(cardAt<TableDynamic4Table>(again).coefficients == cardAt<TableDynamic4Table>(model).coefficients);
    oor::testing::requireModelFileRoundTrip(model);
}

TEST_CASE("a table needs ENDT, nothing after it, and points the evaluation can use", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;1;;;0.;0.;1.;1."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;1;;;0.;0.;1.;1.;EndTable;5.;6."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;1;;;0.;EndTable"), base::DeckError);                 // no point
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;1;;;0.;0.;1.;;EndTable"), base::DeckError);          // a blank y
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;1;;;0.;0.;1.;1.;0.5;2.;EndTable"), base::DeckError); // up, then down
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;1;;;1.;0.;1.;1.;2.;2.;EndTable"), base::DeckError); // a jump at the start
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;1;;;0.;0.;1.;1.;2.;2.;2.;3.;EndTable"),
                      base::DeckError); // and at the end
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;1;;Logarithmic;0.;1.;1.;0.;EndTable"),
                      base::DeckError);                                                      // y = 0 on a LOG axis
    REQUIRE_NOTHROW(readBulk("TableDynamic1;1;;;0.;0.;EndTable"));                           // one point
    REQUIRE_NOTHROW(readBulk("TableDynamic1;1;;;0.;0.;1.;1.;1.;2.;2.;3.;EndTable"));         // a jump inside is fine
    REQUIRE_THROWS_AS(readBulk("TableDynamic4;1;0.;0.;0.;1.;1.;EndTable"), base::DeckError); // X2 = 0
    REQUIRE_THROWS_AS(readBulk("TableDynamic4;1;0.;1.;0.;1.;1.;2."), base::DeckError);       // no ENDT
    REQUIRE_THROWS_AS(readBulk("TableDynamic4;1;0.;1.;0.;1.;1.;EndTable;2."), base::DeckError);
}

TEST_CASE("the tables of one kind share an ID space, and the material tables have one of their own", "[model][cards]")
{
    const std::string points = ";0.;0.;1.;1.;EndTable\n";
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;5;;" + points + "TableDynamic2;5;0." + points), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TableDynamic3;5;0.;1." + points + "TableDynamic4;5;0.;1.;0.;1.;1.;EndTable"),
                      base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TableMaterial1;5;;" + points + "TableMaterial3;5;0.;1." + points), base::DeckError);
    // A dynamic table and a material table may have the same ID.
    REQUIRE_NOTHROW(readBulk("TableDynamic1;5;;" + points + "TableMaterial1;5;;" + points));
    // A material table may have a negative ID (a thermal strain table), a dynamic table may not.
    const Model model = readBulk("TableMaterial1;-5;;" + points);
    REQUIRE(cardAt<TableMaterial1Table>(model).id == -5);
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;-5;;" + points), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TableMaterial1;0;;" + points), base::DeckError);
}

TEST_CASE("TABLED1 interpolates linearly, extends its end segments and averages a jump", "[model][cards]")
{
    // Inside the table and at the points.
    REQUIRE(tableDynamic1At(LINEAR, 0.5) == Approx(1.0));
    REQUIRE(tableDynamic1At(LINEAR, 2.0) == Approx(3.0));
    REQUIRE(tableDynamic1At(LINEAR, 0.0) == 0.0);
    REQUIRE(tableDynamic1At(LINEAR, 1.0) == 2.0);
    REQUIRE(tableDynamic1At(LINEAR, 3.0) == 4.0);
    // Outside: the first segment has slope 2, the last one slope 1.
    REQUIRE(tableDynamic1At(LINEAR, -1.0) == Approx(-2.0));
    REQUIRE(tableDynamic1At(LINEAR, 4.0) == Approx(5.0));
    // A jump at x = 1 from 2 to 6: the mean there, the segments on both sides unchanged.
    const char* const jump = "TableDynamic1;1;;;0.;0.;1.;2.;1.;6.;3.;6.;EndTable";
    REQUIRE(tableDynamic1At(jump, 1.0) == Approx(4.0));
    REQUIRE(tableDynamic1At(jump, 0.5) == Approx(1.0));
    REQUIRE(tableDynamic1At(jump, 2.0) == Approx(6.0));
    // A single point is a constant, NaN stays NaN.
    REQUIRE(tableDynamic1At("TableDynamic1;1;;;2.;7.;EndTable", 100.0) == 7.0);
    REQUIRE(std::isnan(tableDynamic1At(LINEAR, std::nan(""))));
}

TEST_CASE("TABLED1 gives the same values whether x ascends or descends", "[model][cards]")
{
    const char* const descending = "TableDynamic1;1;;;3.;4.;1.;2.;0.;0.;EndTable";
    for (const double x : {-1.0, 0.0, 0.5, 1.0, 2.0, 3.0, 4.0})
        REQUIRE(tableDynamic1At(descending, x) == Approx(tableDynamic1At(LINEAR, x)));
    const char* const descendingJump = "TableDynamic1;1;;;3.;6.;1.;6.;1.;2.;0.;0.;EndTable";
    REQUIRE(tableDynamic1At(descendingJump, 1.0) == Approx(4.0));
    REQUIRE(tableDynamic1At(descendingJump, 0.5) == Approx(1.0));
}

TEST_CASE("TABLED1 interpolates in the logarithm of the axes that are LOG", "[model][cards]")
{
    // LOG x, LINEAR y: y = 4 * ln(x) / ln(100) between (1, 0) and (100, 4).
    const char* const logX = "TableDynamic1;1;Logarithmic;;1.;0.;100.;4.;EndTable";
    REQUIRE(tableDynamic1At(logX, 10.0) == Approx(2.0));
    REQUIRE(tableDynamic1At(logX, 1000.0) == Approx(6.0)); // the segment carries on
    REQUIRE(tableDynamic1At(logX, 0.1) == Approx(-2.0));
    REQUIRE(tableDynamic1At(logX, 1.0) == 0.0);
    REQUIRE(std::isnan(tableDynamic1At(logX, 0.0)));
    REQUIRE(std::isnan(tableDynamic1At(logX, -3.0)));
    // LINEAR x, LOG y: y = 100^(x / 2) between (0, 1) and (2, 100).
    const char* const logY = "TableDynamic1;1;;Logarithmic;0.;1.;2.;100.;EndTable";
    REQUIRE(tableDynamic1At(logY, 1.0) == Approx(10.0));
    REQUIRE(tableDynamic1At(logY, 3.0) == Approx(1000.0));
    REQUIRE(tableDynamic1At(logY, -1.0) == Approx(0.1));
    // LOG x, LOG y: y = x^2 between (1, 1) and (100, 10000).
    const char* const logLog = "TableDynamic1;1;Logarithmic;Logarithmic;1.;1.;100.;1.E+4;EndTable";
    REQUIRE(tableDynamic1At(logLog, 10.0) == Approx(100.0));
    REQUIRE(tableDynamic1At(logLog, 1000.0) == Approx(1.0e6));
    // A jump with LOG y gives the geometric mean of the two values.
    const char* const logJump = "TableDynamic1;1;;Logarithmic;0.;1.;1.;2.;1.;8.;2.;9.;EndTable";
    REQUIRE(tableDynamic1At(logJump, 1.0) == Approx(4.0));
}

TEST_CASE("TABLED2 looks the points up at x - X1", "[model][cards]")
{
    const Model model = readBulk("TableDynamic2;1;1.;0.;0.;2.;4.;3.;4.;EndTable");
    const TableDynamic2Table& table = *model.findTable<TableDynamic2Table>();
    REQUIRE(table.evaluateRow(0, 2.0) == Approx(2.0));  // yT(1)
    REQUIRE(table.evaluateRow(0, 1.0) == 0.0);          // yT(0)
    REQUIRE(table.evaluateRow(0, 0.0) == Approx(-2.0)); // yT(-1): the first segment extended
    REQUIRE(table.evaluateRow(0, 3.0) == 4.0);          // yT(2)
    REQUIRE(table.evaluateRow(0, 5.0) == Approx(4.0));  // yT(4): the last segment is flat
    REQUIRE(TableDynamic2Table::evaluate(table.card(0), 2.0) == Approx(2.0));
    const std::vector<double> in = {0.0, 1.0, 2.0, 3.0};
    std::vector<double> out(4);
    table.evaluateRow(0, in, out);
    const std::vector<double> expected = {-2.0, 0.0, 2.0, 4.0};
    for (std::size_t index = 0; index < expected.size(); ++index)
        REQUIRE(out[index] == Approx(expected[index]));
}

TEST_CASE("TABLED3 looks the points up at (x - X1) / X2", "[model][cards]")
{
    const Model model = readBulk("TableDynamic3;1;1.;2.;0.;0.;2.;4.;EndTable");
    const TableDynamic3Table& table = *model.findTable<TableDynamic3Table>();
    REQUIRE(table.evaluateRow(0, 3.0) == Approx(2.0));   // yT(1)
    REQUIRE(table.evaluateRow(0, 1.0) == 0.0);           // yT(0)
    REQUIRE(table.evaluateRow(0, 5.0) == 4.0);           // yT(2)
    REQUIRE(table.evaluateRow(0, 7.0) == Approx(6.0));   // yT(3): extended
    REQUIRE(table.evaluateRow(0, -1.0) == Approx(-2.0)); // yT(-1)
    REQUIRE(TableDynamic3Table::evaluate(table.card(0), 7.0) == Approx(6.0));
    // A negative X2 turns the table round.
    const Model reversed = readBulk("TableDynamic3;1;1.;-2.;0.;0.;2.;4.;EndTable");
    REQUIRE(reversed.findTable<TableDynamic3Table>()->evaluateRow(0, -1.0) == Approx(2.0)); // yT(1)
    REQUIRE(reversed.findTable<TableDynamic3Table>()->evaluateRow(0, -3.0) == Approx(4.0)); // yT(2)
}

TEST_CASE("TABLED4 is the polynomial in (x - X1) / X2 with x limited to [X3, X4]", "[model][cards]")
{
    // y = 1 + 2 t + 3 t^2 with t = (x - 1) / 2, x limited to [0, 5].
    const Model model = readBulk("TableDynamic4;1;1.;2.;0.;5.;1.;2.;3.;EndTable");
    const TableDynamic4Table& table = *model.findTable<TableDynamic4Table>();
    REQUIRE(table.evaluateRow(0, 3.0) == Approx(6.0));    // t = 1
    REQUIRE(table.evaluateRow(0, 1.0) == Approx(1.0));    // t = 0
    REQUIRE(table.evaluateRow(0, 0.0) == Approx(0.75));   // t = -0.5
    REQUIRE(table.evaluateRow(0, -10.0) == Approx(0.75)); // x = 0 below X3
    REQUIRE(table.evaluateRow(0, 5.0) == Approx(17.0));   // t = 2
    REQUIRE(table.evaluateRow(0, 100.0) == Approx(17.0)); // x = 5 above X4
    REQUIRE(std::isnan(table.evaluateRow(0, std::nan(""))));
    REQUIRE(TableDynamic4Table::evaluate(table.card(0), 3.0) == Approx(6.0));
    const std::vector<double> in = {-10.0, 1.0, 3.0, 100.0};
    std::vector<double> out(4);
    table.evaluateRow(0, in, out);
    const std::vector<double> expected = {0.75, 1.0, 6.0, 17.0};
    for (std::size_t index = 0; index < expected.size(); ++index)
        REQUIRE(out[index] == Approx(expected[index]));
    // One coefficient is a constant.
    const Model constant = readBulk("TableDynamic4;2;0.;1.;0.;1.;4.5;EndTable");
    REQUIRE(constant.findTable<TableDynamic4Table>()->evaluateRow(0, 0.5) == 4.5);
}

TEST_CASE("the material tables evaluate like the dynamic ones, without the factor of the MAT entry", "[model][cards]")
{
    const Model model = readBulk(
        "TableMaterial1;1;;;0.;0.;1.;2.;3.;4.;EndTable\n"
        "TableMaterial2;2;1.;0.;0.;2.;4.;3.;4.;EndTable\n"
        "TableMaterial3;3;1.;2.;0.;0.;2.;4.;EndTable\n"
        "TableMaterial4;4;1.;2.;0.;5.;1.;2.;3.;EndTable\n"
        "TableMaterial1;-5;Logarithmic;Logarithmic;1.;1.;100.;1.E+4;EndTable");
    REQUIRE(model.findTable<TableMaterial1Table>()->evaluateRow(0, 0.5) == Approx(1.0));
    REQUIRE(model.findTable<TableMaterial1Table>()->evaluateRow(0, -1.0) == Approx(-2.0));
    REQUIRE(model.findTable<TableMaterial1Table>()->evaluateRow(0, 4.0) == Approx(5.0));
    REQUIRE(model.findTable<TableMaterial1Table>()->evaluateRow(1, 10.0) == Approx(100.0)); // LOG axes, negative ID
    REQUIRE(TableMaterial1Table::evaluate(cardAt<TableMaterial1Table>(model, 1), 1000.0) == Approx(1.0e6));
    REQUIRE(model.findTable<TableMaterial2Table>()->evaluateRow(0, 2.0) == Approx(2.0));
    REQUIRE(model.findTable<TableMaterial2Table>()->evaluateRow(0, 0.0) == Approx(-2.0));
    REQUIRE(TableMaterial2Table::evaluate(cardAt<TableMaterial2Table>(model), 3.0) == 4.0);
    REQUIRE(model.findTable<TableMaterial3Table>()->evaluateRow(0, 3.0) == Approx(2.0));
    REQUIRE(model.findTable<TableMaterial3Table>()->evaluateRow(0, 7.0) == Approx(6.0));
    REQUIRE(TableMaterial3Table::evaluate(cardAt<TableMaterial3Table>(model), 5.0) == 4.0);
    REQUIRE(model.findTable<TableMaterial4Table>()->evaluateRow(0, 3.0) == Approx(6.0));
    REQUIRE(model.findTable<TableMaterial4Table>()->evaluateRow(0, 100.0) == Approx(17.0));
    REQUIRE(model.findTable<TableMaterial4Table>()->evaluateRow(0, -10.0) == Approx(0.75));
    REQUIRE(TableMaterial4Table::evaluate(cardAt<TableMaterial4Table>(model), 3.0) == Approx(6.0));
    const std::vector<double> in = {-1.0, 0.5, 4.0};
    std::vector<double> out(3);
    model.findTable<TableMaterial1Table>()->evaluateRow(0, in, out);
    const std::vector<double> expected = {-2.0, 1.0, 5.0};
    for (std::size_t index = 0; index < expected.size(); ++index)
        REQUIRE(out[index] == Approx(expected[index]));
}

TEST_CASE("a table row of a model with several tables is evaluated with its own points", "[model][cards]")
{
    const Model model = readBulk(
        "TableDynamic1;1;;;0.;0.;1.;1.;EndTable\n"
        "TableDynamic1;2;;;0.;0.;1.;10.;EndTable\n"
        "TableDynamic1;3;;;0.;0.;1.;100.;2.;100.;EndTable");
    const TableDynamic1Table& table = *model.findTable<TableDynamic1Table>();
    REQUIRE(table.evaluateRow(0, 0.5) == Approx(0.5));
    REQUIRE(table.evaluateRow(1, 0.5) == Approx(5.0));
    REQUIRE(table.evaluateRow(2, 1.5) == Approx(100.0));
}
