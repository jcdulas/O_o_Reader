// The edits of a query result: the changes made to a column, the editable table with its read-only columns,
// the matching of an update by key, and the plan of an apply (cards found by key, positions turned back into the CP
// system).

#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/apply/ColumnEdits.hpp"
#include "oor/query/apply/EditableTable.hpp"
#include "oor/query/apply/QueryApply.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cmath>
#include <limits>

using namespace oor;
namespace q = oor::query;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;
using oor::testing::readModelText;

namespace
{

constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();

q::QueryColumn reals(std::vector<double> values)
{
    return q::QueryColumn::primitive(std::move(values));
}

q::QueryColumn ints32(std::vector<std::int32_t> values)
{
    return q::QueryColumn::primitive(std::move(values));
}

std::vector<double> realValues(const q::QueryColumn& column)
{
    const auto values = column.as<double>();
    return {values.begin(), values.end()};
}

std::vector<std::int32_t> intValues(const q::QueryColumn& column)
{
    const auto values = column.as<std::int32_t>();
    return {values.begin(), values.end()};
}

// A table of cards: `id` (the key), `pid` and `t` stored, `area` computed, `label` derived; the rows are cards of the
// card types 5 and 6 with the keys 100 + row.
std::shared_ptr<q::QueryTable> cardTable(std::size_t count)
{
    auto table = std::make_shared<q::QueryTable>();
    std::vector<std::int32_t> ids(count);
    std::vector<std::int32_t> pids(count);
    std::vector<double> thickness(count);
    std::vector<double> areas(count);
    auto types = std::make_shared<std::vector<std::uint16_t>>(count);
    auto keys = std::make_shared<std::vector<std::uint32_t>>(count);
    for (std::size_t row = 0; row < count; ++row)
    {
        ids[row] = static_cast<std::int32_t>(10 * (row + 1));
        pids[row] = static_cast<std::int32_t>(1 + row % 3);
        thickness[row] = 0.5 * static_cast<double>(row + 1);
        areas[row] = 2.0 * static_cast<double>(row + 1);
        (*types)[row] = row % 2 == 0 ? 5 : 6;
        (*keys)[row] = static_cast<std::uint32_t>(100 + row);
    }
    table->addColumn("id", ints32(ids), q::ColumnOrigin::Stored, "id");
    table->addColumn("pid", ints32(pids), q::ColumnOrigin::Stored, "pid");
    table->addColumn("t", reals(thickness), q::ColumnOrigin::Stored, "t");
    table->addColumn("area", reals(areas), q::ColumnOrigin::Computed, "area");
    std::vector<std::string> labels(count, "A");
    table->addColumn("label", q::QueryColumn::utf8(labels), q::ColumnOrigin::Derived);
    table->addColumn("note", q::QueryColumn::utf8(labels), q::ColumnOrigin::Stored, "note");
    q::RowOrigin origin;
    origin.types = types;
    origin.keys = keys;
    table->setOrigin(origin);
    return table;
}

// What the owner of the field setters says: card type 6 has no setter for `t`.
class FakePolicy final : public q::EditPolicy
{
public:
    [[nodiscard]] std::string fieldProblem(std::uint16_t cardType, const std::string& field,
                                           std::uint32_t) const override
    {
        if (cardType == 6 && field == "t")
            return "no field setter: type 6 cannot write 't'";
        return {};
    }
};

} // namespace

TEST_CASE("the changes to a column read back, with the old value kept in the column", "[query][edit]")
{
    const q::QueryColumn base = reals({1.0, 2.0, NAN_VALUE, 4.0});
    q::ColumnEdits edits(base);
    CHECK(edits.kind() == q::ColumnEdits::Kind::Real);
    CHECK(edits.changedCount() == 0);
    edits.set(1, q::CellValue::ofReal(20.0));
    edits.set(2, q::CellValue::ofInteger(3)); // an integer is a number
    edits.set(3, q::CellValue{});             // a null
    CHECK(edits.changedCount() == 3);
    CHECK(edits.value(0, base).asReal() == 1.0);
    CHECK(edits.value(1, base).asReal() == 20.0);
    CHECK(edits.value(2, base).asReal() == 3.0);
    CHECK(edits.value(3, base).isNull());
    CHECK(edits.changedRows() == std::vector<std::uint32_t>{1, 2, 3});

    const q::QueryColumn made = edits.materialize(base);
    const auto values = realValues(made);
    CHECK(values[0] == 1.0);
    CHECK(values[1] == 20.0);
    CHECK(values[2] == 3.0);
    CHECK(std::isnan(values[3]));
    CHECK(made.isNull(3));
    CHECK_FALSE(made.isNull(2));       // the NaN that was there is a value now
    CHECK(realValues(base)[1] == 2.0); // the column it started from is not touched

    edits.unset(1);
    CHECK(edits.changedCount() == 2);
    CHECK(edits.value(1, base).asReal() == 2.0);
    edits.clear();
    CHECK(edits.changedCount() == 0);
    CHECK(realValues(edits.materialize(base))[3] == 4.0);
}

TEST_CASE("an integer column checks the range and refuses fractions, a bulk set is all or nothing", "[query][edit]")
{
    const q::QueryColumn base = ints32({1, 2, 3, 4});
    q::ColumnEdits edits(base);
    CHECK(edits.kind() == q::ColumnEdits::Kind::Integer);
    CHECK_THROWS_AS(edits.set(0, q::CellValue::ofReal(2.5)), q::QueryTypeError);
    CHECK_THROWS_AS(edits.set(0, q::CellValue::ofInteger(3000000000)), q::QueryTypeError);
    CHECK_THROWS_AS(edits.set(0, q::CellValue::ofText("a")), q::QueryTypeError);
    CHECK_THROWS_AS(edits.set(9, q::CellValue::ofInteger(1)), q::QueryError);
    edits.set(0, q::CellValue::ofReal(7.0)); // a whole number is an integer
    CHECK(edits.value(0, base).asInteger() == 7);

    const std::vector<std::uint32_t> rows = {1, 2};
    const std::vector<double> bad = {5.0, 5.5};
    CHECK_THROWS_AS(edits.setReals(rows, bad), q::QueryTypeError);
    CHECK(edits.changedCount() == 1); // nothing of the failed call stayed
    const std::vector<std::int64_t> good = {50, 60};
    edits.setIntegers(rows, good);
    CHECK(intValues(edits.materialize(base)) == std::vector<std::int32_t>{7, 50, 60, 4});
    const std::vector<std::uint32_t> outside = {1, 17};
    CHECK_THROWS_AS(edits.setIntegers(outside, std::vector<std::int64_t>{1}), q::QueryError);
    CHECK(edits.value(1, base).asInteger() == 50);
    // one value for every row
    edits.setIntegers(std::vector<std::uint32_t>{0, 1, 2, 3}, std::vector<std::int64_t>{9});
    CHECK(intValues(edits.materialize(base)) == std::vector<std::int32_t>{9, 9, 9, 9});
}

TEST_CASE("text, fixed lists and lists of any length can be edited", "[query][edit]")
{
    const std::vector<std::string> texts = {"a", "bb", "ccc"};
    const q::QueryColumn text = q::QueryColumn::utf8(texts);
    q::ColumnEdits textEdits(text);
    textEdits.set(1, q::CellValue::ofText("changed"));
    textEdits.set(2, q::CellValue{});
    const q::QueryColumn madeText = textEdits.materialize(text);
    CHECK(madeText.textAt(0) == "a");
    CHECK(madeText.textAt(1) == "changed");
    CHECK(madeText.isNull(2));
    CHECK_THROWS_AS(textEdits.set(0, q::CellValue::ofReal(1.0)), q::QueryTypeError);

    // a fixed list of three reals (a position)
    const q::QueryColumn positions = q::QueryColumn::fixedList(reals({0, 0, 0, 1, 1, 1, 2, 2, 2}), 3, 3);
    q::ColumnEdits fixed(positions);
    CHECK(fixed.kind() == q::ColumnEdits::Kind::RealList);
    CHECK(fixed.width() == 3);
    CHECK_THROWS_AS(fixed.set(0, q::CellValue::ofReals({1.0, 2.0})), q::QueryTypeError);
    fixed.set(1, q::CellValue::ofReals({5.0, 6.0, 7.0}));
    fixed.set(2, q::CellValue::ofIntegers({1, 2, 3})); // integers are numbers
    const q::QueryColumn madeFixed = fixed.materialize(positions);
    CHECK(realValues(*madeFixed.child()) == std::vector<double>{0, 0, 0, 5, 6, 7, 1, 2, 3});
    CHECK(fixed.value(1, positions).asReals() == std::vector<double>{5.0, 6.0, 7.0});

    // a list of any length (the nodes of a rigid element)
    const q::QueryColumn lists = q::QueryColumn::list(q::ColumnBuffer::adopt(std::vector<std::int64_t>{0, 2, 3, 6}),
                                                      ints32({1, 2, 3, 4, 5, 6}), 3);
    q::ColumnEdits variable(lists);
    CHECK(variable.kind() == q::ColumnEdits::Kind::IntegerList);
    variable.set(0, q::CellValue::ofIntegers({7, 8, 9, 10}));
    variable.set(1, q::CellValue{});
    const q::QueryColumn madeList = variable.materialize(lists);
    CHECK(madeList.listLength(0) == 4);
    CHECK(madeList.listLength(1) == 0);
    CHECK(madeList.isNull(1));
    CHECK(madeList.listLength(2) == 3);
    CHECK(intValues(*madeList.child()) == std::vector<std::int32_t>{7, 8, 9, 10, 4, 5, 6});
}

TEST_CASE("only the stored columns of the cards can be edited, and a read-only column says why at once",
          "[query][edit]")
{
    const auto table = cardTable(6);
    q::EditableTable edit(table, std::make_shared<FakePolicy>());
    CHECK(edit.editability("pid").editable);
    CHECK(edit.editability("t").editable); // card type 5 can take it
    CHECK(edit.editability("t").rejectedTypes == std::vector<std::uint16_t>{6});
    CHECK_FALSE(edit.editability("area").editable);
    CHECK_THAT(edit.editability("area").reason, ContainsSubstring("computed"));
    CHECK_FALSE(edit.editability("label").editable);

    CHECK_THROWS_AS(edit.setCell(edit.columnIndex("area"), 0, q::CellValue::ofReal(1.0)), q::ReadOnlyColumnError);
    try
    {
        edit.setCell(edit.columnIndex("area"), 0, q::CellValue::ofReal(1.0));
    }
    catch (const q::ReadOnlyColumnError& error)
    {
        CHECK_THAT(std::string(error.what()), ContainsSubstring("'area'"));
        CHECK_THAT(std::string(error.what()), ContainsSubstring("computed"));
    }
    CHECK_THROWS_AS(edit.setCell(edit.columnIndex("label"), 0, q::CellValue::ofText("B")), q::ReadOnlyColumnError);

    // a row whose card type has no setter for the column
    edit.setCell(edit.columnIndex("t"), 0, q::CellValue::ofReal(9.0)); // row 0 is type 5
    CHECK_THROWS_AS(edit.setCell(edit.columnIndex("t"), 1, q::CellValue::ofReal(9.0)), q::ReadOnlyColumnError);
    const std::vector<std::uint32_t> both = {2, 3};
    CHECK_THROWS_AS(edit.setBroadcast(edit.columnIndex("t"), both, q::CellValue::ofReal(1.0)), q::ReadOnlyColumnError);
    CHECK(edit.changeCount() == 1); // the refused call changed nothing, not even the row that could have taken it

    // a table that is not made of cards has nothing to write to
    auto loose = std::make_shared<q::QueryTable>();
    loose->addColumn("a", reals({1.0, 2.0}));
    q::EditableTable looseEdit(loose);
    CHECK_FALSE(looseEdit.editability("a").editable);
    CHECK_THROWS_AS(looseEdit.setCell(0, 0, q::CellValue::ofReal(1.0)), q::ReadOnlyColumnError);
    CHECK_THROWS_AS(edit.columnIndex("nothing"), q::QueryError);
}

TEST_CASE("the table reads the new values and keeps the old ones, the original table is not touched", "[query][edit]")
{
    const auto table = cardTable(6);
    q::EditableTable edit(table);
    const std::size_t pid = edit.columnIndex("pid");
    const std::size_t thickness = edit.columnIndex("t");
    edit.setBroadcast(pid, std::vector<std::uint32_t>{0, 2, 4}, q::CellValue::ofInteger(9));
    edit.setReals(thickness, std::vector<std::uint32_t>{1, 3}, std::vector<double>{0.25, 0.75});
    CHECK(edit.changeCount() == 5);
    CHECK(edit.value(pid, 2).asInteger() == 9);
    CHECK(edit.originalValue(pid, 2).asInteger() == 3);
    CHECK(intValues(edit.currentColumn(pid)) == std::vector<std::int32_t>{9, 2, 9, 1, 9, 3});
    CHECK(intValues(table->column("pid")) == std::vector<std::int32_t>{1, 2, 3, 1, 2, 3});
    const auto current = edit.current();
    CHECK(intValues(current->column("pid")) == std::vector<std::int32_t>{9, 2, 9, 1, 9, 3});
    CHECK(current->column("id").values().data() == table->column("id").values().data()); // untouched columns are shared
    CHECK(current->origin().isPresent());

    const auto changes = edit.changes();
    REQUIRE(changes.size() == 5);
    CHECK(changes[0].row == 0);
    CHECK(changes[0].oldValue.asInteger() == 1);
    CHECK(changes[0].newValue.asInteger() == 9);

    edit.reset(pid, std::vector<std::uint32_t>{0});
    CHECK(edit.changeCount() == 4);
    edit.reset(pid);
    CHECK(edit.changeCount() == 2);
    edit.reset();
    CHECK_FALSE(edit.hasChanges());
    CHECK(edit.current() == table);

    // the change is written to the model: the table with the change becomes the one the next changes are relative to
    edit.setCell(pid, 0, q::CellValue::ofInteger(8));
    edit.commit();
    CHECK_FALSE(edit.hasChanges());
    CHECK(edit.originalValue(pid, 0).asInteger() == 8);
}

TEST_CASE("an update matches rows by key and records only the cells that differ", "[query][edit][update]")
{
    const auto table = cardTable(6);
    q::EditableTable edit(table);
    auto data = std::make_shared<q::QueryTable>();
    data->addColumn("id", ints32({30, 10, 50}));
    data->addColumn("pid", ints32({3, 7, 2})); // 30 -> row 2 (pid 3: same), 10 -> row 0 (1 -> 7), 50 -> row 4 (2: same)
    data->addColumn("t", reals({1.5, 0.5, 9.0}));    // row 2: same, row 0: same, row 4: 2.5 -> 9
    data->addColumn("area", reals({0.0, 0.0, 0.0})); // read-only: left out
    data->addColumn("nothing", reals({0.0, 0.0, 0.0}));
    q::UpdateOptions options;
    options.on = "id";
    const q::UpdateResult result = edit.update(*data, options);
    CHECK(result.matchedKeys == 3);
    CHECK(result.cellsChanged == 2);
    CHECK(edit.changeCount() == 2);
    CHECK(edit.value(edit.columnIndex("pid"), 0).asInteger() == 7);
    CHECK(edit.value(edit.columnIndex("t"), 4).asReal() == 9.0);
    CHECK(result.columnsIgnored.count("area") == 1);
    CHECK(result.columnsIgnored.count("nothing") == 1);
    CHECK(result.columnsUpdated == std::vector<std::string>{"pid", "t"});

    // a key that matches no row
    auto missing = std::make_shared<q::QueryTable>();
    missing->addColumn("id", ints32({10, 11}));
    missing->addColumn("pid", ints32({4, 4}));
    CHECK_THROWS_AS(edit.update(*missing, options), q::QueryError);
    CHECK(edit.changeCount() == 2); // nothing of the failed update stayed
    options.ignoreMissing = true;
    const q::UpdateResult ignored = edit.update(*missing, options);
    CHECK(ignored.missingKeys == std::vector<std::int64_t>{11});
    CHECK(edit.value(edit.columnIndex("pid"), 0).asInteger() == 4);

    // naming a read-only column is an error, a bad value is an error and writes nothing
    q::UpdateOptions named;
    named.on = "id";
    named.columns = {"area"};
    CHECK_THROWS_AS(edit.update(*data, named), q::ReadOnlyColumnError);
    auto wrong = std::make_shared<q::QueryTable>();
    wrong->addColumn("id", ints32({20, 40}));
    wrong->addColumn("pid", reals({5.0, 5.5}));
    options.ignoreMissing = false;
    const std::size_t before = edit.changeCount();
    CHECK_THROWS_AS(edit.update(*wrong, options), q::QueryTypeError);
    CHECK(edit.changeCount() == before);

    // the same key twice in the data is ambiguous
    auto twice = std::make_shared<q::QueryTable>();
    twice->addColumn("id", ints32({10, 10}));
    twice->addColumn("pid", ints32({1, 2}));
    CHECK_THROWS_AS(edit.update(*twice, options), q::QueryError);

    // by card key
    auto byKey = std::make_shared<q::QueryTable>();
    byKey->addColumn("cardKey", ints32({101}));
    byKey->addColumn("pid", ints32({6}));
    q::UpdateOptions cardKeys;
    const q::UpdateResult viaKey = edit.update(*byKey, cardKeys);
    CHECK(viaKey.cellsChanged == 1);
    CHECK(edit.value(edit.columnIndex("pid"), 1).asInteger() == 6);
}

TEST_CASE("an update with no key column says what it could have used", "[query][edit][update]")
{
    const auto table = cardTable(3);
    q::EditableTable edit(table);
    auto data = std::make_shared<q::QueryTable>();
    data->addColumn("pid", ints32({1, 2, 3}));
    CHECK_THROWS_AS(edit.update(*data), q::QueryError);
}

namespace
{

// Grid points in the basic system, in a cylindrical system (cid 2: z along the global z, origin at 1, 0, 0) and in a
// spherical one (cid 3).
std::shared_ptr<model::Model> gridModel()
{
    return std::make_shared<model::Model>(
        readModelText("RectFrame2;1;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
                      "CylindricalFrame2;2;1;1.;0.;0.;1.;0.;1.;2.;0.;0.\n"
                      "SphericalFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
                      "Node;1;;1.;2.;3.\n"
                      "Node;2;2;2.;30.;4.\n"
                      "Node;3;3;5.;60.;20.\n"
                      "Node;4;;7.;8.;9.\n"));
}

// The table of the grid points of a model as a query would make it: the CP coordinates stored, the basic position
// computed.
std::shared_ptr<q::QueryTable> nodeTable(const std::shared_ptr<model::Model>& model)
{
    const model::NodeTable& grids = model->table<model::NodeTable>();
    const analysis::BasicPositionCache& cache = analysis::BasicPositionCache::of(*model);
    const std::size_t count = grids.rows().rowCount();
    std::vector<std::int32_t> ids(count);
    std::vector<double> x(count);
    std::vector<double> y(count);
    std::vector<double> z(count);
    std::vector<double> global(3 * count);
    std::vector<double> local(3 * count);
    auto types = std::make_shared<std::vector<std::uint16_t>>(count, model::typeIndexOf<model::NodeTable>());
    auto keys = std::make_shared<std::vector<std::uint32_t>>(count);
    for (std::uint32_t row = 0; row < count; ++row)
    {
        ids[row] = grids.idAt(row);
        const base::Vec3 position = cache.position(row);
        x[row] = global[3 * row] = position.x;
        y[row] = global[3 * row + 1] = position.y;
        z[row] = global[3 * row + 2] = position.z;
        const base::Vec3 own = grids.xyz(row);
        local[3 * row] = own.x;
        local[3 * row + 1] = own.y;
        local[3 * row + 2] = own.z;
        (*keys)[row] = grids.rows().keyAt(row).value;
    }
    auto table = std::make_shared<q::QueryTable>();
    table->addColumn("id", ints32(ids), q::ColumnOrigin::Stored, "id");
    table->addColumn("xyz", q::QueryColumn::fixedList(reals(local), 3, static_cast<std::int64_t>(count)),
                     q::ColumnOrigin::Stored, "xyz");
    table->addColumn("x", reals(x), q::ColumnOrigin::Computed, "x");
    table->addColumn("y", reals(y), q::ColumnOrigin::Computed, "y");
    table->addColumn("z", reals(z), q::ColumnOrigin::Computed, "z");
    table->addColumn("xyzGlobal", q::QueryColumn::fixedList(reals(global), 3, static_cast<std::int64_t>(count)),
                     q::ColumnOrigin::Computed, "xyzGlobal");
    q::RowOrigin origin;
    origin.types = types;
    origin.keys = keys;
    table->setOrigin(origin);
    return table;
}

const q::FieldWrite* fieldOf(const q::CardWrite& card, const std::string& name)
{
    for (const q::FieldWrite& write : card.fields)
    {
        if (write.field == name)
            return &write;
    }
    return nullptr;
}

} // namespace

TEST_CASE("the basic position columns are editable for grid points and follow one another", "[query][edit][position]")
{
    const auto model = gridModel();
    const auto table = nodeTable(model);
    q::EditableTable edit(table);
    for (const char* name : {"x", "y", "z", "xyzGlobal"})
    {
        CHECK(edit.editability(name).editable);
        CHECK(edit.editability(name).position);
    }
    CHECK(edit.editability("xyz").editable);
    CHECK_FALSE(edit.editability("xyz").position);

    const std::size_t x = edit.columnIndex("x");
    const std::size_t global = edit.columnIndex("xyzGlobal");
    edit.setCell(x, 3, q::CellValue::ofReal(70.0));
    // the table reads one position whichever column it asks
    CHECK(edit.value(global, 3).asReals()[0] == 70.0);
    CHECK(edit.value(global, 3).asReals()[1] == Approx(8.0));
    edit.setCell(global, 3, q::CellValue::ofReals({1.0, 2.0, 3.0}));
    CHECK(edit.value(x, 3).asReal() == 1.0);
    CHECK(edit.value(edit.columnIndex("y"), 3).asReal() == 2.0);
    CHECK(edit.value(edit.columnIndex("z"), 3).asReal() == 3.0);
    edit.reset(x, std::vector<std::uint32_t>{3}); // resetting a position resets all of its columns
    CHECK_FALSE(edit.hasChanges());
}

TEST_CASE("a position is editable only when every row is a grid point", "[query][edit][position]")
{
    const auto model = gridModel();
    auto table = std::make_shared<q::QueryTable>(*nodeTable(model));
    // the same columns on a table of another kind of card: the card types of the rows are not the grid type
    q::RowOrigin origin = table->origin();
    const auto other = static_cast<std::uint16_t>(model::typeIndexOf<model::NodeTable>() + 1);
    origin.types = std::make_shared<std::vector<std::uint16_t>>(static_cast<std::size_t>(table->rowCount()), other);
    table->setOrigin(origin);
    q::EditableTable edit(table);
    CHECK_FALSE(edit.editability("x").editable);
    CHECK_THAT(edit.editability("x").reason, ContainsSubstring("GRID"));
    CHECK_THROWS_AS(edit.setCell(edit.columnIndex("x"), 0, q::CellValue::ofReal(1.0)), q::ReadOnlyColumnError);
}

TEST_CASE("a plan turns a basic position back into the CP system of the node", "[query][edit][apply][position]")
{
    const auto model = gridModel();
    const auto table = nodeTable(model);
    q::EditableTable edit(table);
    const analysis::BasicPositionCache& cache = analysis::BasicPositionCache::of(*model);

    // node 2 is in the cylindrical system, node 3 in the spherical one: move each to a new basic position
    edit.setCell(edit.columnIndex("z"), 1, q::CellValue::ofReal(-1.5));
    edit.setCell(edit.columnIndex("xyzGlobal"), 2, q::CellValue::ofReals({0.25, 0.5, 4.0}));
    edit.setCell(edit.columnIndex("x"), 3, q::CellValue::ofReal(-2.0));
    const q::ApplyPlan plan = q::QueryApply::plan(edit, *model);
    REQUIRE(plan.cards.size() == 3);
    CHECK(plan.positions == 3);
    CHECK(plan.cells == edit.changeCount());

    const model::NodeTable& grids = model->table<model::NodeTable>();
    for (const q::CardWrite& card : plan.cards)
    {
        const q::FieldWrite* xyz = fieldOf(card, "xyz");
        REQUIRE(xyz != nullptr);
        const auto& local = xyz->value.asReals();
        const base::EntityId cp = grids.positionFrameId(card.row);
        analysis::CoordinateTransform transform(*model);
        const base::Vec3 basic = transform.toBasic(cp, base::Vec3{local[0], local[1], local[2]});
        const std::size_t tableRow = card.id == 2 ? 1 : card.id == 3 ? 2 : 3;
        const q::CellValue wanted = edit.value(edit.columnIndex("xyzGlobal"), tableRow);
        // written back in the node's own system, read as the position that was asked for
        if (card.id == 2) // only z was edited: x and y stay what the model has
        {
            CHECK(basic.x == Approx(cache.position(card.row).x).margin(1e-9));
            CHECK(basic.y == Approx(cache.position(card.row).y).margin(1e-9));
        }
        CHECK(basic.x == Approx(wanted.asReals()[0]).margin(1e-9));
        CHECK(basic.y == Approx(wanted.asReals()[1]).margin(1e-9));
        CHECK(basic.z == Approx(wanted.asReals()[2]).margin(1e-9));
    }
}

TEST_CASE("an edit made on the model after the table is kept for the axes that were not changed",
          "[query][edit][apply][position]")
{
    const auto model = gridModel();
    const auto table = nodeTable(model);
    q::EditableTable edit(table);
    // grid 4 is in the basic system: the table was made with (7, 8, 9); the model moves y to 80 meanwhile
    model->table<model::NodeTable>().setXyz(3, base::Vec3{7.0, 80.0, 9.0});
    edit.setCell(edit.columnIndex("x"), 3, q::CellValue::ofReal(-7.0));
    const q::ApplyPlan plan = q::QueryApply::plan(edit, *model);
    REQUIRE(plan.cards.size() == 1);
    const auto& xyz = fieldOf(plan.cards[0], "xyz")->value.asReals();
    CHECK(xyz[0] == Approx(-7.0));
    CHECK(xyz[1] == Approx(80.0)); // the model's value, not the one of the table
    CHECK(xyz[2] == Approx(9.0));
}

TEST_CASE("a plan lists the cards that are gone, by card key, and writes nothing", "[query][edit][apply]")
{
    const auto model = gridModel();
    const auto table = nodeTable(model);
    q::EditableTable edit(table);
    edit.setCell(edit.columnIndex("xyz"), 0, q::CellValue::ofReals({5.0, 5.0, 5.0}));
    edit.setCell(edit.columnIndex("xyz"), 2, q::CellValue::ofReals({6.0, 6.0, 6.0}));
    edit.setCell(edit.columnIndex("xyz"), 3, q::CellValue::ofReals({7.0, 7.0, 7.0}));
    // grids 1 and 3 are deleted
    for (const base::EntityId id : {1, 3})
        model->remove(*model->locate(base::IdSpace::Node, id));
    try
    {
        (void)q::QueryApply::plan(edit, *model);
        FAIL("the plan should have failed");
    }
    catch (const q::ApplyError& error)
    {
        CHECK(error.isStale());
        REQUIRE(error.problems().size() == 2);
        CHECK(error.problems()[0].card == "Node");
        CHECK(error.problems()[0].key == (*table->origin().keys)[0]);
        CHECK(error.problems()[1].key == (*table->origin().keys)[2]);
    }
}

TEST_CASE("a plan finds the cards by key, not by ID, after a renumbering", "[query][edit][apply]")
{
    const auto model = gridModel();
    const auto table = nodeTable(model);
    q::EditableTable edit(table);
    edit.setCell(edit.columnIndex("xyz"), 3, q::CellValue::ofReals({5.0, 5.0, 5.0}));
    // grid 4 gets the ID 400 and the rows are compacted after a deletion
    model->setId(*model->locate(base::IdSpace::Node, 4), 400);
    model->remove(*model->locate(base::IdSpace::Node, 1));
    model->compact();
    const q::ApplyPlan plan = q::QueryApply::plan(edit, *model);
    REQUIRE(plan.cards.size() == 1);
    CHECK(plan.cards[0].id == 400);
    CHECK(model->table<model::NodeTable>().idAt(plan.cards[0].row) == 400);
}

TEST_CASE("rows that are one card must give one value", "[query][edit][apply]")
{
    const auto model = gridModel();
    auto repeated = std::make_shared<q::QueryTable>(*nodeTable(model));
    // the same card in two rows (a join repeats the row of a card)
    q::RowOrigin origin = repeated->origin();
    auto keys = std::make_shared<std::vector<std::uint32_t>>(*origin.keys);
    (*keys)[1] = (*keys)[0];
    origin.keys = keys;
    repeated->setOrigin(origin);
    q::EditableTable edit(repeated);
    const std::size_t xyz = edit.columnIndex("xyz");
    edit.setCell(xyz, 0, q::CellValue::ofReals({1.0, 1.0, 1.0}));
    edit.setCell(xyz, 1, q::CellValue::ofReals({1.0, 1.0, 1.0}));
    CHECK(q::QueryApply::plan(edit, *model).cards.size() == 1); // the same value twice is one write
    edit.setCell(xyz, 1, q::CellValue::ofReals({2.0, 2.0, 2.0}));
    CHECK_THROWS_AS(q::QueryApply::plan(edit, *model), q::ApplyError);
}

TEST_CASE("a position and the CP coordinates of the same node cannot both be edited", "[query][edit][apply]")
{
    const auto model = gridModel();
    const auto table = nodeTable(model);
    q::EditableTable edit(table);
    edit.setCell(edit.columnIndex("x"), 3, q::CellValue::ofReal(1.0));
    edit.setCell(edit.columnIndex("xyz"), 3, q::CellValue::ofReals({5.0, 5.0, 5.0}));
    CHECK_THROWS_AS(q::QueryApply::plan(edit, *model), q::ApplyError);
}

TEST_CASE("a plan of a table with no change is empty", "[query][edit][apply]")
{
    const auto model = gridModel();
    q::EditableTable edit(nodeTable(model));
    const q::ApplyPlan plan = q::QueryApply::plan(edit, *model);
    CHECK(plan.cards.empty());
    CHECK(plan.cells == 0);
}
