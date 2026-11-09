#include "oor/query/table/Hdf5Export.hpp"

#include "oor/base/Version.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/io/hdf5/Hdf5Writer.hpp"
#include "oor/query/table/PlainColumn.hpp"

#include <cctype>
#include <deque>
#include <set>

namespace oor::query
{

namespace
{

using io::Hdf5ValueKind;
using io::Hdf5WriteAttribute;
using io::Hdf5WriteDataset;
using io::Hdf5WriteGroup;
using io::Hdf5WriteRequest;

constexpr std::uint64_t SMALLEST_COMPRESSED = 1024;

bool isSafeName(const std::string& name)
{
    if (name.empty())
        return false;
    for (const char character : name)
        if (std::isalnum(static_cast<unsigned char>(character)) == 0 && character != '_' && character != '-')
            return false;
    return true;
}

// The name of the dataset of a column: the column name, with the characters HDF5 gives a meaning to replaced.
std::string datasetName(const std::string& column)
{
    std::string name = column;
    for (char& character : name)
        if (character == '/')
            character = '_';
    return name == "." || name.empty() ? "_" : name;
}

// Keeps the vectors and strings a request borrows until it is written.
struct Storage
{
    std::deque<std::vector<std::int64_t>> integers;
    std::deque<std::vector<double>> reals;
    std::deque<std::vector<std::uint8_t>> flags;
    std::deque<std::string> texts;
};

Hdf5WriteAttribute textAttribute(Storage& storage, const std::string& name, const std::string& value)
{
    const std::string& kept = storage.texts.emplace_back(value.empty() ? std::string(1, '\0') : value);
    Hdf5WriteAttribute attribute;
    attribute.name = name;
    attribute.array.kind = Hdf5ValueKind::FixedString;
    attribute.array.utf8 = true;
    attribute.array.stringBytes = kept.size();
    attribute.array.data = kept.data();
    return attribute;
}

void compress(Hdf5WriteDataset& dataset, int level)
{
    if (level > 0 && dataset.array.elementCount() >= SMALLEST_COMPRESSED)
    {
        dataset.deflate = level;
        dataset.shuffle = true;
    }
}

void addNumbers(Hdf5WriteRequest& request, Storage& storage, const std::string& path, const PlainColumn& column,
                int level)
{
    Hdf5WriteDataset dataset;
    dataset.path = path;
    dataset.array.shape = {column.numbers.size()};
    if (column.integer)
    {
        std::vector<std::int64_t>& values = storage.integers.emplace_back();
        values.reserve(column.numbers.size());
        for (const double number : column.numbers)
            values.push_back(number == number ? static_cast<std::int64_t>(number) : 0); // a null becomes 0 (see .valid)
        dataset.array.kind = Hdf5ValueKind::Int64;
        dataset.array.data = values.data();
    }
    else
    {
        dataset.array.kind = Hdf5ValueKind::Float64;
        dataset.array.data = column.numbers.data();
    }
    compress(dataset, level);
    request.datasets.push_back(std::move(dataset));
}

void addTexts(Hdf5WriteRequest& request, const std::string& path, const std::vector<std::string>& texts)
{
    Hdf5WriteDataset dataset;
    dataset.path = path;
    dataset.array.kind = Hdf5ValueKind::VarString;
    dataset.array.shape = {texts.size()};
    dataset.array.strings = texts;
    request.datasets.push_back(std::move(dataset));
}

} // namespace

void writeQueryTables(const std::filesystem::path& path, const std::vector<NamedQueryTable>& tables,
                      int compressionLevel)
{
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path()); // the folder of the file, when it is missing
    if (compressionLevel < 0 || compressionLevel > 9)
        throw base::UsageError("the compression level is " + std::to_string(compressionLevel) + ", it must be 0 to 9");
    std::set<std::string> names;
    for (const NamedQueryTable& named : tables)
    {
        if (!isSafeName(named.name))
            throw base::UsageError("the table name '" + named.name +
                                   "' must hold only letters, digits, '_' and '-' (it names an HDF5 group)");
        if (!names.insert(named.name).second)
            throw base::UsageError("the table name '" + named.name + "' is given twice");
        if (!named.table)
            throw base::UsageError("the table '" + named.name + "' is empty (no table given)");
    }

    Storage storage;
    Hdf5WriteRequest request;
    Hdf5WriteGroup root;
    root.path = "/";
    root.attributes.push_back(textAttribute(storage, "format", "oor-query-tables"));
    root.attributes.push_back(textAttribute(storage, "formatVersion", "1"));
    root.attributes.push_back(
        textAttribute(storage, "producer", std::string(base::LIBRARY_NAME) + " " + std::string(base::LIBRARY_VERSION)));
    request.groups.push_back(std::move(root));
    request.groups.push_back(Hdf5WriteGroup{"/tables", {}});

    // the plain columns must outlive the request, which borrows their vectors
    std::deque<PlainColumn> columns;
    for (const NamedQueryTable& named : tables)
    {
        const std::string group = "/tables/" + named.name;
        Hdf5WriteGroup tableGroup;
        tableGroup.path = group;
        tableGroup.attributes.push_back(textAttribute(storage, "rowCount", std::to_string(named.table->rowCount())));
        request.groups.push_back(std::move(tableGroup));
        addTexts(request, group + "/columnNames", named.table->columnNames());
        request.groups.push_back(Hdf5WriteGroup{group + "/columns", {}});
        for (std::size_t index = 0; index < named.table->columnCount(); ++index)
        {
            const PlainColumn& column = columns.emplace_back(plainColumn(*named.table, index));
            const std::string base = group + "/columns/" + datasetName(column.name);
            if (column.kind == PlainKind::Text || column.kind == PlainKind::TextList)
                addTexts(request, base, column.texts);
            else
                addNumbers(request, storage, base, column, compressionLevel);
            if (column.kind == PlainKind::NumberList || column.kind == PlainKind::TextList)
            {
                Hdf5WriteDataset offsets;
                offsets.path = base + ".offsets";
                offsets.array.kind = Hdf5ValueKind::Int64;
                offsets.array.shape = {column.offsets.size()};
                offsets.array.data = column.offsets.data();
                compress(offsets, compressionLevel);
                request.datasets.push_back(std::move(offsets));
            }
            if (!column.valid.empty())
            {
                Hdf5WriteDataset valid;
                valid.path = base + ".valid";
                valid.array.kind = Hdf5ValueKind::UInt8;
                valid.array.shape = {column.valid.size()};
                valid.array.data = column.valid.data();
                compress(valid, compressionLevel);
                request.datasets.push_back(std::move(valid));
            }
        }
    }
    io::writeHdf5(path, request);
}

} // namespace oor::query
