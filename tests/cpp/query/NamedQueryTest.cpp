// The named query framework: the registry and its catalogue, the typed arguments, the table builder with its
// nulls and its empty tables. The queries themselves run on models in tests/python/test_named_queries*.py (they read
// the columns registered for every card, src/query/providers/columns/).

#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/NamedQueryRegistry.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CaseRequests.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <set>

using namespace oor;
namespace q = oor::query;

TEST_CASE("request text in query columns uses the model's neutral names", "[queries][named][words]")
{
    model::CaseEntry entry;
    entry.kind = model::EntryKind::Output;
    entry.name = "DisplacementOutput";
    entry.options = {"SortByPoint", "RealPart", "PrintListing"};
    entry.value = model::CaseValue::ofKeyword("All");
    CHECK(q::requestText(entry) == "DisplacementOutput(SortByPoint,RealPart,PrintListing) = All");
}

TEST_CASE("the registry has every query once, in the order of its families, with a consistent catalogue entry",
          "[queries][named]")
{
    const q::NamedQueryRegistry& registry = q::NamedQueryRegistry::instance();
    std::set<std::string> names;
    std::string family = "T0";
    int lastNumber = 0; // the families of the model (T) come before those of the results (R)
    for (const auto& entry : registry.all())
    {
        const q::QueryInfo& info = entry->info();
        CHECK(names.insert(info.name).second);
        CHECK_FALSE(info.description.empty());
        REQUIRE_FALSE(info.columnSets.empty());
        // families come in order (T1 .. T14, then R1 .. R10, none skipped backwards)
        const int number = (info.family[0] == 'R' ? 100 : 0) + std::stoi(info.family.substr(1));
        CHECK(number >= lastNumber);
        lastNumber = number;
        family = info.family;
        // the columns of each set have distinct names; a query with several sets names the parameter that selects them
        for (const q::ColumnSet& set : info.columnSets)
        {
            std::set<std::string> columns;
            for (const q::ColumnSpec& column : set.columns)
                CHECK(columns.insert(column.name).second);
            CHECK_FALSE(set.columns.empty());
        }
        if (info.columnSets.size() > 1 && !info.detailParameter.empty())
        {
            bool found = false;
            for (const q::ParameterInfo& parameter : info.parameters)
                found = found || parameter.name == info.detailParameter;
            CHECK(found);
        }
        // the parameter names do not clash with the common ones
        for (const q::ParameterInfo& parameter : info.parameters)
        {
            for (const q::ParameterInfo& common : q::commonParameters())
                CHECK(parameter.name != common.name);
        }
    }
    CHECK(names.size() == 169);
    CHECK(registry.find("panels") == nullptr); // removed, with panel_stiffeners and fastener_patches
    CHECK(registry.find("freeEdges") != nullptr);
    CHECK(registry.find("no_such_query") == nullptr);
    CHECK(family == "R10");
}

TEST_CASE("the common parameters are the same for every query", "[queries][named]")
{
    const auto& common = q::commonParameters();
    REQUIRE(common.size() == 3);
    CHECK(common[0].name == "columns");
    CHECK(common[1].name == "sort");
    CHECK(common[2].name == "limit");
}

TEST_CASE("arguments are typed and a missing one takes the default", "[queries][named]")
{
    q::QueryArguments arguments;
    arguments.set("tolerance", 0.5);
    arguments.set("types", std::vector<std::string>{"CQUAD4", "CTRIA3"});
    arguments.set("elementIds", std::vector<std::int64_t>{3, 1, 2});
    arguments.set("detail", std::string("elements"));
    arguments.set("nothing", std::monostate{});
    CHECK(arguments.has("tolerance"));
    CHECK_FALSE(arguments.has("nothing"));
    CHECK_FALSE(arguments.has("absent"));
    CHECK(arguments.realOr("tolerance", 9.0) == 0.5);
    CHECK(arguments.realOr("absent", 9.0) == 9.0);
    CHECK(arguments.boolOr("absent", true));
    CHECK(arguments.integerOr("absent", 4) == 4);
    CHECK(arguments.textOr("detail", "components") == "elements");
    CHECK(arguments.textOr("absent", "components") == "components");
    CHECK(arguments.texts("types") == std::vector<std::string>{"CQUAD4", "CTRIA3"});
    CHECK(arguments.texts("absent").empty());
    REQUIRE(arguments.ids("elementIds").has_value());
    CHECK(arguments.ids("elementIds")->size() == 3);
    CHECK_FALSE(arguments.ids("absent").has_value());
    CHECK_THROWS_AS(arguments.boolOr("tolerance", false), q::QueryTypeError);
}

TEST_CASE("the table builder turns NaN into a null and keeps the value in the array", "[queries][named]")
{
    q::TableBuilder builder;
    builder.int32("id", {1, 2, 3});
    builder.real("x", {1.0, std::nan(""), 3.0});
    builder.int32("maybe", {7, 0, 9}, {1, 0, 1});
    builder.flag("flag", {1, 0, 1});
    builder.dictionary("type", std::vector<std::string>{"CQUAD4", "CTRIA3", "CQUAD4"});
    builder.intLists("nodes", {{1, 2, 3}, {}, {4}});
    const auto table = builder.finish();
    REQUIRE(table->rowCount() == 3);
    CHECK(table->columnNames() == std::vector<std::string>{"id", "x", "maybe", "flag", "type", "nodes"});
    CHECK_FALSE(table->column("x").isNull(0));
    CHECK(table->column("x").isNull(1));
    CHECK(std::isnan(table->column("x").as<double>()[1]));
    CHECK(table->column("maybe").isNull(1));
    CHECK_FALSE(table->column("maybe").isNull(0));
    CHECK(table->column("nodes").type() == q::ColumnType::List);
    CHECK(table->column("type").type() == q::ColumnType::Dictionary);
}

TEST_CASE("a column of another length is refused", "[queries][named]")
{
    q::TableBuilder builder;
    builder.int32("id", {1, 2, 3});
    CHECK_THROWS_AS(builder.real("x", {1.0, 2.0}), q::QueryError);
}

TEST_CASE("an empty table has the columns and types the catalogue declares", "[queries][named]")
{
    q::ColumnListBuilder columns;
    columns.int32("elementId")
        .int64("group")
        .real("area")
        .flag("closed")
        .text("name")
        .dictionary("type")
        .intList("nodes")
        .realList("values");
    const q::ColumnSet set = columns.build();
    const auto table = q::emptyTable(set);
    CHECK(table->rowCount() == 0);
    CHECK(table->columnNames() ==
          std::vector<std::string>{"elementId", "group", "area", "closed", "name", "type", "nodes", "values"});
    CHECK(table->column("elementId").type() == q::ColumnType::Int32);
    CHECK(table->column("group").type() == q::ColumnType::Int64);
    CHECK(table->column("area").type() == q::ColumnType::Float64);
    CHECK(table->column("closed").type() == q::ColumnType::Bool);
    CHECK(table->column("type").type() == q::ColumnType::Dictionary);
    CHECK(table->column("nodes").type() == q::ColumnType::List);
}

TEST_CASE("the vector3 helper makes three columns and the second form of a query is found by its detail",
          "[queries][named]")
{
    q::QueryInfo info;
    q::ColumnListBuilder first;
    first.int32("a").vector3("normal", "XYZ");
    q::ColumnListBuilder second;
    second.int32("b");
    info.columnSets.push_back(first.build());
    info.columnSets.push_back(second.build("elements"));
    CHECK(info.defaultColumns().columns.size() == 4);
    CHECK(info.defaultColumns().columns[1].name == "normalX");
    CHECK(info.columnsFor("elements").columns.size() == 1);
    CHECK(info.columnsFor("nope").columns.size() == 4); // the default form
}

TEST_CASE("an unknown query or parameter is a QueryError", "[queries][named]")
{
    q::ExecutionContext context;
    q::QueryArguments arguments;
    const q::QueryRun none{nullptr, arguments, context, nullptr, {}};
    CHECK_THROWS_AS(q::NamedQueryRegistry::instance().run("no_such_query", none), q::QueryError);
    arguments.set("bogus", std::int64_t{1});
    CHECK_THROWS_AS(q::NamedQueryRegistry::instance().run("freeEdges", none), q::QueryError);
}
