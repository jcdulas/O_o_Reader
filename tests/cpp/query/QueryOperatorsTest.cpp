// The operators of the query engine over a table of arrays: every operator is compared with the same computation
// written as a loop over the rows, on tables of random values, and every result is compared between one thread and
// twelve.

#include "QueryTestSupport.hpp"
#include "oor/query/QueryErrors.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <map>
#include <numeric>
#include <set>

namespace q = oor::query;
using q::testing::makeRandomTable;
using q::testing::RandomTable;
using q::testing::runOn;
using q::testing::sameTable;

namespace
{

q::Query queryOn(const RandomTable& data)
{
    return q::Query(std::make_shared<q::TableSource>(data.table));
}

std::vector<std::int32_t> idsOf(const q::QueryTable& table)
{
    const auto values = table.column("id").as<std::int32_t>();
    return {values.begin(), values.end()};
}

// The ids of the rows where `keep(row)` holds, in the order of the table.
template <typename Predicate>
std::vector<std::int32_t> idsWhere(const RandomTable& data, Predicate keep)
{
    std::vector<std::int32_t> ids;
    for (std::size_t row = 0; row < data.id.size(); ++row)
    {
        if (keep(row))
            ids.push_back(data.id[row]);
    }
    return ids;
}

} // namespace

TEST_CASE("where keeps the rows an expression selects, as a loop over the rows does", "[query][where]")
{
    const RandomTable data = makeRandomTable(5000, 7);
    const auto run = [&](const std::string& text)
    { return idsOf(*runOn(queryOn(data).where(q::Expression::parse(text)), 1)); };

    CHECK(run("x > 0.25") == idsWhere(data, [&](std::size_t row) { return data.x[row] > 0.25; }));
    CHECK(run("x <= 0.0 and y > 5") ==
          idsWhere(data, [&](std::size_t row) { return data.x[row] <= 0.0 && data.y[row] > 5; }));
    CHECK(run("group in (1, 2, 3) or y < 1") ==
          idsWhere(data,
                   [&](std::size_t row) { return (data.group[row] >= 1 && data.group[row] <= 3) || data.y[row] < 1; }));
    CHECK(run("group not in (0, 9) and flag") ==
          idsWhere(data, [&](std::size_t row)
                   { return data.group[row] != 0 && data.group[row] != 9 && data.flag[row] != 0; }));
    CHECK(run("not (name == 'A' or name == 'B')") ==
          idsWhere(data, [&](std::size_t row) { return data.name[row] != "A" && data.name[row] != "B"; }));
    CHECK(run("name in ('B', 'D') and key >= 3000") ==
          idsWhere(data, [&](std::size_t row)
                   { return (data.name[row] == "B" || data.name[row] == "D") && data.key[row] >= 3000; }));
    CHECK(run("x * 2 + y / 4 > 3") ==
          idsWhere(data, [&](std::size_t row) { return data.x[row] * 2 + data.y[row] / 4 > 3; }));
    CHECK(run("(group + 1) % 3 == 0") ==
          idsWhere(data, [&](std::size_t row) { return (data.group[row] + 1) % 3 == 0; }));
    CHECK(run("abs(x) < 0.1") == idsWhere(data, [&](std::size_t row) { return std::fabs(data.x[row]) < 0.1; }));
    CHECK(run("sqrt(y) > 2") == idsWhere(data, [&](std::size_t row) { return std::sqrt(data.y[row]) > 2; }));
    CHECK(run("y ** 2 > 50") == idsWhere(data, [&](std::size_t row) { return std::pow(data.y[row], 2) > 50; }));
    CHECK(run("isnan(x)") == idsWhere(data, [&](std::size_t row) { return std::isnan(data.x[row]); }));
    CHECK(run("x != x") == idsWhere(data, [&](std::size_t row) { return std::isnan(data.x[row]); }));
    CHECK(run("true") == idsWhere(data, [&](std::size_t) { return true; }));
}

TEST_CASE("a null makes a comparison null, and and/or follow the three-valued logic", "[query][where]")
{
    const RandomTable data = makeRandomTable(3000, 11);
    const auto run = [&](const std::string& text)
    { return idsOf(*runOn(queryOn(data).where(q::Expression::parse(text)), 1)); };
    const auto valid = [&](std::size_t row) { return data.maybeValid[row] != 0; };

    CHECK(run("maybe > 500") == idsWhere(data, [&](std::size_t row) { return valid(row) && data.maybe[row] > 500; }));
    CHECK(run("maybe is null") == idsWhere(data, [&](std::size_t row) { return !valid(row); }));
    CHECK(run("maybe is not null and group == 1") ==
          idsWhere(data, [&](std::size_t row) { return valid(row) && data.group[row] == 1; }));
    // false and null is false, so `not (...)` of it is true; true or null is true
    CHECK(run("not (maybe > 500 and group == 99)") == idsWhere(data, [&](std::size_t) { return true; }));
    CHECK(run("maybe > 500 or group >= 0") == idsWhere(data, [&](std::size_t) { return true; }));
    // not of a null stays null: the row is not kept
    CHECK(run("not (maybe > 500)") ==
          idsWhere(data, [&](std::size_t row) { return valid(row) && data.maybe[row] <= 500; }));
    CHECK(run("maybe in (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)") ==
          idsWhere(data, [&](std::size_t row) { return valid(row) && data.maybe[row] >= 1 && data.maybe[row] <= 10; }));
}

TEST_CASE("a condition on stored columns runs before one that needs a computed column", "[query][where]")
{
    // A source whose `area` column counts the rows it is asked for: the cheap condition leaves fewer rows to compute.
    class CountingSource final : public q::QuerySource
    {
    public:
        explicit CountingSource(std::shared_ptr<const q::QueryTable> table) : m_inner(std::move(table)) {}
        [[nodiscard]] std::string description() const override { return "counting"; }
        [[nodiscard]] std::size_t rowCount() const override { return m_inner.rowCount(); }
        [[nodiscard]] std::vector<q::ColumnDescription> columns() const override
        {
            std::vector<q::ColumnDescription> all = m_inner.columns();
            all.push_back(q::ColumnDescription{"area", q::ColumnOrigin::Computed, "area", false});
            return all;
        }
        [[nodiscard]] std::vector<q::NamedColumn> fetch(std::string_view name, const q::RowSelection& rows,
                                                        q::ExecutionContext& context) const override
        {
            if (name != "area")
                return m_inner.fetch(name, rows, context);
            computedRows = rows.count;
            std::vector<double> values(rows.count);
            for (std::size_t index = 0; index < rows.count; ++index)
                values[index] = static_cast<double>(rows.at(index));
            std::vector<q::NamedColumn> result;
            result.push_back(q::NamedColumn{"area", q::QueryColumn::primitive(std::move(values))});
            return result;
        }
        mutable std::size_t computedRows = 0;

    private:
        q::TableSource m_inner;
    };
    const RandomTable data = makeRandomTable(1000, 3);
    auto source = std::make_shared<CountingSource>(data.table);
    q::Query query(source);
    const auto table = runOn(query.where(q::Expression::parse("area >= 100 and group == 4")), 1);
    const std::size_t expected = static_cast<std::size_t>(std::count(data.group.begin(), data.group.end(), 4));
    CHECK(source->computedRows == expected);
    CHECK(table->report().computedColumns == std::vector<std::string>{"area"});
    // The column is not computed when nothing uses it.
    source->computedRows = 0;
    const auto plain = runOn(query.where(q::Expression::parse("group == 4")), 1);
    CHECK(source->computedRows == 0);
    CHECK(plain->report().computedColumns.empty());
    CHECK_FALSE(plain->has("area"));
}

TEST_CASE("sort orders the rows by several keys, stably, with nulls and NaN last", "[query][sort]")
{
    const RandomTable data = makeRandomTable(4000, 21);
    const auto sorted = runOn(queryOn(data).sortBy({{"group", true}, {"x", false}}), 1);
    std::vector<std::size_t> order(data.id.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t a, std::size_t b)
                     {
                         if (data.group[a] != data.group[b])
                             return data.group[a] > data.group[b];
                         const bool aNan = std::isnan(data.x[a]);
                         const bool bNan = std::isnan(data.x[b]);
                         if (aNan || bNan)
                             return !aNan && bNan;
                         return data.x[a] < data.x[b];
                     });
    std::vector<std::int32_t> expected;
    for (const std::size_t row : order)
        expected.push_back(data.id[row]);
    CHECK(idsOf(*sorted) == expected);

    // a column with nulls, descending: the nulls stay last
    const auto byMaybe = runOn(queryOn(data).sortBy({{"maybe", true}}), 1);
    const auto& column = byMaybe->column("maybe");
    bool seenNull = false;
    std::int32_t previous = std::numeric_limits<std::int32_t>::max();
    for (std::size_t row = 0; row < static_cast<std::size_t>(column.length()); ++row)
    {
        if (column.isNull(row))
            seenNull = true;
        else
        {
            CHECK_FALSE(seenNull);
            CHECK(column.as<std::int32_t>()[row] <= previous);
            previous = column.as<std::int32_t>()[row];
        }
    }
    CHECK(seenNull);

    // text keys
    const auto byName = runOn(queryOn(data).sortBy({{"name", false}, {"id", false}}), 1);
    std::vector<std::pair<std::string, std::int32_t>> pairs;
    for (std::size_t row = 0; row < data.id.size(); ++row)
        pairs.emplace_back(data.name[row], data.id[row]);
    std::sort(pairs.begin(), pairs.end());
    std::vector<std::int32_t> expectedByName;
    for (const auto& pair : pairs)
        expectedByName.push_back(pair.second);
    CHECK(idsOf(*byName) == expectedByName);
}

TEST_CASE("limit keeps the first rows", "[query][limit]")
{
    const RandomTable data = makeRandomTable(1000, 5);
    const auto table = runOn(queryOn(data).sortBy({{"id", false}}).limit(10), 1);
    CHECK(idsOf(*table) == std::vector<std::int32_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    CHECK(runOn(queryOn(data).limit(5000), 1)->rowCount() == 1000);
    CHECK(runOn(queryOn(data).limit(0), 1)->rowCount() == 0);
}

TEST_CASE("groupBy computes count, sum, min, max and mean in each group", "[query][group]")
{
    const RandomTable data = makeRandomTable(6000, 33);
    const auto table = runOn(queryOn(data).groupBy({"group"}, {{"n", q::AggregateFunction::Count, "", ""},
                                                               {"nx", q::AggregateFunction::Count, "x", ""},
                                                               {"sumY", q::AggregateFunction::Sum, "y", ""},
                                                               {"sumX", q::AggregateFunction::Sum, "x", ""},
                                                               {"minX", q::AggregateFunction::Min, "x", ""},
                                                               {"maxId", q::AggregateFunction::Max, "id", ""},
                                                               {"meanY", q::AggregateFunction::Mean, "y", ""},
                                                               {"flags", q::AggregateFunction::Sum, "flag", ""},
                                                               {"meanMaybe", q::AggregateFunction::Mean, "maybe", ""}}),
                             1);
    REQUIRE(table->rowCount() == 10);
    for (std::int32_t group = 0; group < 10; ++group)
    {
        const auto row = static_cast<std::size_t>(group);
        CHECK(table->column("group").as<std::int32_t>()[row] == group);
        std::int64_t count = 0;
        std::int64_t countX = 0;
        std::int64_t flags = 0;
        double sumY = 0.0;
        double sumX = 0.0;
        double minX = std::numeric_limits<double>::infinity();
        std::int32_t maxId = 0;
        double sumMaybe = 0.0;
        std::int64_t countMaybe = 0;
        for (std::size_t other = 0; other < data.id.size(); ++other)
        {
            if (data.group[other] != group)
                continue;
            ++count;
            sumY += data.y[other];
            maxId = std::max(maxId, data.id[other]);
            flags += data.flag[other];
            if (!std::isnan(data.x[other]))
            {
                ++countX;
                sumX += data.x[other];
                minX = std::min(minX, data.x[other]);
            }
            if (data.maybeValid[other] != 0)
            {
                sumMaybe += data.maybe[other];
                ++countMaybe;
            }
        }
        CHECK(table->column("n").as<std::int64_t>()[row] == count);
        CHECK(table->column("nx").as<std::int64_t>()[row] == countX);
        CHECK(table->column("sumY").as<double>()[row] == Catch::Approx(sumY).epsilon(1e-12));
        CHECK(table->column("sumX").as<double>()[row] == Catch::Approx(sumX).epsilon(1e-9).margin(1e-9));
        CHECK(table->column("minX").as<double>()[row] == minX);
        CHECK(table->column("maxId").as<std::int32_t>()[row] == maxId);
        CHECK(table->column("meanY").as<double>()[row] ==
              Catch::Approx(sumY / static_cast<double>(count)).epsilon(1e-12));
        CHECK(table->column("flags").as<std::int64_t>()[row] == flags);
        CHECK(table->column("meanMaybe").as<double>()[row] ==
              Catch::Approx(sumMaybe / static_cast<double>(countMaybe)).epsilon(1e-12));
    }
}

TEST_CASE("groupBy with no key gives one group, also of no row", "[query][group]")
{
    const RandomTable data = makeRandomTable(100, 2);
    const auto total = runOn(queryOn(data).groupBy({}, {{"n", q::AggregateFunction::Count, "", ""},
                                                        {"top", q::AggregateFunction::Max, "y", ""},
                                                        {"all", q::AggregateFunction::Sum, "y", ""}}),
                             1);
    REQUIRE(total->rowCount() == 1);
    CHECK(total->column("n").as<std::int64_t>()[0] == 100);
    CHECK(total->column("top").as<double>()[0] == *std::max_element(data.y.begin(), data.y.end()));

    const auto none = runOn(queryOn(data)
                                .where(q::Expression::parse("y > 100"))
                                .groupBy({}, {{"n", q::AggregateFunction::Count, "", ""},
                                              {"top", q::AggregateFunction::Max, "y", ""},
                                              {"all", q::AggregateFunction::Sum, "y", ""}}),
                            1);
    REQUIRE(none->rowCount() == 1);
    CHECK(none->column("n").as<std::int64_t>()[0] == 0);
    CHECK(none->column("top").isNull(0));
    CHECK(none->column("all").as<double>()[0] == 0.0);

    const auto nothing = runOn(queryOn(data)
                                   .where(q::Expression::parse("y > 100"))
                                   .groupBy({"group"}, {{"n", q::AggregateFunction::Count, "", ""}}),
                               1);
    CHECK(nothing->rowCount() == 0);
}

TEST_CASE("groups of several keys and of text keys come out in increasing order of the keys", "[query][group]")
{
    const RandomTable data = makeRandomTable(3000, 17);
    const auto table = runOn(queryOn(data).groupBy({"name", "flag"}, {{"n", q::AggregateFunction::Count, "", ""}}), 1);
    std::map<std::pair<std::string, int>, std::int64_t> expected;
    for (std::size_t row = 0; row < data.id.size(); ++row)
        ++expected[{data.name[row], data.flag[row]}];
    REQUIRE(static_cast<std::size_t>(table->rowCount()) == expected.size());
    std::size_t row = 0;
    for (const auto& [key, count] : expected)
    {
        CHECK(table->column("name").textAt(row) == key.first);
        CHECK(table->column("flag").boolAt(row) == (key.second != 0));
        CHECK(table->column("n").as<std::int64_t>()[row] == count);
        ++row;
    }
    // a key with nulls: the nulls are one group, last
    const auto byMaybe = runOn(queryOn(data).groupBy({"maybe"}, {{"n", q::AggregateFunction::Count, "", ""}}), 1);
    const auto last = static_cast<std::size_t>(byMaybe->rowCount()) - 1;
    CHECK(byMaybe->column("maybe").isNull(last));
    const std::int64_t nulls = std::count(data.maybeValid.begin(), data.maybeValid.end(), std::uint8_t{0});
    CHECK(byMaybe->column("n").as<std::int64_t>()[last] == nulls);
}

TEST_CASE("select picks, renames and computes columns", "[query][select]")
{
    const RandomTable data = makeRandomTable(500, 9);
    q::SelectItem plain;
    plain.name = "id";
    q::SelectItem renamed;
    renamed.name = "g";
    renamed.from = "group";
    q::SelectItem computed;
    computed.name = "ratio";
    computed.expression = q::Expression::parse("y / (group + 1)");
    q::SelectItem big;
    big.name = "big";
    big.expression = q::Expression::parse("y > 5 and flag");
    const auto table = runOn(queryOn(data).select({plain, renamed, computed, big}), 1);
    REQUIRE(table->columnCount() == 4);
    CHECK(table->columnNames() == std::vector<std::string>{"id", "g", "ratio", "big"});
    for (std::size_t row = 0; row < data.id.size(); ++row)
    {
        CHECK(table->column("g").as<std::int32_t>()[row] == data.group[row]);
        CHECK(table->column("ratio").as<double>()[row] == data.y[row] / (data.group[row] + 1));
        CHECK(table->column("big").boolAt(row) == (data.y[row] > 5 && data.flag[row] != 0));
    }
}

TEST_CASE("a join to a table matches the keys, left or inner", "[query][join]")
{
    const RandomTable data = makeRandomTable(2000, 4);
    auto right = std::make_shared<q::QueryTable>();
    std::vector<std::int32_t> keys;
    std::vector<double> weights;
    for (std::int32_t group = 0; group < 10; group += 2) // the even groups only
    {
        keys.push_back(group);
        weights.push_back(group * 1.5);
    }
    right->addColumn("group", q::QueryColumn::primitive(std::vector<std::int32_t>(keys)));
    right->addColumn("weight", q::QueryColumn::primitive(std::vector<double>(weights)));
    q::JoinStep left;
    left.on = {"group"};
    left.table = right;
    left.tableKey = "group";
    left.how = q::JoinKind::Left;
    const auto leftJoin = runOn(queryOn(data).join(left), 1);
    REQUIRE(leftJoin->rowCount() == 2000);
    CHECK(leftJoin->has("weight"));
    CHECK(leftJoin->has("joinedGroup")); // `group` is taken: the name of the target goes in front
    for (std::size_t row = 0; row < data.id.size(); ++row)
    {
        const bool found = data.group[row] % 2 == 0;
        CHECK(leftJoin->column("weight").isNull(row) == !found);
        if (found)
            CHECK(leftJoin->column("weight").as<double>()[row] == data.group[row] * 1.5);
    }
    q::JoinStep inner = left;
    inner.how = q::JoinKind::Inner;
    const auto innerJoin = runOn(queryOn(data).join(inner), 1);
    const std::size_t even = static_cast<std::size_t>(
        std::count_if(data.group.begin(), data.group.end(), [](int group) { return group % 2 == 0; }));
    CHECK(static_cast<std::size_t>(innerJoin->rowCount()) == even);
    CHECK_FALSE(innerJoin->column("weight").hasNulls());

    // a key that is not unique
    auto repeated = std::make_shared<q::QueryTable>();
    repeated->addColumn("group", q::QueryColumn::primitive(std::vector<std::int32_t>{1, 1}));
    q::JoinStep bad = left;
    bad.table = repeated;
    CHECK_THROWS_AS(runOn(queryOn(data).join(bad), 1), q::QueryError);
}

TEST_CASE("a query gives the same table on one thread and on twelve", "[query][threads]")
{
    const RandomTable data = makeRandomTable(100000, 99);
    q::SelectItem computed;
    computed.name = "ratio";
    computed.expression = q::Expression::parse("y / (group + 1) + x");
    q::SelectItem id;
    id.name = "id";
    q::SelectItem x;
    x.name = "x";
    const std::vector<q::Query> queries = {
        queryOn(data).where(q::Expression::parse("x > 0 and group in (2, 3, 5) or name == 'B'")),
        queryOn(data).sortBy({{"x", true}, {"id", false}}),
        queryOn(data).sortBy({{"name", false}, {"key", true}}),
        queryOn(data).groupBy({"group", "name"}, {{"n", q::AggregateFunction::Count, "", ""},
                                                  {"sy", q::AggregateFunction::Sum, "y", ""},
                                                  {"sx", q::AggregateFunction::Sum, "x", ""},
                                                  {"mean", q::AggregateFunction::Mean, "y", ""}}),
        queryOn(data).groupBy({},
                              {{"sy", q::AggregateFunction::Sum, "y", ""}, {"sx", q::AggregateFunction::Sum, "x", ""}}),
        queryOn(data)
            .select({id, computed})
            .where(q::Expression::parse("ratio > 0.5"))
            .sortBy({{"ratio", true}})
            .limit(1000),
    };
    for (const q::Query& query : queries)
    {
        const auto one = runOn(query, 1);
        const auto twelve = runOn(query, 12);
        CHECK(sameTable(*one, *twelve));
    }
}

TEST_CASE("errors name the column or the expression at fault", "[query][errors]")
{
    const RandomTable data = makeRandomTable(10, 1);
    const auto run = [&](const q::Query& query) { return runOn(query, 1); };
    CHECK_THROWS_AS(run(queryOn(data).where(q::Expression::parse("nothing > 1"))), q::QueryError);
    CHECK_THROWS_AS(run(queryOn(data).sortBy({{"nothing", false}})), q::QueryError);
    CHECK_THROWS_AS(run(queryOn(data).groupBy({"nothing"}, {})), q::QueryError);
    CHECK_THROWS_AS(run(queryOn(data).groupBy({"group"}, {{"s", q::AggregateFunction::Sum, "nothing", ""}})),
                    q::QueryError);
    CHECK_THROWS_AS(run(queryOn(data).groupBy({"group"}, {{"s", q::AggregateFunction::Sum, "name", ""}})),
                    q::QueryTypeError);
    CHECK_THROWS_AS(run(queryOn(data).groupBy({"group"}, {{"s", q::AggregateFunction::Min, "flag", ""}})),
                    q::QueryTypeError);
    CHECK_THROWS_AS(run(queryOn(data).where(q::Expression::parse("name > 3"))), q::QueryTypeError);
    CHECK_THROWS_AS(run(queryOn(data).where(q::Expression::parse("name + 1 > 3"))), q::QueryTypeError);
    CHECK_THROWS_AS(run(queryOn(data).where(q::Expression::parse("x and y"))), q::QueryTypeError);
    CHECK_THROWS_AS(run(queryOn(data).where(q::Expression::parse("x + 1"))), q::QueryTypeError); // not a condition
    CHECK_THROWS_AS(run(queryOn(data).where(q::Expression::parse("group in ('A', 'B')"))), q::QueryTypeError);
    CHECK_THROWS_AS(
        run(queryOn(data).limit(1).join(q::JoinStep{"property", {}, q::JoinKind::Left, std::nullopt, {}, nullptr, {}})),
        q::QueryError);

    for (const char* text : {"x >", "x > > 1", "(x > 1", "x > 1)", "x ! 1", "'abc", "x in 1", "x in (1, y)",
                             "1 < x < 2", "foo(x) > 1", "x is 3", "", "and", "x > 1 and", "3 4"})
        CHECK_THROWS_AS(q::Expression::parse(text), q::QueryError);

    try
    {
        (void)q::Expression::parse("x > ");
        FAIL("the expression must not parse");
    }
    catch (const q::QueryError& error)
    {
        CHECK(std::string(error.what()).find("position") != std::string::npos);
    }
    try
    {
        (void)run(queryOn(data).where(q::Expression::parse("area > 1")));
        FAIL("the column does not exist");
    }
    catch (const q::QueryError& error)
    {
        CHECK(std::string(error.what()).find("area") != std::string::npos);
        CHECK(std::string(error.what()).find("columns:") != std::string::npos);
    }
}

TEST_CASE("the expression language reads numbers, texts, precedence and spellings", "[query][expression]")
{
    CHECK(q::Expression::parse("1 + 2 * 3 ** 2").toString() == "(1 + (2 * (3 ** 2)))");
    CHECK(q::Expression::parse("-2 ** 2").toString() == "-(2 ** 2)");
    CHECK(q::Expression::parse("2 ** -1").toString() == "(2 ** -1)");
    CHECK(q::Expression::parse("a == 1 AND NOT b <> 2 or c").toString() == "(((a == 1) and not (b != 2)) or c)");
    CHECK(q::Expression::parse("a = 1").toString() == "(a == 1)");
    CHECK(q::Expression::parse("x in [1, 2.5, -3]").toString() == "(x in (1, 2.5, -3))");
    CHECK(q::Expression::parse("t not in ('a', \"b\")").toString() == "(t not in ('a', 'b'))");
    CHECK(q::Expression::parse("1e-3 < .5").toString() == "(0.001 < 0.5)");
    CHECK(q::Expression::parse("name == 'it\\'s'").toString() == "(name == 'it\\'s')");
    CHECK(q::Expression::parse("a is not null").toString() == "(a is not null)");
    CHECK(q::Expression::parse("a > 1 and b < 2 and c == 3").conjuncts().size() == 3);
    CHECK(q::Expression::parse("a > 1 and (b < 2 or c == 3)").conjuncts().size() == 2);
    CHECK(q::Expression::parse("area > 1 and area < pid").columns() == std::vector<std::string>{"area", "pid"});
    const q::Expression equals = q::Expression::equals("pid", q::Literal::ofInt(3));
    CHECK(equals.toString() == "(pid == 3)");
    CHECK(q::Expression::isIn("pid", {q::Literal::ofInt(1), q::Literal::ofInt(2)}).toString() == "(pid in (1, 2))");
    CHECK(q::Expression::equals("pid", q::Literal::null()).toString() == "(pid is null)");
    CHECK(q::Expression::allOf({equals, q::Expression::parse("t > 1")}).toString() == "((pid == 3) and (t > 1))");
}

TEST_CASE("a table cut by `take` keeps types, nulls, lists and texts", "[query][table]")
{
    q::ExecutionContext context;
    const RandomTable data = makeRandomTable(300, 6);
    std::vector<std::uint32_t> rows = {5, 0, 299, 5, q::NO_ROW, 17};
    for (std::size_t index = 0; index < data.table->columnCount(); ++index)
    {
        const q::QueryColumn& source = data.table->column(index);
        const q::QueryColumn taken = q::take(source, rows, context);
        REQUIRE(taken.length() == 6);
        for (std::size_t row = 0; row < rows.size(); ++row)
        {
            if (rows[row] == q::NO_ROW)
            {
                CHECK(taken.isNull(row));
                continue;
            }
            CHECK(taken.isNull(row) == source.isNull(rows[row]));
        }
    }
    // fixed lists and lists
    std::vector<std::int32_t> flat = {1, 2, 3, 4, 5, 6, 7, 8};
    const q::QueryColumn fixed =
        q::QueryColumn::fixedList(q::QueryColumn::primitive(std::vector<std::int32_t>(flat)), 2, 4);
    const q::QueryColumn fixedTaken = q::take(fixed, std::vector<std::uint32_t>{3, 0}, context);
    CHECK(fixedTaken.child()->as<std::int32_t>()[0] == 7);
    CHECK(fixedTaken.child()->as<std::int32_t>()[3] == 2);
    const q::QueryColumn list =
        q::QueryColumn::list(q::ColumnBuffer::adopt(std::vector<std::int64_t>{0, 1, 3, 3, 6}),
                             q::QueryColumn::primitive(std::vector<std::int32_t>{10, 20, 21, 30, 31, 32}), 4);
    const q::QueryColumn listTaken = q::take(list, std::vector<std::uint32_t>{3, 2, 0, 1}, context);
    CHECK(listTaken.listLength(0) == 3);
    CHECK(listTaken.listLength(1) == 0);
    CHECK(listTaken.listLength(2) == 1);
    CHECK(listTaken.listLength(3) == 2);
    const auto values = listTaken.child()->as<std::int32_t>();
    CHECK(std::vector<std::int32_t>(values.begin(), values.end()) == std::vector<std::int32_t>{30, 31, 32, 10, 20, 21});
    // putting columns end to end, lists of different widths included
    const q::QueryColumn asList = q::fixedListAsList(fixed);
    const q::QueryColumn* parts[] = {&asList, nullptr, &list};
    const std::int64_t lengths[] = {4, 2, 4};
    const q::QueryColumn joined = q::concatenate(parts, lengths);
    CHECK(joined.length() == 10);
    CHECK(joined.listLength(0) == 2);
    CHECK(joined.isNull(4));
    CHECK(joined.listLength(9) == 3);
}
