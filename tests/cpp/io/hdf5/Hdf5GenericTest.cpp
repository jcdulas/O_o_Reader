#include "oor/base/errors/Errors.hpp"
#include "oor/io/hdf5/Hdf5File.hpp"
#include "oor/io/hdf5/Hdf5GlobalHeapCheck.hpp"
#include "oor/io/hdf5/Hdf5Table.hpp"
#include "oor/io/hdf5/Hdf5Tree.hpp"
#include "oor/io/hdf5/Hdf5Writer.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

using namespace oor::io;
using oor::base::IoError;
using oor::base::LimitExceededError;
using oor::base::OperationCancelled;
using oor::base::UsageError;

namespace
{

class TemporaryDirectory
{
public:
    explicit TemporaryDirectory(const std::string& name) : m_path(std::filesystem::temp_directory_path() / name)
    {
        std::error_code ignored;
        std::filesystem::remove_all(m_path, ignored);
        std::filesystem::create_directories(m_path);
    }
    ~TemporaryDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(m_path, ignored);
    }
    [[nodiscard]] std::filesystem::path file(const std::string& name) const { return m_path / name; }
    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }
    [[nodiscard]] std::size_t fileCount() const
    {
        std::size_t count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(m_path))
            count += entry.is_regular_file() ? 1 : 0;
        return count;
    }

private:
    std::filesystem::path m_path;
};

template <typename T>
Hdf5WriteArray arrayOf(Hdf5ValueKind kind, const std::vector<T>& values, std::vector<std::uint64_t> shape)
{
    Hdf5WriteArray array;
    array.kind = kind;
    array.shape = std::move(shape);
    array.data = values.data();
    return array;
}

Hdf5WriteArray stringsOf(const std::vector<std::string>& values, std::vector<std::uint64_t> shape)
{
    Hdf5WriteArray array;
    array.kind = Hdf5ValueKind::VarString;
    array.shape = std::move(shape);
    array.strings = values;
    return array;
}

Hdf5WriteColumn column(const std::string& name, Hdf5WriteArray array)
{
    Hdf5WriteColumn result;
    result.name = name;
    result.array = std::move(array);
    return result;
}

Hdf5WriteAttribute attribute(const std::string& name, Hdf5WriteArray array)
{
    Hdf5WriteAttribute result;
    result.name = name;
    result.array = std::move(array);
    return result;
}

template <typename T>
std::vector<T> valuesOf(const Hdf5Column& source)
{
    std::vector<T> values(source.data.size() / sizeof(T));
    std::memcpy(values.data(), source.data.data(), source.data.size());
    return values;
}

// The sample table: 5 rows of every kind of member, a nested compound and an array member.
struct SampleData
{
    std::vector<std::int32_t> domain{1, 1, 2, 2, 2};
    std::vector<std::int64_t> ids{10, 11, 12, 13, 14};
    std::vector<double> x{0.5, 1.5, 2.5, 3.5, 4.5};
    std::vector<float> f{1.f, 2.f, 3.f, 4.f, 5.f};
    std::vector<std::uint8_t> flag{1, 0, 1, 0, 1};
    std::vector<char> names = {'A', 'B', 0, 0, 'C', 'D', 'E', 'F', 'G', 0, 0, 0, 'H', 'I', 'J', 'K', 'L', 0, 0, 0};
    std::vector<std::string> titles{"one", "two", "three", "", "five"};
    std::vector<double> vec{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    std::vector<std::int16_t> a{-1, -2, -3, -4, -5};
    std::vector<std::uint8_t> b{200, 201, 202, 203, 204};

    [[nodiscard]] Hdf5WriteDataset dataset(const std::string& path) const
    {
        Hdf5WriteDataset result;
        result.path = path;
        result.columns.push_back(column("DOMAIN_ID", arrayOf(Hdf5ValueKind::Int32, domain, {5})));
        result.columns.push_back(column("ID", arrayOf(Hdf5ValueKind::Int64, ids, {5})));
        result.columns.push_back(column("X", arrayOf(Hdf5ValueKind::Float64, x, {5})));
        result.columns.push_back(column("F", arrayOf(Hdf5ValueKind::Float32, f, {5})));
        result.columns.push_back(column("FLAG", arrayOf(Hdf5ValueKind::Bool, flag, {5})));
        Hdf5WriteArray text = arrayOf(Hdf5ValueKind::FixedString, names, {5});
        text.stringBytes = 4;
        result.columns.push_back(column("NAME", text));
        result.columns.push_back(column("TITLE", stringsOf(titles, {5})));
        result.columns.push_back(column("VEC", arrayOf(Hdf5ValueKind::Float64, vec, {5, 3})));
        Hdf5WriteColumn nested;
        nested.name = "P";
        nested.members.push_back(column("A", arrayOf(Hdf5ValueKind::Int16, a, {5})));
        nested.members.push_back(column("B", arrayOf(Hdf5ValueKind::UInt8, b, {5})));
        result.columns.push_back(std::move(nested));
        return result;
    }
};

const Hdf5Node& nodeAt(const Hdf5Tree& tree, const char* path)
{
    const Hdf5Node* node = tree.find(path);
    REQUIRE(node != nullptr);
    return *node;
}

} // namespace

TEST_CASE("every kind of column is written and read back, nested compound and array member included", "[io][hdf5]")
{
    const TemporaryDirectory directory("oor_h5_generic_kinds");
    const SampleData data;
    Hdf5WriteRequest request;
    request.datasets.push_back(data.dataset("/RESULTS/GROUP/TABLE"));
    writeHdf5(directory.file("kinds.h5"), request);
    REQUIRE(openHdf5ObjectCount() == 0);
    REQUIRE(directory.fileCount() == 1); // no temporary file left behind

    const Hdf5Tree tree = Hdf5Tree::read(directory.file("kinds.h5"));
    REQUIRE(openHdf5ObjectCount() == 0);
    const Hdf5Node& node = nodeAt(tree, "/RESULTS/GROUP/TABLE");
    REQUIRE(node.kind == Hdf5NodeKind::Dataset);
    REQUIRE(node.shape == std::vector<std::uint64_t>{5});
    REQUIRE(node.layout == "contiguous");
    REQUIRE(node.type->typeClass == Hdf5TypeClass::Compound);
    REQUIRE(node.type->size == 64);
    const auto& members = node.type->members;
    REQUIRE(members.size() == 9);
    REQUIRE(members[0].name == "DOMAIN_ID");
    REQUIRE(members[0].offset == 0);
    REQUIRE(members[0].type.description() == "int32");
    REQUIRE(members[1].offset == 4);
    REQUIRE(members[2].type.numpyString() == "<f8");
    REQUIRE(members[3].type.description() == "float32");
    REQUIRE(members[4].type.typeClass == Hdf5TypeClass::Enum);
    REQUIRE(members[4].type.enumValues.size() == 2);
    REQUIRE(members[5].type.description() == "S4");
    REQUIRE(members[5].type.padding == "nullpad");
    REQUIRE(members[6].type.variableLength);
    REQUIRE(members[6].type.description() == "str");
    REQUIRE(members[7].type.typeClass == Hdf5TypeClass::Array);
    REQUIRE(members[7].type.dims == std::vector<std::uint64_t>{3});
    REQUIRE(members[7].type.description() == "float64[3]");
    REQUIRE(members[8].type.typeClass == Hdf5TypeClass::Compound);
    REQUIRE(members[8].type.members.size() == 2);
    REQUIRE(members[8].type.members[1].offset == 2);

    const Hdf5Table table = readHdf5Table(directory.file("kinds.h5"), "/RESULTS/GROUP/TABLE");
    REQUIRE(openHdf5ObjectCount() == 0);
    REQUIRE(table.rowCount == 5);
    REQUIRE(table.columns.size() == 10); // the nested compound is flattened: P.A and P.B
    REQUIRE(valuesOf<std::int32_t>(*table.column("DOMAIN_ID")) == data.domain);
    REQUIRE(valuesOf<std::int64_t>(*table.column("ID")) == data.ids);
    REQUIRE(valuesOf<double>(*table.column("X")) == data.x);
    REQUIRE(valuesOf<float>(*table.column("F")) == data.f);
    REQUIRE(valuesOf<std::uint8_t>(*table.column("FLAG")) == data.flag);
    REQUIRE(table.column("FLAG")->numpyString() == "|i1");
    REQUIRE(table.column("NAME")->kind == Hdf5ColumnKind::FixedString);
    REQUIRE(table.column("NAME")->itemSize == 4);
    REQUIRE(valuesOf<char>(*table.column("NAME")) == data.names);
    REQUIRE(table.column("TITLE")->strings == data.titles);
    REQUIRE(table.column("VEC")->shape == std::vector<std::uint64_t>{3});
    REQUIRE(valuesOf<double>(*table.column("VEC")) == data.vec);
    REQUIRE(valuesOf<std::int16_t>(*table.column("P.A")) == data.a);
    REQUIRE(valuesOf<std::uint8_t>(*table.column("P.B")) == data.b);
}

TEST_CASE("rows, columns and DOMAIN_ID select without reading the rest", "[io][hdf5]")
{
    const TemporaryDirectory directory("oor_h5_generic_select");
    const SampleData data;
    Hdf5WriteRequest request;
    request.datasets.push_back(data.dataset("/T"));
    writeHdf5(directory.file("select.h5"), request);

    Hdf5ReadOptions options;
    options.rows = std::make_pair(std::uint64_t{1}, std::uint64_t{4});
    options.columns = {"X", "TITLE", "ID"};
    Hdf5Table table = readHdf5Table(directory.file("select.h5"), "/T", options);
    REQUIRE(table.rowCount == 3);
    REQUIRE(table.columns.size() == 3);
    REQUIRE(table.columns[0].name == "X"); // the order of the request
    REQUIRE(table.columns[1].name == "TITLE");
    REQUIRE(valuesOf<double>(table.columns[0]) == std::vector<double>{1.5, 2.5, 3.5});
    REQUIRE(table.columns[1].strings == std::vector<std::string>{"two", "three", ""});
    REQUIRE(valuesOf<std::int64_t>(table.columns[2]) == std::vector<std::int64_t>{11, 12, 13});

    options = {};
    options.domainId = 2;
    options.columns = {"ID", "NAME"};
    table = readHdf5Table(directory.file("select.h5"), "/T", options);
    REQUIRE(table.rowCount == 3);
    REQUIRE(valuesOf<std::int64_t>(*table.column("ID")) == std::vector<std::int64_t>{12, 13, 14});
    REQUIRE(table.sourceRuns == std::vector<std::pair<std::uint64_t, std::uint64_t>>{{2, 5}});

    options.domainId = 7;
    table = readHdf5Table(directory.file("select.h5"), "/T", options);
    REQUIRE(table.rowCount == 0);
    REQUIRE(table.columns.size() == 2);
    REQUIRE(table.columns[0].data.size() == 0);

    options = {};
    options.columns = {"P"}; // a nested compound stands for its leaves
    table = readHdf5Table(directory.file("select.h5"), "/T", options);
    REQUIRE(table.columns.size() == 2);
    REQUIRE(table.columns[0].name == "P.A");

    options = {};
    options.rows = std::make_pair(std::uint64_t{3}, std::uint64_t{100}); // clamped
    options.domainId = 2;
    options.columns = {"DOMAIN_ID"};
    table = readHdf5Table(directory.file("select.h5"), "/T", options);
    REQUIRE(valuesOf<std::int32_t>(table.columns[0]) == std::vector<std::int32_t>{2, 2});
    REQUIRE(table.sourceRuns == std::vector<std::pair<std::uint64_t, std::uint64_t>>{{3, 5}});
    REQUIRE(openHdf5ObjectCount() == 0);
}

TEST_CASE("a table with several runs of one domain, in several blocks", "[io][hdf5]")
{
    const TemporaryDirectory directory("oor_h5_generic_runs");
    constexpr std::uint64_t ROWS = 100'000;
    std::vector<std::int64_t> domain(ROWS);
    std::vector<double> value(ROWS);
    for (std::uint64_t row = 0; row < ROWS; ++row)
    {
        domain[row] = (row / 7) % 3 == 0 ? 5 : 6;
        value[row] = static_cast<double>(row) * 0.25;
    }
    Hdf5WriteDataset dataset;
    dataset.path = "/BIG";
    dataset.columns.push_back(column("DOMAIN_ID", arrayOf(Hdf5ValueKind::Int64, domain, {ROWS})));
    dataset.columns.push_back(column("V", arrayOf(Hdf5ValueKind::Float64, value, {ROWS})));
    dataset.chunkRows = 4096;
    dataset.deflate = 3;
    dataset.shuffle = true;
    Hdf5WriteRequest request;
    request.datasets.push_back(std::move(dataset));
    writeHdf5(directory.file("runs.h5"), request);

    const Hdf5Tree tree = Hdf5Tree::read(directory.file("runs.h5"));
    const Hdf5Node& node = nodeAt(tree, "/BIG");
    REQUIRE(node.layout == "chunked");
    REQUIRE(node.chunks == std::vector<std::uint64_t>{4096});
    REQUIRE(node.filters.size() == 2);
    REQUIRE(node.filters[0].name == "shuffle");
    REQUIRE(node.filters[1].name == "deflate");
    REQUIRE(node.filters[1].parameters == std::vector<std::uint32_t>{3});
    REQUIRE(node.storageBytes > 0);
    REQUIRE(node.storageBytes < ROWS * 16);

    Hdf5ReadOptions options;
    options.domainId = 5;
    const Hdf5Table table = readHdf5Table(directory.file("runs.h5"), "/BIG", options);
    std::vector<double> expected;
    std::uint64_t runs = 0;
    for (std::uint64_t row = 0; row < ROWS; ++row)
    {
        if (domain[row] == 5)
        {
            expected.push_back(value[row]);
            runs += (row == 0 || domain[row - 1] != 5) ? 1 : 0;
        }
    }
    REQUIRE(table.rowCount == expected.size());
    REQUIRE(table.sourceRuns.size() == runs);
    REQUIRE(valuesOf<double>(*table.column("V")) == expected);

    options = {};
    options.rows = std::make_pair(std::uint64_t{5000}, std::uint64_t{9000});
    options.columns = {"V"};
    const Hdf5Table part = readHdf5Table(directory.file("runs.h5"), "/BIG", options);
    REQUIRE(valuesOf<double>(part.columns[0]) == std::vector<double>(value.begin() + 5000, value.begin() + 9000));
    REQUIRE(openHdf5ObjectCount() == 0);
}

TEST_CASE("plain datasets of every numeric kind, scalars, empty and multi-dimensional", "[io][hdf5]")
{
    const TemporaryDirectory directory("oor_h5_generic_plain");
    const std::vector<std::int8_t> i8{-1, 2, -3, 4, -5, 6};
    const std::vector<std::int16_t> i16{-1, 2, -3, 4, -5, 6};
    const std::vector<std::int32_t> i32{-1, 2, -3, 4, -5, 6};
    const std::vector<std::int64_t> i64{-1, 2, -3, 4, -5, 6};
    const std::vector<std::uint8_t> u8{1, 2, 3, 4, 5, 6};
    const std::vector<std::uint16_t> u16{1, 2, 3, 4, 5, 6};
    const std::vector<std::uint32_t> u32{1, 2, 3, 4, 5, 6};
    const std::vector<std::uint64_t> u64{1, 2, 3, 4, 5, std::uint64_t{1} << 63};
    const std::vector<float> f32{1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f};
    const std::vector<double> f64{1.5, 2.5, 3.5, 4.5, 5.5, 6.5};
    const std::vector<double> scalar{42.0};
    const std::vector<double> none;
    const std::vector<std::string> words{"a", "bb", "ccc", "", "eeeee", "f"};

    Hdf5WriteRequest request;
    const auto add = [&request](const std::string& name, Hdf5WriteArray array)
    {
        Hdf5WriteDataset dataset;
        dataset.path = "/plain/" + name;
        dataset.array = std::move(array);
        request.datasets.push_back(std::move(dataset));
    };
    add("i8", arrayOf(Hdf5ValueKind::Int8, i8, {2, 3}));
    add("i16", arrayOf(Hdf5ValueKind::Int16, i16, {2, 3}));
    add("i32", arrayOf(Hdf5ValueKind::Int32, i32, {2, 3}));
    add("i64", arrayOf(Hdf5ValueKind::Int64, i64, {2, 3}));
    add("u8", arrayOf(Hdf5ValueKind::UInt8, u8, {6}));
    add("u16", arrayOf(Hdf5ValueKind::UInt16, u16, {6}));
    add("u32", arrayOf(Hdf5ValueKind::UInt32, u32, {6}));
    add("u64", arrayOf(Hdf5ValueKind::UInt64, u64, {6}));
    add("f32", arrayOf(Hdf5ValueKind::Float32, f32, {3, 2}));
    add("f64", arrayOf(Hdf5ValueKind::Float64, f64, {3, 2}));
    add("scalar", arrayOf(Hdf5ValueKind::Float64, scalar, {}));
    add("empty", arrayOf(Hdf5ValueKind::Float64, none, {0}));
    add("empty2d", arrayOf(Hdf5ValueKind::Float64, none, {0, 4}));
    add("words", stringsOf(words, {6}));
    writeHdf5(directory.file("plain.h5"), request);

    const Hdf5Tree tree = Hdf5Tree::read(directory.file("plain.h5"));
    REQUIRE(nodeAt(tree, "/plain/i8").type->description() == "int8");
    REQUIRE(nodeAt(tree, "/plain/u64").type->description() == "uint64");
    REQUIRE(nodeAt(tree, "/plain/f32").type->numpyString() == "<f4");
    REQUIRE(nodeAt(tree, "/plain/i8").shape == std::vector<std::uint64_t>{2, 3});
    REQUIRE(nodeAt(tree, "/plain/i8").maxShape == std::vector<std::uint64_t>{2, 3});
    REQUIRE(nodeAt(tree, "/plain/scalar").shape.empty());
    REQUIRE(nodeAt(tree, "/plain/empty").shape == std::vector<std::uint64_t>{0});
    REQUIRE(nodeAt(tree, "/plain/empty").elementCount() == 0);
    REQUIRE(tree.datasetCount() == 14);

    const auto read = [&directory](const std::string& name)
    { return readHdf5Table(directory.file("plain.h5"), "/plain/" + name); };
    REQUIRE(valuesOf<std::int8_t>(read("i8").columns[0]) == i8);
    REQUIRE(read("i8").columns[0].shape == std::vector<std::uint64_t>{3});
    REQUIRE(read("i8").rowCount == 2);
    REQUIRE(valuesOf<std::int16_t>(read("i16").columns[0]) == i16);
    REQUIRE(valuesOf<std::int32_t>(read("i32").columns[0]) == i32);
    REQUIRE(valuesOf<std::int64_t>(read("i64").columns[0]) == i64);
    REQUIRE(valuesOf<std::uint8_t>(read("u8").columns[0]) == u8);
    REQUIRE(valuesOf<std::uint16_t>(read("u16").columns[0]) == u16);
    REQUIRE(valuesOf<std::uint32_t>(read("u32").columns[0]) == u32);
    REQUIRE(valuesOf<std::uint64_t>(read("u64").columns[0]) == u64);
    REQUIRE(read("u64").columns[0].numpyString() == "<u8");
    REQUIRE(valuesOf<float>(read("f32").columns[0]) == f32);
    REQUIRE(valuesOf<double>(read("f64").columns[0]) == f64);
    REQUIRE(valuesOf<double>(read("scalar").columns[0]) == scalar);
    REQUIRE(read("scalar").rowCount == 1);
    REQUIRE(read("empty").rowCount == 0);
    REQUIRE(read("empty").columns[0].data.size() == 0);
    REQUIRE(read("empty2d").rowCount == 0);
    REQUIRE(read("words").columns[0].strings == words);
    Hdf5ReadOptions options;
    options.rows = std::make_pair(std::uint64_t{1}, std::uint64_t{3});
    const Hdf5Table rows = readHdf5Table(directory.file("plain.h5"), "/plain/f64", options);
    REQUIRE(valuesOf<double>(rows.columns[0]) == std::vector<double>{3.5, 4.5, 5.5, 6.5});
    REQUIRE(openHdf5ObjectCount() == 0);
}

TEST_CASE("attributes are listed with their types and small values", "[io][hdf5]")
{
    const TemporaryDirectory directory("oor_h5_generic_attributes");
    const std::vector<std::int64_t> one{7};
    const std::vector<std::int32_t> many{1, 2, 3};
    const std::vector<double> real{2.5};
    const std::vector<std::uint8_t> flags{1, 0};
    std::vector<char> text{'p', 'l', 'a', 't', 'e'};
    const std::vector<std::string> list{"x", "yy"};

    Hdf5WriteRequest request;
    Hdf5WriteGroup root;
    root.path = "/";
    root.attributes.push_back(attribute("COUNT", arrayOf(Hdf5ValueKind::Int64, one, {})));
    Hdf5WriteArray title = arrayOf(Hdf5ValueKind::FixedString, text, {});
    title.stringBytes = 5;
    root.attributes.push_back(attribute("TITLE", title));
    request.groups.push_back(std::move(root));
    Hdf5WriteDataset dataset;
    dataset.path = "/g/d";
    dataset.array = arrayOf(Hdf5ValueKind::Float64, real, {1});
    dataset.attributes.push_back(attribute("IDS", arrayOf(Hdf5ValueKind::Int32, many, {3})));
    dataset.attributes.push_back(attribute("SCALE", arrayOf(Hdf5ValueKind::Float64, real, {})));
    dataset.attributes.push_back(attribute("FLAGS", arrayOf(Hdf5ValueKind::Bool, flags, {2})));
    dataset.attributes.push_back(attribute("NAMES", stringsOf(list, {2})));
    request.datasets.push_back(std::move(dataset));
    writeHdf5(directory.file("attributes.h5"), request);

    const Hdf5Tree tree = Hdf5Tree::read(directory.file("attributes.h5"));
    const Hdf5Attribute* count = tree.root().attribute("COUNT");
    REQUIRE(count != nullptr);
    REQUIRE(count->shape.empty());
    REQUIRE(std::get<std::vector<std::int64_t>>(count->value) == std::vector<std::int64_t>{7});
    REQUIRE(std::get<std::vector<std::string>>(tree.root().attribute("TITLE")->value) ==
            std::vector<std::string>{"plate"});
    const Hdf5Node& node = nodeAt(tree, "/g/d");
    REQUIRE(node.attributes.size() == 4);
    REQUIRE(std::get<std::vector<std::int64_t>>(node.attribute("IDS")->value) == std::vector<std::int64_t>{1, 2, 3});
    REQUIRE(node.attribute("IDS")->shape == std::vector<std::uint64_t>{3});
    REQUIRE(std::get<std::vector<double>>(node.attribute("SCALE")->value) == std::vector<double>{2.5});
    REQUIRE(std::get<std::vector<std::int64_t>>(node.attribute("FLAGS")->value) == std::vector<std::int64_t>{1, 0});
    REQUIRE(node.attribute("FLAGS")->type.typeClass == Hdf5TypeClass::Enum);
    // variable-length strings live in the global heap, where libhdf5 can loop on a damaged file: not read unless asked
    REQUIRE(std::holds_alternative<std::monostate>(node.attribute("NAMES")->value));
    REQUIRE_FALSE(node.attribute("NAMES")->unreadReason.empty());
    REQUIRE(node.attribute("NAMES")->shape == std::vector<std::uint64_t>{2});
    Hdf5Limits withStrings;
    withStrings.readVarLenAttributes = true;
    const Hdf5Tree readStrings = Hdf5Tree::read(directory.file("attributes.h5"), withStrings);
    REQUIRE(std::get<std::vector<std::string>>(nodeAt(readStrings, "/g/d").attribute("NAMES")->value) == list);
    REQUIRE(tree.find("/g")->children == std::vector<std::uint32_t>{2});

    // a cap smaller than a value leaves the value unread, not the type
    Hdf5Limits limits;
    limits.maxAttributeBytes = 4;
    const Hdf5Tree capped = Hdf5Tree::read(directory.file("attributes.h5"), limits);
    const Hdf5Attribute* ids = nodeAt(capped, "/g/d").attribute("IDS");
    REQUIRE(std::holds_alternative<std::monostate>(ids->value));
    REQUIRE_FALSE(ids->unreadReason.empty());
    REQUIRE(ids->type.description() == "int32");
    REQUIRE(openHdf5ObjectCount() == 0);
}

TEST_CASE("links are listed and never followed; a loop of hard links ends", "[io][hdf5]")
{
    const TemporaryDirectory directory("oor_h5_generic_links");
    const std::vector<double> values{1, 2, 3};
    Hdf5WriteRequest request;
    Hdf5WriteDataset dataset;
    dataset.path = "/a/b/data";
    dataset.array = arrayOf(Hdf5ValueKind::Float64, values, {3});
    request.datasets.push_back(std::move(dataset));
    request.links.push_back({"/a/soft", "/a/b/data", "", false});
    request.links.push_back({"/a/dangling", "/nowhere", "", false});
    request.links.push_back({"/a/b/loop", "/a", "", true});
    request.links.push_back({"/a/alias", "/a/b/data", "", true});
    request.links.push_back({"/a/external", "/other/data", "elsewhere.h5", false});
    writeHdf5(directory.file("links.h5"), request);

    const Hdf5Tree tree = Hdf5Tree::read(directory.file("links.h5"));
    REQUIRE(nodeAt(tree, "/a/soft").kind == Hdf5NodeKind::SoftLink);
    REQUIRE(nodeAt(tree, "/a/soft").linkTarget == "/a/b/data");
    REQUIRE(nodeAt(tree, "/a/dangling").kind == Hdf5NodeKind::SoftLink);
    REQUIRE(nodeAt(tree, "/a/external").kind == Hdf5NodeKind::ExternalLink);
    REQUIRE(nodeAt(tree, "/a/external").linkTarget == "elsewhere.h5::/other/data");
    const Hdf5Node& loop = nodeAt(tree, "/a/b/loop");
    REQUIRE(loop.kind == Hdf5NodeKind::Group);
    REQUIRE(loop.aliasOf == "/a");
    REQUIRE(loop.children.empty());
    const Hdf5Node& alias = nodeAt(tree, "/a/alias");
    REQUIRE(alias.kind == Hdf5NodeKind::Dataset);
    REQUIRE(alias.aliasOf.empty()); // "alias" sorts before "b": it is the first listing of the dataset
    REQUIRE(nodeAt(tree, "/a/b/data").aliasOf == "/a/alias");
    REQUIRE(tree.datasetCount() == 2);
    REQUIRE(openHdf5ObjectCount() == 0);

    // a path through a soft link is read; a path through an external link is refused
    REQUIRE(readHdf5Table(directory.file("links.h5"), "/a/soft").rowCount == 3);
    REQUIRE_THROWS_AS(readHdf5Table(directory.file("links.h5"), "/a/external"), IoError);
    REQUIRE_THROWS_AS(readHdf5Table(directory.file("links.h5"), "/a/external/deeper"), IoError);
    {
        const Hdf5File file = Hdf5File::openReadOnly(directory.file("links.h5"));
        REQUIRE_THROWS_AS(file.openDataset("a/external"), IoError);
        REQUIRE_THROWS_AS(file.hasChild("a/external/x"), IoError);
        REQUIRE(file.hasChild("a/soft"));
    }
    REQUIRE(openHdf5ObjectCount() == 0);
}

TEST_CASE("resource limits are reported before the work they protect", "[io][hdf5]")
{
    const TemporaryDirectory directory("oor_h5_generic_limits");
    const std::vector<double> values(1000, 1.0);
    Hdf5WriteRequest request;
    for (int index = 0; index < 20; ++index)
    {
        Hdf5WriteDataset dataset;
        dataset.path = "/deep/g" + std::to_string(index) + "/d";
        dataset.array = arrayOf(Hdf5ValueKind::Float64, values, {1000});
        request.datasets.push_back(std::move(dataset));
    }
    Hdf5WriteDataset deep;
    deep.path = "/1/2/3/4/5/6/7/8";
    deep.array = arrayOf(Hdf5ValueKind::Float64, values, {1000});
    request.datasets.push_back(std::move(deep));
    writeHdf5(directory.file("limits.h5"), request);

    Hdf5Limits limits;
    limits.maxObjects = 10;
    REQUIRE_THROWS_AS(Hdf5Tree::read(directory.file("limits.h5"), limits), LimitExceededError);
    limits = {};
    limits.maxDepth = 4;
    REQUIRE_THROWS_AS(Hdf5Tree::read(directory.file("limits.h5"), limits), LimitExceededError);
    REQUIRE_THROWS_AS(writeHdf5(directory.file("never.h5"), request, limits), LimitExceededError); // the path of `deep`
    REQUIRE_FALSE(std::filesystem::exists(directory.file("never.h5")));

    Hdf5ReadOptions options;
    options.limits.maxReadBytes = 4000;
    REQUIRE_THROWS_AS(readHdf5Table(directory.file("limits.h5"), "/deep/g0/d", options), LimitExceededError);
    options.rows = std::make_pair(std::uint64_t{0}, std::uint64_t{400});
    REQUIRE(readHdf5Table(directory.file("limits.h5"), "/deep/g0/d", options).rowCount == 400);
    options = {};
    options.limits.maxReadRows = 999;
    REQUIRE_THROWS_AS(readHdf5Table(directory.file("limits.h5"), "/deep/g0/d", options), LimitExceededError);

    // an attribute that is too big to store is refused before anything is written
    std::vector<char> big(70'000, 'x');
    Hdf5WriteDataset dataset;
    dataset.path = "/d";
    dataset.array = arrayOf(Hdf5ValueKind::Float64, values, {1000});
    Hdf5WriteArray text = arrayOf(Hdf5ValueKind::FixedString, big, {});
    text.stringBytes = big.size();
    dataset.attributes.push_back(attribute("BIG", text));
    Hdf5WriteRequest bigRequest;
    bigRequest.datasets.push_back(std::move(dataset));
    REQUIRE_THROWS_AS(writeHdf5(directory.file("never.h5"), bigRequest), LimitExceededError);
    REQUIRE_FALSE(std::filesystem::exists(directory.file("never.h5")));
    REQUIRE(openHdf5ObjectCount() == 0);
}

TEST_CASE("requests that make no sense and reads that cannot be made are errors, with nothing left open", "[io][hdf5]")
{
    const TemporaryDirectory directory("oor_h5_generic_errors");
    const SampleData data;
    const std::vector<double> values{1, 2, 3};
    {
        Hdf5WriteRequest request;
        request.datasets.push_back(data.dataset("/T"));
        Hdf5WriteDataset plain;
        plain.path = "/G/plain";
        plain.array = arrayOf(Hdf5ValueKind::Float64, values, {3});
        request.datasets.push_back(std::move(plain));
        writeHdf5(directory.file("ok.h5"), request);
    }
    const auto path = directory.file("ok.h5");
    REQUIRE_THROWS_AS(readHdf5Table(path, "/missing"), IoError);
    REQUIRE_THROWS_AS(readHdf5Table(path, "/G"), IoError); // a group
    REQUIRE_THROWS_AS(readHdf5Table(directory.file("absent.h5"), "/T"), IoError);
    Hdf5ReadOptions options;
    options.columns = {"NOPE"};
    REQUIRE_THROWS_AS(readHdf5Table(path, "/T", options), UsageError);
    options = {};
    options.domainId = 1;
    options.domainColumn = "ID_MISSING";
    REQUIRE_THROWS_AS(readHdf5Table(path, "/T", options), UsageError);
    options.domainColumn = "X"; // a float
    REQUIRE_THROWS_AS(readHdf5Table(path, "/T", options), UsageError);
    options = {};
    options.rows = std::make_pair(std::uint64_t{4}, std::uint64_t{2});
    REQUIRE_THROWS_AS(readHdf5Table(path, "/T", options), UsageError);

    // requests that disagree with themselves
    {
        Hdf5WriteRequest request;
        Hdf5WriteDataset dataset = data.dataset("/T");
        dataset.columns[1].array.shape = {4}; // 4 rows against 5
        request.datasets.push_back(std::move(dataset));
        REQUIRE_THROWS_AS(writeHdf5(directory.file("bad.h5"), request), UsageError);
    }
    {
        Hdf5WriteRequest request;
        Hdf5WriteDataset dataset;
        dataset.path = "relative/path";
        dataset.array = arrayOf(Hdf5ValueKind::Float64, values, {3});
        request.datasets.push_back(dataset);
        REQUIRE_THROWS_AS(writeHdf5(directory.file("bad.h5"), request), UsageError);
        request.datasets[0].path = "/d";
        request.datasets[0].deflate = 12;
        REQUIRE_THROWS_AS(writeHdf5(directory.file("bad.h5"), request), UsageError);
        request.datasets[0].deflate = 0;
        request.datasets.push_back(request.datasets[0]);
        REQUIRE_THROWS_AS(writeHdf5(directory.file("bad.h5"), request), UsageError); // twice at /d
        request.datasets.pop_back();
        request.datasets[0].array.data = nullptr;
        REQUIRE_THROWS_AS(writeHdf5(directory.file("bad.h5"), request), UsageError);
    }
    // a dataset below a dataset: the library refuses, as an IoError, and no file is left
    {
        Hdf5WriteRequest request;
        Hdf5WriteDataset first;
        first.path = "/d";
        first.array = arrayOf(Hdf5ValueKind::Float64, values, {3});
        Hdf5WriteDataset second = first;
        second.path = "/d/e";
        request.datasets = {first, second};
        REQUIRE_THROWS_AS(writeHdf5(directory.file("bad.h5"), request), IoError);
    }
    REQUIRE_FALSE(std::filesystem::exists(directory.file("bad.h5")));
    REQUIRE(directory.fileCount() == 1);
    REQUIRE(openHdf5ObjectCount() == 0);
}

TEST_CASE("a write that fails or is cancelled leaves the earlier file and no temporary file", "[io][hdf5]")
{
    const TemporaryDirectory directory("oor_h5_generic_atomic");
    const std::vector<double> first{1, 2, 3};
    const std::vector<double> second{4, 5, 6, 7};
    Hdf5WriteRequest request;
    Hdf5WriteDataset dataset;
    dataset.path = "/d";
    dataset.array = arrayOf(Hdf5ValueKind::Float64, first, {3});
    request.datasets.push_back(dataset);
    writeHdf5(directory.file("target.h5"), request);

    request.datasets[0].array = arrayOf(Hdf5ValueKind::Float64, second, {4});
    oor::base::CancellationToken token;
    token.requestCancellation();
    REQUIRE_THROWS_AS(writeHdf5(directory.file("target.h5"), request, {}, &token), OperationCancelled);
    REQUIRE(directory.fileCount() == 1);
    REQUIRE(readHdf5Table(directory.file("target.h5"), "/d").rowCount == 3);

    writeHdf5(directory.file("target.h5"), request); // replaces
    REQUIRE(readHdf5Table(directory.file("target.h5"), "/d").rowCount == 4);
    REQUIRE(directory.fileCount() == 1);

    // a read is cancelled the same way
    Hdf5ReadOptions options;
    options.token = &token;
    REQUIRE_THROWS_AS(readHdf5Table(directory.file("target.h5"), "/d", options), OperationCancelled);
    REQUIRE_THROWS_AS(Hdf5Tree::read(directory.file("target.h5"), {}, &token), OperationCancelled);
    REQUIRE(openHdf5ObjectCount() == 0);
}

TEST_CASE("progress counts the rows written and read", "[io][hdf5]")
{
    const TemporaryDirectory directory("oor_h5_generic_progress");
    const SampleData data;
    Hdf5WriteRequest request;
    request.datasets.push_back(data.dataset("/T"));
    oor::base::ProgressCounter counter;
    writeHdf5(directory.file("progress.h5"), request, {}, nullptr, &counter);
    auto state = counter.snapshot();
    REQUIRE(state.stage == "write_h5");
    REQUIRE(state.total == 5);
    REQUIRE(state.done == 5);
    Hdf5ReadOptions options;
    options.progress = &counter;
    (void)readHdf5Table(directory.file("progress.h5"), "/T", options);
    state = counter.snapshot();
    REQUIRE(state.stage == "read_h5_table");
    REQUIRE(state.done == 5);
}

TEST_CASE("a damaged or truncated file is an error or a result, never a crash", "[io][hdf5]")
{
    const TemporaryDirectory directory("oor_h5_generic_damage");
    const SampleData data;
    Hdf5WriteRequest request;
    request.datasets.push_back(data.dataset("/RESULTS/T"));
    writeHdf5(directory.file("whole.h5"), request);
    std::ifstream input(directory.file("whole.h5"), std::ios::binary);
    const std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();
    REQUIRE(content.size() > 1000);

    const auto attempt = [&directory](const std::string& bytes)
    {
        {
            std::ofstream output(directory.file("damaged.h5"), std::ios::binary | std::ios::trunc);
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        try
        {
            (void)Hdf5Tree::read(directory.file("damaged.h5"));
            (void)readHdf5Table(directory.file("damaged.h5"), "/RESULTS/T");
        }
        catch (const oor::base::OorError&)
        {
        }
    };
    for (std::size_t length = 0; length < content.size(); length += 211)
        attempt(content.substr(0, length));
    for (std::size_t position = 0; position < content.size(); position += 101)
    {
        std::string damaged = content;
        damaged[position] = static_cast<char>(damaged[position] ^ 0xFF);
        attempt(damaged);
    }
    REQUIRE(openHdf5ObjectCount() == 0);
}

TEST_CASE("a damaged global heap is refused before libhdf5 reads a variable-length value", "[io][hdf5][robustness]")
{
    const TemporaryDirectory directory("oor_h5_generic_heap");
    const std::vector<std::string> words{"alpha", "beta", "gamma"};
    const std::vector<double> values{1.0};
    Hdf5WriteRequest request;
    Hdf5WriteDataset text;
    text.path = "/words";
    text.array = stringsOf(words, {3});
    request.datasets.push_back(std::move(text));
    Hdf5WriteDataset plain;
    plain.path = "/plain";
    plain.array = arrayOf(Hdf5ValueKind::Float64, values, {1});
    plain.attributes.push_back(attribute("NAMES", stringsOf(words, {3})));
    request.datasets.push_back(std::move(plain));
    writeHdf5(directory.file("sound.h5"), request);
    std::ifstream input(directory.file("sound.h5"), std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();
    const auto bytesOf = [](const std::string& data)
    { return std::span<const std::byte>(reinterpret_cast<const std::byte*>(data.data()), data.size()); };
    REQUIRE_FALSE(findGlobalHeapProblem(bytesOf(content)).has_value());
    REQUIRE(readHdf5Table(directory.file("sound.h5"), "/words").columns[0].strings == words);

    // The size of the first object of the collection (GCOL + 24) changed: libhdf5 never returns from reading it.
    const std::size_t heap = content.find("GCOL");
    REQUIRE(heap != std::string::npos);
    std::string damaged = content;
    damaged[heap + 24] = static_cast<char>(damaged[heap + 24] ^ 0xFF);
    {
        std::ofstream output(directory.file("damaged.h5"), std::ios::binary | std::ios::trunc);
        output.write(damaged.data(), static_cast<std::streamsize>(damaged.size()));
    }
    REQUIRE(findGlobalHeapProblem(directory.file("damaged.h5")).has_value());
    Hdf5Limits withStrings;
    withStrings.readVarLenAttributes = true;
    const Hdf5Tree tree = Hdf5Tree::read(directory.file("damaged.h5"), withStrings);
    const Hdf5Attribute* names = nodeAt(tree, "/plain").attribute("NAMES");
    REQUIRE(std::holds_alternative<std::monostate>(names->value));
    REQUIRE(names->unreadReason.find("global heap") != std::string::npos);
    REQUIRE_THROWS_AS(readHdf5Table(directory.file("damaged.h5"), "/words"), IoError);
    // the other columns and datasets are read as before
    REQUIRE(readHdf5Table(directory.file("damaged.h5"), "/plain").rowCount == 1);
    REQUIRE(openHdf5ObjectCount() == 0);

    // The other ways a collection can stop the walk of libhdf5: a free-space object of size 0, an object or a
    // collection that runs past its end.
    const auto withLength = [&](std::size_t at, std::uint64_t value)
    {
        std::string changed = content;
        for (std::size_t index = 0; index < 8; ++index)
            changed[at + index] = static_cast<char>((value >> (8 * index)) & 0xFF);
        return findGlobalHeapProblem(bytesOf(changed));
    };
    REQUIRE(withLength(heap + 8, content.size()).has_value());          // the collection is longer than the file
    REQUIRE(withLength(heap + 8, 4).has_value());                       // shorter than its header
    REQUIRE(withLength(heap + 24, std::uint64_t{1} << 62).has_value()); // an object of 2^62 bytes
    std::string zeroFree = content;
    const std::size_t firstObject = heap + 16;
    std::size_t freeObject = firstObject;
    // walk to the free-space object (index 0) and make its size 0
    for (int step = 0; step < 16; ++step)
    {
        const auto index = static_cast<unsigned char>(content[freeObject]) |
                           (static_cast<unsigned char>(content[freeObject + 1]) << 8);
        if (index == 0)
            break;
        std::uint64_t size = 0;
        for (std::size_t byte = 0; byte < 8; ++byte)
            size |= static_cast<std::uint64_t>(static_cast<unsigned char>(content[freeObject + 8 + byte]))
                    << (8 * byte);
        freeObject += 16 + (size + 7) / 8 * 8;
    }
    REQUIRE(withLength(freeObject + 8, 0).has_value());
}
