#include "oor/base/threading/ThreadPool.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/EnvelopeOperator.hpp"
#include "oor/query/operators/ExecutionContext.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <map>
#include <random>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::query;

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// Three analyses (a, b, c) of two items, each in two subcases.
struct Rows
{
    std::vector<std::string> analysis;
    std::vector<std::int32_t> item;
    std::vector<std::int32_t> subcase;
    std::vector<double> value;
    std::vector<double> other;

    void add(const std::string& label, std::int32_t itemId, std::int32_t subcaseId, double number)
    {
        analysis.push_back(label);
        item.push_back(itemId);
        subcase.push_back(subcaseId);
        value.push_back(number);
        other.push_back(number * 10.0);
    }

    [[nodiscard]] QueryTable table() const
    {
        QueryTable result;
        result.addColumn("analysis", QueryColumn::utf8(std::span<const std::string>(analysis)));
        result.addColumn("subcase", QueryColumn::primitive(std::vector<std::int32_t>(subcase)));
        result.addColumn("item", QueryColumn::primitive(std::vector<std::int32_t>(item)));
        result.addColumn("value", QueryColumn::primitive(std::vector<double>(value)));
        result.addColumn("other", QueryColumn::primitive(std::vector<double>(other)));
        return result;
    }
};

Rows example()
{
    Rows rows;
    // item 1, subcase 1: a 1, b -5, c 3; subcase 2: a 2, b 0, c -1
    // item 2, subcase 1: a 7, b 7, c nan
    rows.add("a", 1, 1, 1.0);
    rows.add("b", 1, 1, -5.0);
    rows.add("c", 1, 1, 3.0);
    rows.add("a", 1, 2, 2.0);
    rows.add("b", 1, 2, 0.0);
    rows.add("c", 1, 2, -1.0);
    rows.add("a", 2, 1, 7.0);
    rows.add("b", 2, 1, 7.0);
    rows.add("c", 2, 1, NOT_A_NUMBER);
    return rows;
}

struct Answer
{
    std::string analysis;
    double value;
    std::int64_t count;
};

std::map<std::pair<std::int32_t, std::int32_t>, Answer> answers(const QueryTable& table)
{
    std::map<std::pair<std::int32_t, std::int32_t>, Answer> result;
    for (std::size_t row = 0; row < static_cast<std::size_t>(table.rowCount()); ++row)
        result[{table.column("item").as<std::int32_t>()[row], table.column("subcase").as<std::int32_t>()[row]}] = {
            std::string(table.column("analysis").textAt(row)), table.column("value").as<double>()[row],
            table.column("envelopeCount").as<std::int64_t>()[row]};
    return result;
}

} // namespace

TEST_CASE("the envelope keeps the critical row of each item and says its analysis", "[query][envelope]")
{
    base::ThreadPool pool(3);
    ExecutionContext context(&pool, nullptr, nullptr);
    const QueryTable table = example().table();
    EnvelopeSpec spec;
    spec.value = "value";
    SECTION("max")
    {
        const auto result = envelopeTable(table, spec, context);
        const auto found = answers(*result);
        REQUIRE(found.size() == 3);
        CHECK(found.at({1, 1}).analysis == "c");
        CHECK(found.at({1, 1}).value == 3.0);
        CHECK(found.at({1, 1}).count == 3);
        CHECK(found.at({1, 2}).analysis == "a");
        // a tie keeps the first row of the table; a NaN never wins
        CHECK(found.at({2, 1}).analysis == "a");
        CHECK(found.at({2, 1}).count == 3);
        // the row kept is a whole row of the table
        CHECK(result->column("other").as<double>()[0] == 30.0);
    }
    SECTION("min")
    {
        spec.by = EnvelopeBy::Min;
        const auto found = answers(*envelopeTable(table, spec, context));
        CHECK(found.at({1, 1}).analysis == "b");
        CHECK(found.at({1, 1}).value == -5.0);
        CHECK(found.at({1, 2}).analysis == "c");
    }
    SECTION("absMax keeps the sign of the value")
    {
        spec.by = EnvelopeBy::AbsMax;
        const auto found = answers(*envelopeTable(table, spec, context));
        CHECK(found.at({1, 1}).analysis == "b");
        CHECK(found.at({1, 1}).value == -5.0);
        CHECK(found.at({1, 2}).analysis == "a");
    }
    SECTION("over the analyses and the subcases")
    {
        spec.over = {"analysis", "subcase"};
        const auto result = envelopeTable(table, spec, context);
        REQUIRE(result->rowCount() == 2);
        CHECK(result->column("item").as<std::int32_t>()[0] == 1);
        CHECK(result->column("value").as<double>()[0] == 3.0);
        CHECK(result->column("subcase").as<std::int32_t>()[0] == 1);
        CHECK(result->column("envelopeCount").as<std::int64_t>()[0] == 6);
    }
    SECTION("some analyses only")
    {
        spec.analyses = {"a", "b"};
        const auto found = answers(*envelopeTable(table, spec, context));
        CHECK(found.at({1, 1}).analysis == "a");
        CHECK(found.at({1, 1}).count == 2);
    }
}

TEST_CASE("the envelope says what is wrong with the request", "[query][envelope]")
{
    base::ThreadPool pool(0);
    ExecutionContext context(&pool, nullptr, nullptr);
    const QueryTable table = example().table();
    EnvelopeSpec spec;
    spec.value = "nothing";
    CHECK_THROWS_AS(envelopeTable(table, spec, context), QueryError);
    spec.value = "analysis";
    CHECK_THROWS_AS(envelopeTable(table, spec, context), QueryError); // not a number
    spec.value = "value";
    spec.over = {};
    CHECK_THROWS_AS(envelopeTable(table, spec, context), QueryError);
    spec.over = {"analysis"};
    spec.analyses = {"z"};
    CHECK_THROWS_AS(envelopeTable(table, spec, context), QueryError);
    spec.analyses = {};
    spec.keys = {"analysis"};
    CHECK_THROWS_AS(envelopeTable(table, spec, context), QueryError);
    CHECK(envelopeByFromName("absmax") == EnvelopeBy::AbsMax);
    CHECK_THROWS_AS(envelopeByFromName("median"), QueryError);
}

TEST_CASE("the envelope is the same on any number of threads", "[query][envelope]")
{
    std::mt19937 generator(3);
    Rows rows;
    const std::vector<std::string> labels = {"a", "b", "c", "d"};
    for (int item = 1; item <= 3000; ++item)
        for (const std::string& label : labels)
            rows.add(label, item % 700, static_cast<std::int32_t>(generator() % 3),
                     static_cast<double>(generator() % 1000) - 500.0);
    const QueryTable table = rows.table();
    EnvelopeSpec spec;
    spec.value = "value";
    spec.by = EnvelopeBy::AbsMax;
    base::ThreadPool one(0);
    base::ThreadPool many(5);
    ExecutionContext serial(&one, nullptr, nullptr);
    ExecutionContext parallel(&many, nullptr, nullptr);
    const auto left = envelopeTable(table, spec, serial);
    const auto right = envelopeTable(table, spec, parallel);
    REQUIRE(left->rowCount() == right->rowCount());
    for (std::size_t row = 0; row < static_cast<std::size_t>(left->rowCount()); ++row)
    {
        CHECK(left->column("value").as<double>()[row] == right->column("value").as<double>()[row]);
        CHECK(left->column("analysis").textAt(row) == right->column("analysis").textAt(row));
    }
    // against a direct computation
    std::map<std::pair<std::int32_t, std::int32_t>, double> best;
    for (std::size_t row = 0; row < rows.value.size(); ++row)
    {
        auto [position, inserted] = best.try_emplace({rows.item[row], rows.subcase[row]}, rows.value[row]);
        if (!inserted && std::fabs(rows.value[row]) > std::fabs(position->second))
            position->second = rows.value[row];
    }
    REQUIRE(static_cast<std::size_t>(left->rowCount()) == best.size());
    for (std::size_t row = 0; row < static_cast<std::size_t>(left->rowCount()); ++row)
        CHECK(left->column("value").as<double>()[row] ==
              best.at({left->column("item").as<std::int32_t>()[row], left->column("subcase").as<std::int32_t>()[row]}));
}
