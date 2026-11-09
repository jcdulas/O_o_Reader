// The result queries: the catalogue of the R families, the chain of providers (order, per-row resolution,
// switches), the accumulator that makes the tables, and the optimisation queries on results built in memory. The
// queries on result files run in tests/python/test_result_queries*.py.

#include "QueryTestSupport.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/NamedQueryRegistry.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/providers/quantities/NodePointForceSet.hpp"
#include "oor/query/providers/quantities/QuantityProvider.hpp"
#include "oor/query/results/ResultSets.hpp"
#include "oor/query/results/RowAccumulator.hpp"
#include "oor/results/core/NamedResultBuilder.hpp"
#include "oor/results/core/ResultTables.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cmath>
#include <cstring>
#include <set>

using namespace oor;
namespace q = oor::query;

namespace
{

// A set of results made in memory: a ResultTables with the named results given.
q::ResultSet memorySet(std::vector<std::shared_ptr<results::NamedResult>> named, const std::string& label = "memory")
{
    q::ResultSet set;
    set.analysis = label;
    set.tables = std::make_shared<results::ResultTables>();
    for (auto& result : named)
        set.tables->addNamed(std::move(result));
    return set;
}

results::NamedArrayInput doubles(const std::string& name, const std::vector<double>& values,
                                 std::vector<std::size_t> shape = {})
{
    static std::vector<std::vector<double>> keep; // the bytes must outlive the call: the test cases are short
    keep.push_back(values);
    if (shape.empty())
        shape = {values.size()};
    return {name, results::ArrayType::Float64, shape,
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(keep.back().data()),
                                       values.size() * sizeof(double))};
}

results::NamedArrayInput ints(const std::string& name, const std::vector<std::int32_t>& values)
{
    static std::vector<std::vector<std::int32_t>> keep;
    keep.push_back(values);
    return {name,
            results::ArrayType::Int32,
            {values.size()},
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(keep.back().data()),
                                       values.size() * sizeof(std::int32_t))};
}

std::shared_ptr<const q::QueryTable> runQuery(const std::string& name, const std::vector<q::ResultSet>& sets,
                                              const q::QueryArguments& arguments = {})
{
    base::ThreadPool pool(1);
    q::ExecutionContext context(&pool, nullptr, nullptr);
    const q::QueryRun run{nullptr, arguments, context, nullptr, sets};
    return q::NamedQueryRegistry::instance().run(name, run);
}

std::vector<double> realColumn(const q::QueryTable& table, const std::string& name)
{
    return q::asDoubles(table.column(name));
}

} // namespace

TEST_CASE("the result queries are the families R1 to R10, flagged, with the common parameters of the result queries",
          "[queries][results]")
{
    const q::NamedQueryRegistry& registry = q::NamedQueryRegistry::instance();
    std::set<std::string> families;
    std::size_t count = 0;
    for (const auto& entry : registry.all())
    {
        const q::QueryInfo& info = entry->info();
        const bool isResult = info.family[0] == 'R';
        CHECK(info.resultQuery == isResult);
        if (!isResult)
            continue;
        ++count;
        families.insert(info.family);
        // the parameters of the query and the common ones have distinct names
        std::set<std::string> names;
        for (const q::ParameterInfo& parameter : info.parameters)
            CHECK(names.insert(parameter.name).second);
        for (const q::ParameterInfo& parameter : q::commonParametersFor(info))
            CHECK(names.insert(parameter.name).second);
        // `[step]` is at most once in a form, and a query that has `designCycle` as a column lists it once
        for (const q::ColumnSet& set : info.columnSets)
        {
            int steps = 0;
            for (const q::ColumnSpec& column : set.columns)
                steps += column.kind == q::ColumnKind::Step ? 1 : 0;
            CHECK(steps <= 1);
        }
    }
    CHECK(count == 62);
    CHECK(families.size() == 10);
    CHECK(q::resultParameters().size() == 15);
    CHECK(q::resultParameters().front().name == "subcases");
}

namespace
{

struct NumberSink
{
    std::vector<std::pair<int, std::int16_t>> rows; // the number and the provider that gave it
    std::int16_t current = 0;
    void beginProvider(std::int16_t index, const q::ProviderInfo&) { current = index; }
    [[nodiscard]] std::int64_t rowCount() const noexcept { return static_cast<std::int64_t>(rows.size()); }
    [[nodiscard]] bool has(int number) const
    {
        for (const auto& row : rows)
            if (row.first == number)
                return true;
        return false;
    }
};

class NumbersProvider final : public q::QuantityProvider<int, NumberSink>
{
public:
    NumbersProvider(std::string label, std::vector<int> numbers, bool approximateWay = false)
        : m_label(std::move(label)), m_numbers(std::move(numbers)), m_approximate(approximateWay)
    {
    }
    [[nodiscard]] std::string name() const override { return m_label; }
    [[nodiscard]] bool approximate() const override { return m_approximate; }
    [[nodiscard]] Outcome provide(const q::ResultQueryContext&, const int& request, NumberSink& sink) const override
    {
        Outcome outcome;
        for (const int number : m_numbers)
            if (number <= request && !sink.has(number)) // a way gives the rows the ones before it did not give
            {
                sink.rows.emplace_back(number, sink.current);
                ++outcome.rows;
            }
        if (outcome.rows == 0)
            outcome.reason = "nothing up to " + std::to_string(request);
        return outcome;
    }

private:
    std::string m_label;
    std::vector<int> m_numbers;
    bool m_approximate;
};

} // namespace

TEST_CASE("a chain of providers gives each row from the first way that has it and says what each way did",
          "[queries][results]")
{
    base::ThreadPool pool(1);
    q::ExecutionContext execution(&pool, nullptr, nullptr);
    const q::NamedQueryRegistry& registry = q::NamedQueryRegistry::instance();
    const q::QueryArguments arguments;
    const q::QueryRun run{nullptr, arguments, execution, nullptr, {memorySet({})}};
    const q::ResultQueryContext context(run, registry.find("designHistory")->info());

    q::ProviderChain<int, NumberSink> chain("numbers");
    chain.add(std::make_unique<NumbersProvider>("first", std::vector<int>{1, 2, 3}));
    chain.add(std::make_unique<NumbersProvider>("second", std::vector<int>{2, 3, 4, 5}));
    chain.add(std::make_unique<NumbersProvider>("estimate", std::vector<int>{4, 5, 6}, true));

    SECTION("each way adds the rows the ones before it did not give")
    {
        NumberSink sink;
        const std::vector<q::ProviderAttempt> attempts = chain.resolve(context, 6, sink);
        REQUIRE(attempts.size() == 3);
        CHECK(attempts[0].rows == 3);
        CHECK(attempts[1].rows == 2); // 4 and 5
        CHECK(attempts[2].rows == 1); // 6
        CHECK(attempts[2].approximate);
        std::vector<int> from;
        for (const auto& row : sink.rows)
            from.push_back(row.second);
        CHECK(from == std::vector<int>{0, 0, 0, 1, 1, 2});
    }
    SECTION("a way that has nothing says why, and the info lists the ways in order")
    {
        NumberSink sink;
        const std::vector<q::ProviderAttempt> attempts = chain.resolve(context, 1, sink);
        CHECK(attempts[0].rows == 1);
        CHECK(attempts[1].reason == "nothing up to 1");
        const q::QuantityInfo info = chain.info();
        CHECK(info.quantity == "numbers");
        REQUIRE(info.providers.size() == 3);
        CHECK(info.providers[0].name == "first");
        CHECK(info.providers[2].approximate);
        const std::string text = q::describeAttempts("the numbers", attempts);
        CHECK(text.find("first") < text.find("second"));
        CHECK(text.find("second") < text.find("estimate"));
    }
    SECTION("a way that is switched off is skipped and says so")
    {
        q::ProviderSwitches::setDisabled({"first"});
        NumberSink sink;
        const std::vector<q::ProviderAttempt> attempts = chain.resolve(context, 3, sink);
        q::ProviderSwitches::setDisabled({});
        CHECK(attempts[0].reason == "switched off");
        CHECK(attempts[1].rows == 2); // 2 and 3
        CHECK_FALSE(sink.has(1));
    }
}

TEST_CASE(
    "the accumulator makes the table of the catalogue: its columns and types, nulls for what a block lacks, a source "
    "column",
    "[queries][results]")
{
    q::QueryInfo info;
    info.name = "probe";
    info.resultQuery = true;
    q::ColumnListBuilder columns;
    columns.int32("subcase").step().int32("nodeId").results("t1 t2").dictionary("frame");
    info.columnSets.push_back(columns.build());

    base::ThreadPool pool(1);
    q::ExecutionContext execution(&pool, nullptr, nullptr);
    q::QueryArguments arguments;
    arguments.set("withSource", true);
    const q::QueryRun run{nullptr, arguments, execution, nullptr, {memorySet({})}};
    const q::ResultQueryContext context(run, q::NamedQueryRegistry::instance().find("designHistory")->info());

    q::RowAccumulator accumulator;
    const std::int16_t table = accumulator.addProvider("stored table", false);
    const std::int16_t estimate = accumulator.addProvider("an estimate", true);
    q::RowBlock first(2);
    first.ints("subcase", {1, 1});
    first.ints("nodeId", {10, 20});
    first.reals("t1", {1.5, 2.5});
    first.reals("t2", {0.25, std::nan("")}); // a NaN is a null
    first.texts("frame", {"basic", "basic"});
    first.steps(results::StepKind::Time, {0.1, 0.2});
    first.precision(q::Precision::Single);
    first.provider(table);
    first.validation("validated");
    accumulator.add(std::move(first));
    q::RowBlock second(1); // a block with no step and no t2: nulls
    second.ints("subcase", {2});
    second.ints("nodeId", {30});
    second.reals("t1", {9.0});
    second.texts("frame", {"element"});
    second.precision(q::Precision::Single);
    second.provider(estimate);
    second.validation("experimental");
    accumulator.add(std::move(second));
    const std::shared_ptr<const q::QueryTable> result = accumulator.finish(context, info);

    REQUIRE(result->rowCount() == 3);
    CHECK(result->columnNames() ==
          std::vector<std::string>{"subcase", "time", "nodeId", "t1", "t2", "frame", "source"});
    CHECK(result->column("t1").type() == q::ColumnType::Float32); // a value of a result table stays single precision
    CHECK(result->column("time").isNull(2));
    CHECK(result->column("t2").isNull(1));
    CHECK(result->column("t2").isNull(2));
    CHECK_THAT(realColumn(*result, "t1")[1], Catch::Matchers::WithinAbs(2.5, 1e-6));
    REQUIRE(result->sources().size() == 2);
    CHECK(result->sources()[0].provider == "stored table");
    CHECK(result->sources()[0].rows == 2);
    CHECK(result->sources()[0].status == "validated");
    CHECK(result->sources()[1].approximate);
    CHECK(result->sources()[1].status == "experimental");
}

TEST_CASE("the optimisation queries run on results built in memory", "[queries][results][optimisation]")
{
    // two design variables, two cycles, one response (the objective) with a sensitivity to each variable
    std::vector<std::shared_ptr<results::NamedResult>> named;
    const results::NamedResultOrigin origin;
    named.push_back(results::makeNamedResult(
        "designVariables", origin,
        std::vector<results::NamedArrayInput>{ints("internalId", {1, 2}), ints("id", {10, 20}),
                                              doubles("lower", {1.0, 1.0}), doubles("upper", {5.0, 9.0}),
                                              doubles("moveLimit", {0.2, 0.2}), doubles("unused", {0.0, 0.0})},
        std::vector<results::NamedTextInput>{{"label", {"T", "A"}}}));
    named.push_back(results::makeNamedResult(
        "designHistory", origin,
        std::vector<results::NamedArrayInput>{
            ints("designCycle", {0, 1}), ints("designIteration", {0, 1}), ints("convergenceFlag", {0, 2}),
            ints("convergenceResult", {0, 2}), doubles("objectiveInitial", {100.0, 90.0}),
            doubles("objectiveFinal", {90.0, 80.0}), doubles("maxConstraint", {0.2, -0.1}),
            ints("maxConstraintRow", {1, 1}), doubles("designVariableValues", {2.0, 3.0, 1.0, 4.0}, {2, 2})},
        std::vector<results::NamedTextInput>{{"convergence", {"soft", "hard"}}, {"result", {"no", "hard"}}}));
    named.push_back(results::makeNamedResult(
        "responseValues", origin,
        std::vector<results::NamedArrayInput>{ints("designCycle", {1}), ints("id", {1}), ints("subcase", {0}),
                                              doubles("value", {80.0}), doubles("lower", {std::nan("")}),
                                              doubles("upper", {std::nan("")}), ints("constraintSet", {0}),
                                              ints("isObjective", {1})},
        std::vector<results::NamedTextInput>{{"label", {"WEIGHT"}}, {"responseType", {"weight"}}}));
    named.push_back(results::makeNamedResult(
        "sensitivities", origin,
        std::vector<results::NamedArrayInput>{ints("designCycle", {1, 1}), ints("responseId", {1, 1}),
                                              ints("subcase", {0, 0}), ints("designVariable", {10, 20}),
                                              doubles("sensitivity", {5.0, 2.0})},
        std::vector<results::NamedTextInput>{}));
    const std::vector<q::ResultSet> sets = {memorySet(named)};

    const auto history = runQuery("designHistory", sets);
    REQUIRE(history->rowCount() == 2);
    CHECK(realColumn(*history, "objective") == std::vector<double>{90.0, 80.0});
    // the constraint of a result file is the most critical one: positive when violated, zero otherwise
    CHECK(realColumn(*history, "maxConstraintViolation") == std::vector<double>{0.2, 0.0});

    q::QueryArguments values;
    values.set("values", std::vector<std::pair<std::string, double>>{{"T", 3.0}});
    const auto prediction = runQuery("linearisedPrediction", sets, values);
    REQUIRE(prediction->rowCount() == 1);
    // the response is 80 at the cycle 1 where T = 1: 80 + 5 * (3 - 1)
    CHECK_THAT(realColumn(*prediction, "predicted")[0], Catch::Matchers::WithinAbs(90.0, 1e-9));
    REQUIRE(prediction->sources().size() == 1);
    CHECK(prediction->sources()[0].approximate);

    q::QueryArguments top;
    top.set("onlyActive", false);
    // the constraint is the most critical one of the history (the row 1 of the constraint table: the response 1): the
    // two variables, the one with the larger sensitivity times range (5 * 4 against 2 * 8) first
    const auto drivers = runQuery("constraintDrivers", sets, top);
    REQUIRE(drivers->rowCount() == 2);
    CHECK(q::asInt32(drivers->column("designVariable")) == std::vector<std::int32_t>{10, 20});
    CHECK(q::asInt32(drivers->column("rank")) == std::vector<std::int32_t>{1, 2});
    const auto importance = runQuery("variableImportance", sets);
    REQUIRE(importance->rowCount() == 2);
    // normalised by the range of the variable (4 and 8) over the objective (80)
    CHECK_THAT(realColumn(*importance, "normalisedObjectiveSensitivity")[0],
               Catch::Matchers::WithinAbs(5.0 * 4.0 / 80.0, 1e-12));
    CHECK_THAT(realColumn(*importance, "normalisedObjectiveSensitivity")[1],
               Catch::Matchers::WithinAbs(2.0 * 8.0 / 80.0, 1e-12));
    CHECK(runQuery("variablesAtBounds", sets)->rowCount() == 1); // T is on its lower bound at the cycle 1
}

TEST_CASE("a result query with no results says so, and one that needs the model names it", "[queries][results]")
{
    base::ThreadPool pool(1);
    q::ExecutionContext execution(&pool, nullptr, nullptr);
    const q::QueryArguments arguments;
    const q::QueryRun none{nullptr, arguments, execution, nullptr, {}};
    CHECK_THROWS_AS(q::NamedQueryRegistry::instance().run("designHistory", none), q::QueryError);
    const q::QueryRun noModel{nullptr, arguments, execution, nullptr, {memorySet({})}};
    CHECK_THROWS_WITH(q::NamedQueryRegistry::instance().run("sectionCut", noModel),
                      Catch::Matchers::ContainsSubstring("needs the model"));
}

TEST_CASE("the rows of a grid point force table are known by the labels the solver writes", "[queries][results]")
{
    CHECK(q::nodePointForceKindOf("appliedLoad") == q::NodePointForceKind::AppliedLoad);
    CHECK(q::nodePointForceKindOf("singlePointConstrForce") == q::NodePointForceKind::Spc);
    CHECK(q::nodePointForceKindOf("multiPointConstrForce") == q::NodePointForceKind::Mpc);
    CHECK(q::nodePointForceKindOf(" total ") == q::NodePointForceKind::Total);
    // An element row is known by its element ID, whatever its label; a condensed unit is a row of its own.
    CHECK(q::nodePointForceKindOf("ShellQuad4") == q::NodePointForceKind::Other);
    CHECK(q::nodePointForceKindOf("condensedUnit") == q::NodePointForceKind::Other);
}
