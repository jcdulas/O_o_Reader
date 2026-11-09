// The Float32 column: a column of single precision values keeps its type through the operators, the Arrow export
// ("f") and the CSV, with the float64 columns unchanged next to it.

#include "QueryTestSupport.hpp"

#include "oor/query/operators/Expression.hpp"
#include "oor/query/table/ArrowExport.hpp"
#include "oor/query/table/CsvExport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <sstream>

namespace q = oor::query;
using q::testing::runOn;

namespace
{

std::shared_ptr<q::QueryTable> singleTable()
{
    auto table = std::make_shared<q::QueryTable>();
    table->addColumn("id", q::QueryColumn::primitive(std::vector<std::int32_t>{1, 2, 3, 4}));
    table->addColumn("s", q::QueryColumn::primitive(std::vector<float>{4.5f, -1.0f, 2.25f, 0.5f}));
    table->addColumn("d", q::QueryColumn::primitive(std::vector<double>{4.5, -1.0, 2.25, 0.5}));
    // a complex result in single precision: the real and imaginary parts as a fixed list of two floats
    table->addColumn(
        "c", q::QueryColumn::fixedList(q::QueryColumn::primitive(std::vector<float>{1, 2, 3, 4, 5, 6, 7, 8}), 2, 4));
    return table;
}

q::Query queryOn(const std::shared_ptr<q::QueryTable>& table)
{
    return q::Query(std::make_shared<q::TableSource>(table));
}

} // namespace

TEST_CASE("a Float32 column has its type, its width and its Arrow format", "[query][float32]")
{
    const auto table = singleTable();
    CHECK(table->column("s").type() == q::ColumnType::Float32);
    CHECK(q::valueWidth(q::ColumnType::Float32) == 4);
    CHECK(q::isFloating(q::ColumnType::Float32));
    CHECK(q::isNumeric(q::ColumnType::Float32));
    CHECK(q::columnTypeName(q::ColumnType::Float32) == "float32");
    ArrowSchema schema{};
    q::ArrowExport::exportSchema(*table, &schema);
    CHECK(std::string(schema.children[1]->format) == "f");
    CHECK(std::string(schema.children[2]->format) == "g");
    CHECK(std::string(schema.children[3]->format) == "+w:2");
    CHECK(std::string(schema.children[3]->children[0]->format) == "f");
    schema.release(&schema);
    ArrowArray array{};
    q::ArrowExport::exportArray(*table, &array);
    // the exported array is the bytes of the column, not a copy
    CHECK(array.children[1]->buffers[1] == table->column("s").values().data());
    array.release(&array);
}

TEST_CASE("the operators keep a Float32 column single and the Float64 one double", "[query][float32]")
{
    const auto table = singleTable();
    const auto sorted = runOn(queryOn(table).sortBy({{"s", true}}), 1);
    REQUIRE(sorted->column("s").type() == q::ColumnType::Float32);
    CHECK(sorted->column("d").type() == q::ColumnType::Float64);
    const auto values = sorted->column("s").as<float>();
    CHECK(std::vector<float>(values.begin(), values.end()) == std::vector<float>{4.5f, 2.25f, 0.5f, -1.0f});
    CHECK(sorted->column("id").as<std::int32_t>()[0] == 1);
    // a comparison of single precision values with a literal selects the rows the double comparison does
    const auto kept = runOn(queryOn(table).where(q::Expression::parse("s > 0.4 and s < 3")), 1);
    CHECK(kept->column("s").type() == q::ColumnType::Float32);
    CHECK(kept->column("id").as<std::int32_t>()[0] == 3);
    REQUIRE(kept->rowCount() == 2);
    // the rows of the fixed list follow the rows
    const auto limited = runOn(queryOn(table).sortBy({{"s", false}}).limit(2), 1);
    const auto list = limited->column("c").child()->as<float>();
    CHECK(std::vector<float>(list.begin(), list.end()) == std::vector<float>{3, 4, 7, 8});
}

TEST_CASE("a Float32 column is written to a CSV with the digits of a single", "[query][float32]")
{
    const auto table = singleTable();
    std::ostringstream out;
    q::writeCsv(*table, out);
    const std::string text = out.str();
    CHECK(text.find("2.25") != std::string::npos);
    CHECK(text.find("4.5") != std::string::npos);
}
