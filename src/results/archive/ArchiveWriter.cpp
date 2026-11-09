#include "oor/results/archive/ArchiveWriter.hpp"

#include "oor/base/Version.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/io/hdf5/Hdf5StreamWriter.hpp"
#include "oor/io/hdf5/Hdf5Writer.hpp"
#include "oor/results/archive/ArchiveFormat.hpp"

#include <cstdio>
#include <ctime>
#include <deque>
#include <filesystem>
#include <limits>
#include <system_error>
#include <utility>
#include <vector>

namespace oor::results
{

namespace
{

using io::Hdf5ValueKind;
using io::Hdf5WriteArray;
using io::Hdf5WriteAttribute;
using io::Hdf5WriteColumn;
using io::Hdf5WriteDataset;
using io::Hdf5WriteGroup;
using io::Hdf5WriteRequest;

// Datasets smaller than this many values are written contiguous and uncompressed: a filter costs more than it saves on
// a few hundred numbers.
constexpr std::uint64_t SMALLEST_COMPRESSED = 1024;

std::string currentTimeUtc()
{
    const std::time_t now = std::time(nullptr);
    std::tm parts{};
#if defined(_WIN32)
    gmtime_s(&parts, &now);
#else
    gmtime_r(&now, &parts);
#endif
    char text[32];
    std::strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", &parts);
    return text;
}

// "/data/tables/000042": the zero padding keeps the groups in the order they were written in any HDF5 viewer.
std::string groupPath(std::string_view parent, std::size_t number)
{
    char digits[16];
    std::snprintf(digits, sizeof(digits), "%06zu", number);
    return std::string(parent) + "/" + digits;
}

// One row of the index of the tables, kept until `finish` writes the index.
struct TableIndexRow
{
    std::string group, kind, variant, container, layout, tableName, stepKind, valueType, storedFormat, status,
        storedFrame, units, title, subtitle, label;
    std::int64_t subcase = 0, analysisCode = 0, superelement = -1, designCycle = -1, loadSet = 0, sort = 1,
                 frameSystem = 0, wordSize = 4, stepCount = 0, rowCount = 0, componentCount = 0, stepOffset = 0,
                 hasEigenvalues = 0, hasEigenvaluesImag = 0, hasCycles = 0;
};

struct NamedIndexRow
{
    std::string group, kind, name, tableName, status, title, subtitle, label;
    std::int64_t subcase = 0, analysisCode = 0, superelement = -1, designCycle = -1, wordSize = 4;
};

// Builds the datasets of one request. The arrays of a request borrow their memory: the strings and vectors made here
// are kept in this object until the request is written.
class RequestBuilder
{
public:
    explicit RequestBuilder(const ArchiveWriteOptions& options) : m_options(options) {}

    [[nodiscard]] Hdf5WriteRequest& request() noexcept { return m_request; }

    void numbers(const std::string& path, Hdf5ValueKind kind, std::vector<std::uint64_t> shape, const void* data)
    {
        Hdf5WriteDataset dataset;
        dataset.path = path;
        dataset.array.kind = kind;
        dataset.array.shape = std::move(shape);
        dataset.array.data = data;
        compressIfWorth(dataset);
        m_request.datasets.push_back(std::move(dataset));
    }

    void texts(const std::string& path, std::vector<std::string> values, std::vector<std::uint64_t> shape = {})
    {
        Hdf5WriteDataset dataset;
        dataset.path = path;
        dataset.array.kind = Hdf5ValueKind::VarString;
        dataset.array.shape = shape.empty() ? std::vector<std::uint64_t>{values.size()} : std::move(shape);
        dataset.array.strings = std::move(values);
        m_request.datasets.push_back(std::move(dataset));
    }

    // A vector of numbers kept by the builder until the request is written.
    template <typename T>
    const T* keep(std::vector<T> values)
    {
        auto kept = std::make_shared<std::vector<T>>(std::move(values));
        m_keepers.push_back(kept);
        return kept->data();
    }

    void textAttribute(Hdf5WriteGroup& group, std::string_view name, const std::string& value)
    {
        const std::string& kept = m_strings.emplace_back(value.empty() ? std::string(1, '\0') : value);
        Hdf5WriteAttribute attribute;
        attribute.name = std::string(name);
        attribute.array.kind = Hdf5ValueKind::FixedString;
        attribute.array.utf8 = true;
        attribute.array.stringBytes = kept.size();
        attribute.array.data = kept.data();
        group.attributes.push_back(std::move(attribute));
    }

private:
    void compressIfWorth(Hdf5WriteDataset& dataset) const
    {
        if (m_options.compressionLevel <= 0 || dataset.array.shape.empty() ||
            dataset.array.elementCount() < SMALLEST_COMPRESSED)
            return;
        dataset.deflate = m_options.compressionLevel;
        dataset.shuffle = m_options.shuffle;
    }

    const ArchiveWriteOptions& m_options;
    Hdf5WriteRequest m_request;
    std::deque<std::string> m_strings;
    std::vector<std::shared_ptr<const void>> m_keepers;
};

Hdf5ValueKind realKindOf(ValueType type)
{
    return type == ValueType::Float32 || type == ValueType::Complex64 ? Hdf5ValueKind::Float32 : Hdf5ValueKind::Float64;
}

Hdf5ValueKind arrayKindOf(ArrayType type)
{
    switch (type)
    {
    case ArrayType::Int32:
        return Hdf5ValueKind::Int32;
    case ArrayType::Int64:
        return Hdf5ValueKind::Int64;
    case ArrayType::Float32:
    case ArrayType::Complex64:
        return Hdf5ValueKind::Float32;
    case ArrayType::Float64:
    case ArrayType::Complex128:
        return Hdf5ValueKind::Float64;
    }
    return Hdf5ValueKind::Float64;
}

bool isComplexArray(ArrayType type)
{
    return type == ArrayType::Complex64 || type == ArrayType::Complex128;
}

// The text labels of a table or a named result: their names, then texts/<i>/dictionary and texts/<i>/codes.
void addTexts(RequestBuilder& builder, const std::string& group, const std::vector<ResultText>& texts)
{
    if (texts.empty())
        return;
    std::vector<std::string> names;
    for (std::size_t index = 0; index < texts.size(); ++index)
    {
        const ResultText& text = texts[index];
        names.push_back(text.name);
        const std::string folder = group + "/" + std::string(archive::TEXTS) + "/" + std::to_string(index);
        builder.texts(folder + "/" + std::string(archive::DICTIONARY), text.dictionary);
        builder.numbers(folder + "/" + std::string(archive::CODES), Hdf5ValueKind::UInt32, {text.codes.size()},
                        text.codes.data());
    }
    builder.texts(group + "/" + std::string(archive::TEXT_NAMES), std::move(names));
}

// A column of the index: texts or integers taken from the rows by a member pointer.
template <typename Row>
Hdf5WriteColumn textColumn(const std::vector<Row>& rows, const char* name, std::string Row::* member)
{
    Hdf5WriteColumn column;
    column.name = name;
    column.array.kind = Hdf5ValueKind::VarString;
    column.array.shape = {rows.size()};
    for (const Row& row : rows)
        column.array.strings.push_back(row.*member);
    return column;
}

template <typename Row>
Hdf5WriteColumn integerColumn(const std::vector<Row>& rows, const char* name, std::int64_t Row::* member,
                              std::deque<std::vector<std::int64_t>>& storage)
{
    std::vector<std::int64_t>& values = storage.emplace_back();
    for (const Row& row : rows)
        values.push_back(row.*member);
    Hdf5WriteColumn column;
    column.name = name;
    column.array.kind = Hdf5ValueKind::Int64;
    column.array.shape = {rows.size()};
    column.array.data = values.data();
    return column;
}

class ArchiveSession final : public ResultWriteSession
{
public:
    ArchiveSession(std::filesystem::path target, ArchiveWriteOptions options)
        : m_target(std::move(target)), m_temporary(m_target), m_options(std::move(options))
    {
        if (m_options.compressionLevel < 0 || m_options.compressionLevel > 9)
            throw base::UsageError("archive: the compression level is " + std::to_string(m_options.compressionLevel) +
                                   ", it must be 0 to 9");
        m_temporary += ".oor_tmp";
        if (m_target.has_parent_path())
            std::filesystem::create_directories(m_target.parent_path()); // the folder of the file, when it is missing
        m_report.path = m_target.string();
        m_report.format = "archive";

        // the file starts with its root attributes and its empty groups; the tables are added by `push`
        RequestBuilder builder(m_options);
        Hdf5WriteGroup root;
        root.path = "/";
        const std::string producer = m_options.producer.empty()
                                         ? std::string(base::LIBRARY_NAME) + " " + std::string(base::LIBRARY_VERSION)
                                         : m_options.producer;
        builder.textAttribute(root, archive::ATTRIBUTE_FORMAT, std::string(archive::FORMAT_NAME));
        builder.textAttribute(root, archive::ATTRIBUTE_FORMAT_VERSION, std::string(archive::FORMAT_VERSION));
        builder.textAttribute(root, archive::ATTRIBUTE_PRODUCER, producer);
        builder.textAttribute(root, archive::ATTRIBUTE_CREATED_AT, currentTimeUtc());
        builder.textAttribute(root, archive::ATTRIBUTE_TITLE, m_options.title);
        builder.textAttribute(root, archive::ATTRIBUTE_SUBTITLE, m_options.subtitle);
        builder.textAttribute(root, archive::ATTRIBUTE_LABEL, m_options.label);
        builder.request().groups.push_back(std::move(root));
        for (const std::string_view path : {std::string_view("/index"), archive::TABLE_GROUPS, archive::NAMED_GROUPS})
            builder.request().groups.push_back(Hdf5WriteGroup{std::string(path), {}});
        io::writeHdf5(m_temporary, builder.request());
    }

    ~ArchiveSession() override
    {
        if (!m_finished)
            abort();
    }

    void push(const ResultTables& tables) override
    {
        for (const std::shared_ptr<ResultTable>& table : tables.all())
            push(table);
        for (const std::shared_ptr<NamedResult>& named : tables.namedResults())
            pushNamed(named);
    }

    void push(const std::shared_ptr<ResultTable>& table) override
    {
        checkOpen();
        const ResultTableMeta& meta = table->meta();
        const std::shared_ptr<const ResultData> data = table->data();
        const std::string container = meta.elementName.empty() ? meta.subName : meta.elementName;
        if (data->stepCount == 0 || data->rowCount == 0 || data->componentCount == 0)
        {
            skip(std::string(resultKindName(meta.kind)), container, meta.subcase, write_reasons::NO_FILE_LAYOUT,
                 "an empty table (no step, row or component) has nothing to write");
            return;
        }

        const std::string group = groupPath(archive::TABLE_GROUPS, ++m_tableNumber);
        const auto path = [&](std::string_view name) { return group + "/" + std::string(name); };
        RequestBuilder builder(m_options);

        // the values: (steps, rows, components), with a last dimension of 2 (real, imaginary) for complex values
        std::vector<std::uint64_t> shape = {data->stepCount, data->rowCount, data->componentCount};
        if (isComplexType(data->type))
            shape.push_back(2);
        builder.numbers(path(archive::VALUES), realKindOf(data->type), shape, data->bytes());
        builder.texts(path(archive::COMPONENTS), data->components);

        // the labels of the steps
        builder.numbers(path(archive::STEPS), Hdf5ValueKind::Float64, {data->steps.size()}, data->steps.data());
        const auto perStep = [&](std::string_view name, const std::vector<double>& values)
        {
            if (!values.empty())
                builder.numbers(path(name), Hdf5ValueKind::Float64, {values.size()}, values.data());
        };
        perStep(archive::EIGENVALUES, data->eigenvalues);
        perStep(archive::EIGENVALUES_IMAG, data->eigenvaluesImag);
        perStep(archive::CYCLES, data->cycles);

        // the labels of the rows
        const auto perRow = [&](std::string_view name, const LabelArray<std::int32_t>& values)
        {
            if (!values.empty())
                builder.numbers(path(name), Hdf5ValueKind::Int32, {values.size()}, values.data());
        };
        perRow(archive::IDS, data->ids);
        perRow(archive::POINT_TYPES, data->pointTypes);
        perRow(archive::NODE_IDS, data->nodeIds);
        perRow(archive::ELEMENT_IDS, data->elementIds);
        perRow(archive::LAYERS, data->layers);
        if (!data->positions.empty())
            builder.numbers(path(archive::POSITIONS), Hdf5ValueKind::Float64, {data->positions.size()},
                            data->positions.data());
        addTexts(builder, group, data->texts);
        io::appendHdf5(m_temporary, builder.request());

        TableIndexRow row;
        row.group = group;
        row.kind = std::string(resultKindName(meta.kind));
        row.variant = std::string(resultVariantName(meta.variant));
        row.container = container;
        row.layout = meta.layout;
        row.tableName = meta.tableName;
        row.subcase = meta.subcase;
        row.analysisCode = meta.analysisCode;
        row.superelement = meta.superelement;
        row.designCycle = meta.designCycle;
        row.loadSet = meta.loadSet;
        row.sort = static_cast<std::int64_t>(meta.sort);
        row.stepKind = std::string(stepKindName(meta.stepKind));
        row.valueType = std::string(valueTypeName(data->type));
        row.storedFormat = std::string(resultFormatName(meta.storedFormat));
        row.status = std::string(resultStatusName(meta.status));
        row.storedFrame = std::string(storedFrameName(meta.storedFrame));
        row.frameSystem = meta.frameSystem;
        row.wordSize = meta.wordSize;
        row.units = meta.units;
        row.title = meta.title;
        row.subtitle = meta.subtitle;
        row.label = meta.label;
        row.stepCount = static_cast<std::int64_t>(data->stepCount);
        row.rowCount = static_cast<std::int64_t>(data->rowCount);
        row.componentCount = static_cast<std::int64_t>(data->componentCount);
        row.stepOffset = static_cast<std::int64_t>(m_steps.size());
        row.hasEigenvalues = data->eigenvalues.empty() ? 0 : 1;
        row.hasEigenvaluesImag = data->eigenvaluesImag.empty() ? 0 : 1;
        row.hasCycles = data->cycles.empty() ? 0 : 1;
        for (std::size_t step = 0; step < data->stepCount; ++step)
        {
            const auto valueAt = [&](const std::vector<double>& values)
            { return step < values.size() ? values[step] : std::numeric_limits<double>::quiet_NaN(); };
            m_steps.push_back(valueAt(data->steps));
            m_eigenvalues.push_back(valueAt(data->eigenvalues));
            m_eigenvaluesImag.push_back(valueAt(data->eigenvaluesImag));
            m_cycles.push_back(valueAt(data->cycles));
        }
        m_tables.push_back(std::move(row));

        WrittenTable written;
        written.kind = std::string(resultKindName(meta.kind));
        written.container = container;
        written.subcase = meta.subcase;
        written.fileTable = group;
        written.layout = meta.layout;
        written.steps = data->stepCount;
        written.rows = data->rowCount;
        m_report.written.push_back(std::move(written));
    }

    void pushNamed(const std::shared_ptr<NamedResult>& result) override
    {
        checkOpen();
        const NamedResultMeta& meta = result->meta();
        const std::shared_ptr<const NamedResultData> data = result->data();
        const std::string group = groupPath(archive::NAMED_GROUPS, ++m_namedNumber);
        RequestBuilder builder(m_options);

        std::vector<std::string> names;
        std::vector<std::string> descriptions;
        std::vector<std::string> types;
        std::vector<std::string> shapes;
        for (std::size_t index = 0; index < data->arrays.size(); ++index)
        {
            const ResultArray& array = data->arrays[index];
            names.push_back(array.name);
            descriptions.push_back(array.description);
            types.emplace_back(arrayTypeName(array.type));
            std::string shapeText;
            for (const std::size_t extent : array.shape)
                shapeText += (shapeText.empty() ? "" : ",") + std::to_string(extent);
            shapes.push_back(std::move(shapeText));
            std::vector<std::uint64_t> shape(array.shape.begin(), array.shape.end());
            if (isComplexArray(array.type))
                shape.push_back(2);
            builder.numbers(group + "/" + std::string(archive::ARRAYS) + "/" + std::to_string(index),
                            arrayKindOf(array.type), shape, array.bytes());
        }
        builder.texts(group + "/" + std::string(archive::ARRAY_NAMES), std::move(names));
        builder.texts(group + "/" + std::string(archive::ARRAY_DESCRIPTIONS), std::move(descriptions));
        builder.texts(group + "/" + std::string(archive::ARRAY_TYPES), std::move(types));
        builder.texts(group + "/" + std::string(archive::ARRAY_SHAPES), std::move(shapes));
        if (!meta.attributes.empty())
        {
            std::vector<std::string> pairs;
            for (const auto& [key, value] : meta.attributes)
            {
                pairs.push_back(key);
                pairs.push_back(value);
            }
            builder.texts(group + "/" + std::string(archive::ATTRIBUTES), std::move(pairs),
                          {meta.attributes.size(), 2});
        }
        addTexts(builder, group, data->texts);
        io::appendHdf5(m_temporary, builder.request());

        NamedIndexRow row;
        row.group = group;
        row.kind = meta.kind;
        row.name = meta.name;
        row.tableName = meta.tableName;
        row.subcase = meta.subcase;
        row.analysisCode = meta.analysisCode;
        row.superelement = meta.superelement;
        row.designCycle = meta.designCycle;
        row.status = std::string(resultStatusName(meta.status));
        row.wordSize = meta.wordSize;
        row.title = meta.title;
        row.subtitle = meta.subtitle;
        row.label = meta.label;
        m_named.push_back(std::move(row));

        WrittenTable written;
        written.kind = meta.kind;
        written.container = meta.name;
        written.subcase = meta.subcase;
        written.fileTable = group;
        m_report.written.push_back(std::move(written));
    }

    const ResultWriteReport& finish() override
    {
        checkOpen();
        if (m_options.strict && !m_report.skipped.empty())
        {
            std::string message = "archive: tables left out:";
            for (const SkippedTable& skipped : m_report.skipped)
                message += "\n  " + skipped.kind + " " + skipped.container + ": " + skipped.message;
            throw base::UsageError(message);
        }
        writeIndex();
        std::error_code ignored;
        std::filesystem::remove(m_target, ignored); // a rename does not replace a file on every platform
        std::filesystem::rename(m_temporary, m_target);
        m_report.bytes = std::filesystem::file_size(m_target);
        m_report.finished = true;
        m_finished = true;
        return m_report;
    }

    void abort() noexcept override
    {
        std::error_code ignored;
        std::filesystem::remove(m_temporary, ignored);
        m_finished = true;
    }

    [[nodiscard]] bool finished() const noexcept override { return m_finished; }
    [[nodiscard]] const ResultWriteReport& report() const noexcept override { return m_report; }

private:
    void checkOpen() const
    {
        if (m_finished)
            throw base::UsageError("archive: the session is finished");
    }

    void skip(std::string kind, std::string container, std::int32_t subcase, std::string_view reason,
              std::string message)
    {
        if (m_options.strict)
            throw base::UsageError("archive: " + kind + " " + container + ": " + message);
        m_report.skipped.push_back(
            SkippedTable{std::move(kind), std::move(container), subcase, std::string(reason), std::move(message)});
    }

    void writeIndex()
    {
        Hdf5WriteRequest request;
        std::deque<std::vector<std::int64_t>> storage; // the integer columns, alive until the write returns
        if (!m_tables.empty())
        {
            using Row = TableIndexRow;
            Hdf5WriteDataset index;
            index.path = std::string(archive::TABLE_INDEX);
            index.columns = {
                textColumn(m_tables, "group", &Row::group),
                textColumn(m_tables, "kind", &Row::kind),
                textColumn(m_tables, "variant", &Row::variant),
                textColumn(m_tables, "container", &Row::container),
                textColumn(m_tables, "layout", &Row::layout),
                textColumn(m_tables, "table_name", &Row::tableName),
                integerColumn(m_tables, "subcase", &Row::subcase, storage),
                integerColumn(m_tables, "analysis_code", &Row::analysisCode, storage),
                integerColumn(m_tables, "superelement", &Row::superelement, storage),
                integerColumn(m_tables, "design_cycle", &Row::designCycle, storage),
                integerColumn(m_tables, "load_set", &Row::loadSet, storage),
                integerColumn(m_tables, "sort", &Row::sort, storage),
                textColumn(m_tables, "step_kind", &Row::stepKind),
                textColumn(m_tables, "value_type", &Row::valueType),
                textColumn(m_tables, "stored_format", &Row::storedFormat),
                textColumn(m_tables, "status", &Row::status),
                textColumn(m_tables, "stored_frame", &Row::storedFrame),
                integerColumn(m_tables, "frame_system", &Row::frameSystem, storage),
                integerColumn(m_tables, "word_size", &Row::wordSize, storage),
                textColumn(m_tables, "units", &Row::units),
                textColumn(m_tables, "title", &Row::title),
                textColumn(m_tables, "subtitle", &Row::subtitle),
                textColumn(m_tables, "label", &Row::label),
                integerColumn(m_tables, "step_count", &Row::stepCount, storage),
                integerColumn(m_tables, "row_count", &Row::rowCount, storage),
                integerColumn(m_tables, "component_count", &Row::componentCount, storage),
                integerColumn(m_tables, "step_offset", &Row::stepOffset, storage),
                integerColumn(m_tables, "has_eigenvalues", &Row::hasEigenvalues, storage),
                integerColumn(m_tables, "has_eigenvalues_imag", &Row::hasEigenvaluesImag, storage),
                integerColumn(m_tables, "has_cycles", &Row::hasCycles, storage),
            };
            request.datasets.push_back(std::move(index));
            const auto stepColumn = [&](std::string_view path, const std::vector<double>& values)
            {
                Hdf5WriteDataset dataset;
                dataset.path = std::string(path);
                dataset.array.kind = Hdf5ValueKind::Float64;
                dataset.array.shape = {values.size()};
                dataset.array.data = values.data();
                request.datasets.push_back(std::move(dataset));
            };
            stepColumn(archive::INDEX_STEPS, m_steps);
            stepColumn(archive::INDEX_EIGENVALUES, m_eigenvalues);
            stepColumn(archive::INDEX_EIGENVALUES_IMAG, m_eigenvaluesImag);
            stepColumn(archive::INDEX_CYCLES, m_cycles);
        }
        if (!m_named.empty())
        {
            using Row = NamedIndexRow;
            Hdf5WriteDataset index;
            index.path = std::string(archive::NAMED_INDEX);
            index.columns = {
                textColumn(m_named, "group", &Row::group),
                textColumn(m_named, "kind", &Row::kind),
                textColumn(m_named, "name", &Row::name),
                textColumn(m_named, "table_name", &Row::tableName),
                integerColumn(m_named, "subcase", &Row::subcase, storage),
                integerColumn(m_named, "analysis_code", &Row::analysisCode, storage),
                integerColumn(m_named, "superelement", &Row::superelement, storage),
                integerColumn(m_named, "design_cycle", &Row::designCycle, storage),
                textColumn(m_named, "status", &Row::status),
                integerColumn(m_named, "word_size", &Row::wordSize, storage),
                textColumn(m_named, "title", &Row::title),
                textColumn(m_named, "subtitle", &Row::subtitle),
                textColumn(m_named, "label", &Row::label),
            };
            request.datasets.push_back(std::move(index));
        }
        if (!request.datasets.empty())
            io::appendHdf5(m_temporary, request);
    }

    std::filesystem::path m_target;
    std::filesystem::path m_temporary;
    ArchiveWriteOptions m_options;
    ResultWriteReport m_report;
    std::vector<TableIndexRow> m_tables;
    std::vector<NamedIndexRow> m_named;
    // the steps of every table, in the order of m_tables (see archive::INDEX_STEPS)
    std::vector<double> m_steps;
    std::vector<double> m_eigenvalues;
    std::vector<double> m_eigenvaluesImag;
    std::vector<double> m_cycles;
    std::size_t m_tableNumber = 0;
    std::size_t m_namedNumber = 0;
    bool m_finished = false;
};

} // namespace

std::unique_ptr<ResultWriteSession> beginArchive(const std::filesystem::path& target,
                                                 const ArchiveWriteOptions& options)
{
    return std::make_unique<ArchiveSession>(target, options);
}

ResultWriteReport writeArchive(const std::filesystem::path& target, const ResultTables& tables,
                               const ArchiveWriteOptions& options)
{
    const std::unique_ptr<ResultWriteSession> session = beginArchive(target, options);
    session->push(tables);
    return session->finish();
}

} // namespace oor::results
