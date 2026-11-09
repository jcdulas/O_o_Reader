// The Arrow C Data Interface export of a table: the format of every type, the buffers (the same bytes as the columns,
// no copy), the lifetime (a release callback holds the buffers after the table is gone), the stream and the metadata.

#include "QueryTestSupport.hpp"

#include "oor/query/table/ArrowExport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstring>

namespace q = oor::query;

namespace
{

std::shared_ptr<q::QueryTable> mixedTable()
{
    auto table = std::make_shared<q::QueryTable>();
    table->addColumn("i", q::QueryColumn::primitive(std::vector<std::int32_t>{1, 2, 3}));
    table->addColumn("l", q::QueryColumn::primitive(std::vector<std::int64_t>{10, 20, 30}));
    table->addColumn("d", q::QueryColumn::primitive(std::vector<double>{0.5, 1.5, 2.5}));
    table->addColumn("u8", q::QueryColumn::primitive(std::vector<std::uint8_t>{7, 8, 9}));
    table->addColumn("b", q::QueryColumn::boolean(std::vector<std::uint8_t>{1, 0, 1}));
    table->addColumn("s", q::QueryColumn::utf8(std::vector<std::string>{"alpha", "", "gamma"}));
    table->addColumn("t", q::QueryColumn::dictionary(std::vector<std::int32_t>{0, 1, 0},
                                                     std::vector<std::string>{"CQUAD4", "CTRIA3"}));
    table->addColumn(
        "f", q::QueryColumn::fixedList(q::QueryColumn::primitive(std::vector<std::int32_t>{1, 2, 3, 4, 5, 6}), 2, 3));
    table->addColumn("v", q::QueryColumn::list(q::ColumnBuffer::adopt(std::vector<std::int64_t>{0, 1, 1, 4}),
                                               q::QueryColumn::primitive(std::vector<std::int32_t>{9, 8, 7, 6}), 3));
    std::int64_t nullCount = 0;
    q::ColumnBuffer bits = q::validityFromFlags(std::vector<std::uint8_t>{1, 0, 1}, nullCount);
    table->addColumn("n", q::QueryColumn::primitive(std::vector<double>{1.0, 0.0, 3.0}).withValidity(bits, nullCount));
    table->setContractVersion(14);
    return table;
}

} // namespace

TEST_CASE("every column type has its Arrow format", "[query][arrow]")
{
    const auto table = mixedTable();
    ArrowSchema schema{};
    q::ArrowExport::exportSchema(*table, &schema);
    REQUIRE(std::string(schema.format) == "+s");
    REQUIRE(schema.n_children == 10);
    const char* formats[] = {"i", "l", "g", "C", "b", "u", "i", "+w:2", "+L", "g"};
    for (std::int64_t index = 0; index < schema.n_children; ++index)
    {
        CHECK(std::string(schema.children[index]->format) == formats[index]);
        CHECK(std::string(schema.children[index]->name) == table->name(static_cast<std::size_t>(index)));
    }
    CHECK(schema.children[6]->dictionary != nullptr);
    CHECK(std::string(schema.children[6]->dictionary->format) == "u");
    CHECK(std::string(schema.children[7]->children[0]->format) == "i");
    CHECK(std::string(schema.children[8]->children[0]->format) == "i");
    // the metadata carries the version of the data contract
    REQUIRE(schema.metadata != nullptr);
    std::int32_t pairs = 0;
    std::memcpy(&pairs, schema.metadata, sizeof(pairs));
    CHECK(pairs == 1);
    CHECK(std::string(schema.metadata + 8, 32) == "oor_reader.data_contract_version");
    schema.release(&schema);
    CHECK(schema.release == nullptr);
}

TEST_CASE("the exported arrays are the bytes of the columns, not a copy", "[query][arrow]")
{
    const auto table = mixedTable();
    ArrowArray array{};
    q::ArrowExport::exportArray(*table, &array);
    REQUIRE(array.length == 3);
    REQUIRE(array.n_children == 10);
    CHECK(array.null_count == 0);
    for (std::size_t index : {0U, 1U, 2U, 3U})
    {
        CHECK(array.children[index]->buffers[1] == table->column(index).values().data());
        CHECK(array.children[index]->null_count == 0);
        CHECK(array.children[index]->buffers[0] == nullptr);
    }
    CHECK(array.children[4]->buffers[1] == table->column(4).values().data());
    // text: offsets and characters
    CHECK(array.children[5]->n_buffers == 3);
    CHECK(array.children[5]->buffers[1] == table->column(5).offsets().data());
    CHECK(array.children[5]->buffers[2] == table->column(5).values().data());
    // dictionary: indices and a dictionary array
    CHECK(array.children[6]->buffers[1] == table->column(6).values().data());
    REQUIRE(array.children[6]->dictionary != nullptr);
    CHECK(array.children[6]->dictionary->length == 2);
    // fixed list: no buffer of its own beyond the validity, the values in the child
    CHECK(array.children[7]->n_buffers == 1);
    CHECK(array.children[7]->children[0]->length == 6);
    CHECK(array.children[7]->children[0]->buffers[1] == table->column(7).child()->values().data());
    // list: offsets and a child
    CHECK(array.children[8]->n_buffers == 2);
    CHECK(array.children[8]->buffers[1] == table->column(8).offsets().data());
    CHECK(array.children[8]->children[0]->length == 4);
    // nulls: the bitmap and the count
    CHECK(array.children[9]->null_count == 1);
    REQUIRE(array.children[9]->buffers[0] != nullptr);
    CHECK((static_cast<const std::uint8_t*>(array.children[9]->buffers[0])[0] & 0x7) == 0x5);
    array.release(&array);
    CHECK(array.release == nullptr);
}

TEST_CASE("the buffers live as long as the consumer holds the array", "[query][arrow]")
{
    ArrowArray array{};
    ArrowSchema schema{};
    {
        auto table = mixedTable();
        q::ArrowExport::exportArray(*table, &array);
        q::ArrowExport::exportSchema(*table, &schema);
    } // the table is gone; only the array holds the buffers
    const auto* doubles = static_cast<const double*>(array.children[2]->buffers[1]);
    CHECK(doubles[0] == 0.5);
    CHECK(doubles[2] == 2.5);
    const auto* offsets = static_cast<const std::int64_t*>(array.children[8]->buffers[1]);
    CHECK(offsets[3] == 4);
    const auto* characters = static_cast<const char*>(array.children[5]->buffers[2]);
    CHECK(std::string(characters, 5) == "alpha");
    array.release(&array);
    schema.release(&schema);
}

TEST_CASE("a table with no row and a column of no byte export", "[query][arrow]")
{
    q::QueryTable table;
    table.addColumn("i", q::QueryColumn::primitive(std::vector<std::int32_t>{}));
    table.addColumn("s", q::QueryColumn::utf8(std::vector<std::string>{}));
    ArrowArray array{};
    q::ArrowExport::exportArray(table, &array);
    CHECK(array.length == 0);
    CHECK(array.children[0]->buffers[1] != nullptr);
    CHECK(array.children[1]->buffers[2] != nullptr);
    array.release(&array);
}

TEST_CASE("the stream gives the schema, one batch, and the end", "[query][arrow]")
{
    ArrowArrayStream stream{};
    q::ArrowExport::exportStream(mixedTable(), &stream);
    ArrowSchema schema{};
    REQUIRE(stream.get_schema(&stream, &schema) == 0);
    CHECK(schema.n_children == 10);
    schema.release(&schema);
    ArrowArray batch{};
    REQUIRE(stream.get_next(&stream, &batch) == 0);
    REQUIRE(batch.release != nullptr);
    CHECK(batch.length == 3);
    batch.release(&batch);
    ArrowArray end{};
    end.release = reinterpret_cast<void (*)(ArrowArray*)>(1); // anything but null
    REQUIRE(stream.get_next(&stream, &end) == 0);
    CHECK(end.release == nullptr);
    stream.release(&stream);
    CHECK(stream.release == nullptr);
}
