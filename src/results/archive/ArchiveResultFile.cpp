#include "oor/results/archive/ArchiveResultFile.hpp"

#include "ArchiveNames.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/io/hdf5/Hdf5File.hpp"
#include "oor/io/hdf5/Hdf5Table.hpp"
#include "oor/results/archive/ArchiveFormat.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>

namespace oor::results
{

namespace
{

// ---------------------------------------------------------------------------------------------------------------
// Reading plain datasets into vectors, whatever integer or float size the file holds them in.

const io::Hdf5Column& onlyColumn(const io::Hdf5Table& table, const std::string& what)
{
    if (table.columns.empty())
        throw base::IoError("archive: " + what + " has no values");
    return table.columns.front();
}

template <typename T>
std::vector<T> numbersOf(const io::Hdf5Column& column, std::uint64_t count)
{
    std::vector<T> values(count);
    const std::byte* bytes = column.data.data();
    for (std::uint64_t index = 0; index < count; ++index)
    {
        const std::byte* item = bytes + index * column.itemSize;
        if (column.kind == io::Hdf5ColumnKind::Float)
        {
            if (column.itemSize == 4)
            {
                float value = 0;
                std::memcpy(&value, item, 4);
                values[index] = static_cast<T>(value);
            }
            else
            {
                double value = 0;
                std::memcpy(&value, item, 8);
                values[index] = static_cast<T>(value);
            }
        }
        else
        {
            std::int64_t value = 0;
            if (column.itemSize == 1)
                value = static_cast<std::int8_t>(*item);
            else if (column.itemSize == 2)
            {
                std::int16_t narrow = 0;
                std::memcpy(&narrow, item, 2);
                value = narrow;
            }
            else if (column.itemSize == 4)
            {
                if (column.kind == io::Hdf5ColumnKind::Unsigned)
                {
                    std::uint32_t narrow = 0;
                    std::memcpy(&narrow, item, 4);
                    value = narrow;
                }
                else
                {
                    std::int32_t narrow = 0;
                    std::memcpy(&narrow, item, 4);
                    value = narrow;
                }
            }
            else
            {
                std::memcpy(&value, item, 8);
            }
            values[index] = static_cast<T>(value);
        }
    }
    return values;
}

template <typename T>
std::vector<T> readNumbers(const std::filesystem::path& file, const std::string& dataset)
{
    const io::Hdf5Table table = io::readHdf5Table(file, dataset);
    const io::Hdf5Column& column = onlyColumn(table, dataset);
    std::uint64_t perRow = 1;
    for (const std::uint64_t extent : column.shape)
        perRow *= extent;
    return numbersOf<T>(column, table.rowCount * perRow);
}

std::vector<std::string> readTexts(const std::filesystem::path& file, const std::string& dataset)
{
    io::Hdf5Table table = io::readHdf5Table(file, dataset);
    if (table.columns.empty())
        return {};
    return std::move(table.columns.front().strings);
}

// The columns of a compound index dataset, by name.
class IndexColumns
{
public:
    IndexColumns(const std::filesystem::path& file, std::string_view dataset)
        : m_table(io::readHdf5Table(file, std::string(dataset))), m_name(dataset)
    {
    }

    [[nodiscard]] std::uint64_t rowCount() const noexcept { return m_table.rowCount; }

    [[nodiscard]] const std::vector<std::string>& texts(const char* name) const
    {
        const io::Hdf5Column* column = m_table.column(name);
        if (column == nullptr || column->kind != io::Hdf5ColumnKind::VarString)
            throw base::IoError("archive: " + m_name + " has no text column '" + name + "'");
        return column->strings;
    }

    [[nodiscard]] std::vector<std::int64_t> integers(const char* name) const
    {
        const io::Hdf5Column* column = m_table.column(name);
        if (column == nullptr)
            throw base::IoError("archive: " + m_name + " has no column '" + name + "'");
        return numbersOf<std::int64_t>(*column, m_table.rowCount);
    }

private:
    io::Hdf5Table m_table;
    std::string m_name;
};

std::string readRootText(const io::Hdf5File& file, std::string_view name)
{
    try
    {
        std::string text = file.readStringAttribute(std::string(name));
        // an empty text is written as one NUL byte (a fixed string has at least one byte)
        while (!text.empty() && text.back() == '\0')
            text.pop_back();
        return text;
    }
    catch (const base::IoError&)
    {
        return {};
    }
}

template <typename T>
std::vector<T> gather(const std::vector<T>& values, const std::vector<std::uint32_t>& positions)
{
    std::vector<T> result;
    if (values.empty())
        return result;
    result.reserve(positions.size());
    for (const std::uint32_t position : positions)
        result.push_back(values[position]);
    return result;
}

// ---------------------------------------------------------------------------------------------------------------
// The source of the values of one table: its group, and the steps of the table it reads.

class ArchiveTableSource final : public ResultTableSource
{
public:
    ArchiveTableSource(std::filesystem::path file, std::string group, const ResultTableMeta& meta,
                       std::vector<std::uint32_t> stepPositions)
        : m_file(std::move(file)),
          m_group(std::move(group)),
          m_type(meta.valueType),
          m_stepKind(meta.stepKind),
          m_rowCount(meta.rowCount),
          m_components(meta.components),
          m_stepPositions(std::move(stepPositions))
    {
    }

    [[nodiscard]] std::shared_ptr<const ResultData> load(const LoadRequest& request,
                                                         const DecodeContext& context) const override
    {
        if (context.token != nullptr)
            context.token->throwIfCancelled();
        auto data = std::make_shared<ResultData>();
        data->type = m_type;
        data->stepCount = m_stepPositions.size();
        data->rowCount = m_rowCount;
        data->componentCount = m_components.size();
        data->components = m_components;
        data->allocate();
        readValues(*data);

        const io::Hdf5File file = io::Hdf5File::openReadOnly(m_file);
        const io::Hdf5Group group = file.openGroup(m_group);
        const auto has = [&](std::string_view name) { return group.hasChild(std::string(name)); };
        const auto path = [&](std::string_view name) { return m_group + "/" + std::string(name); };

        data->steps = gather(readNumbers<double>(m_file, path(archive::STEPS)), m_stepPositions);
        if (has(archive::EIGENVALUES))
            data->eigenvalues = gather(readNumbers<double>(m_file, path(archive::EIGENVALUES)), m_stepPositions);
        if (has(archive::EIGENVALUES_IMAG))
            data->eigenvaluesImag =
                gather(readNumbers<double>(m_file, path(archive::EIGENVALUES_IMAG)), m_stepPositions);
        if (has(archive::CYCLES))
            data->cycles = gather(readNumbers<double>(m_file, path(archive::CYCLES)), m_stepPositions);

        const auto readRow = [&](std::string_view name, LabelArray<std::int32_t>& target)
        {
            if (has(name))
                target = LabelArray<std::int32_t>(readNumbers<std::int32_t>(m_file, path(name)));
        };
        readRow(archive::IDS, data->ids);
        readRow(archive::POINT_TYPES, data->pointTypes);
        readRow(archive::NODE_IDS, data->nodeIds);
        readRow(archive::ELEMENT_IDS, data->elementIds);
        readRow(archive::LAYERS, data->layers);
        if (has(archive::POSITIONS))
            data->positions = LabelArray<double>(readNumbers<double>(m_file, path(archive::POSITIONS)));
        if (has(archive::TEXT_NAMES))
        {
            const std::vector<std::string> names = readTexts(m_file, path(archive::TEXT_NAMES));
            for (std::size_t index = 0; index < names.size(); ++index)
            {
                const std::string folder = path(archive::TEXTS) + "/" + std::to_string(index) + "/";
                std::vector<std::string> dictionary = readTexts(m_file, folder + std::string(archive::DICTIONARY));
                std::vector<std::uint32_t> codes =
                    readNumbers<std::uint32_t>(m_file, folder + std::string(archive::CODES));
                data->texts.emplace_back(names[index], std::move(dictionary),
                                         LabelArray<std::uint32_t>(std::move(codes)));
            }
        }

        // the rows, layers and components the request keeps
        ResultFilter selection = request.filter;
        if (!request.components.empty() && request.components != m_components)
            selection.components = request.components;
        return subsetResultData(std::shared_ptr<const ResultData>(std::move(data)), selection, m_stepKind);
    }

    [[nodiscard]] bool stepsInData() const noexcept override { return false; }

    [[nodiscard]] std::shared_ptr<const ResultTableSource> withSteps(
        std::span<const std::uint32_t> positions) const override
    {
        std::vector<std::uint32_t> kept;
        kept.reserve(positions.size());
        for (const std::uint32_t position : positions)
            kept.push_back(m_stepPositions[position]);
        auto source = std::make_shared<ArchiveTableSource>(*this);
        source->m_stepPositions = std::move(kept);
        return source;
    }

private:
    // Reads the values of the steps of the source: one read a run of consecutive steps, each step a whole chunk.
    void readValues(ResultData& data) const
    {
        const std::size_t stepBytes = data.rowCount * data.componentCount * valueTypeSize(data.type);
        const std::string dataset = m_group + "/" + std::string(archive::VALUES);
        std::size_t done = 0;
        while (done < m_stepPositions.size())
        {
            std::size_t runEnd = done + 1;
            while (runEnd < m_stepPositions.size() && m_stepPositions[runEnd] == m_stepPositions[runEnd - 1] + 1)
                ++runEnd;
            io::Hdf5ReadOptions options;
            options.rows = std::pair<std::uint64_t, std::uint64_t>(m_stepPositions[done],
                                                                   std::uint64_t{m_stepPositions[runEnd - 1]} + 1);
            const io::Hdf5Table table = io::readHdf5Table(m_file, dataset, options);
            const io::Hdf5Column& column = onlyColumn(table, dataset);
            const std::size_t bytes = (runEnd - done) * stepBytes;
            if (column.data.size() != bytes)
                throw base::IoError("archive: " + dataset + " does not have the shape its index gives");
            std::memcpy(data.mutableBytes() + done * stepBytes, column.data.data(), bytes);
            done = runEnd;
        }
    }

    std::filesystem::path m_file;
    std::string m_group;
    ValueType m_type;
    StepKind m_stepKind;
    std::size_t m_rowCount;
    std::vector<std::string> m_components;
    std::vector<std::uint32_t> m_stepPositions;
};

ArrayType arrayTypeFromName(std::string_view text)
{
    return archive::enumFromName(text,
                                 {ArrayType::Int32, ArrayType::Int64, ArrayType::Float32, ArrayType::Float64,
                                  ArrayType::Complex64, ArrayType::Complex128},
                                 arrayTypeName, "array type");
}

// Reads the arrays and texts of a named result group.
std::shared_ptr<const NamedResultData> loadNamed(const std::filesystem::path& file, const std::string& group)
{
    auto data = std::make_shared<NamedResultData>();
    const std::vector<std::string> names = readTexts(file, group + "/" + std::string(archive::ARRAY_NAMES));
    const std::vector<std::string> descriptions =
        readTexts(file, group + "/" + std::string(archive::ARRAY_DESCRIPTIONS));
    const std::vector<std::string> types = readTexts(file, group + "/" + std::string(archive::ARRAY_TYPES));
    const std::vector<std::string> shapes = readTexts(file, group + "/" + std::string(archive::ARRAY_SHAPES));
    for (std::size_t index = 0; index < names.size(); ++index)
    {
        const std::string dataset = group + "/" + std::string(archive::ARRAYS) + "/" + std::to_string(index);
        const io::Hdf5Table table = io::readHdf5Table(file, dataset);
        const io::Hdf5Column& column = onlyColumn(table, dataset);
        ResultArray array;
        array.name = names[index];
        array.description = index < descriptions.size() ? descriptions[index] : std::string();
        array.type = arrayTypeFromName(index < types.size() ? types[index] : "float64");
        // the shape as written ("3,4"; "" for a single number)
        std::vector<std::size_t> shape;
        const std::string shapeText = index < shapes.size() ? shapes[index] : std::string();
        std::size_t start = 0;
        while (start < shapeText.size())
        {
            const std::size_t comma = std::min(shapeText.find(',', start), shapeText.size());
            shape.push_back(static_cast<std::size_t>(std::stoull(shapeText.substr(start, comma - start))));
            start = comma + 1;
        }
        array.shape = std::move(shape);
        array.allocate();
        if (column.data.size() != array.byteCount())
            throw base::IoError("archive: " + dataset + " does not have the size of its type and shape");
        std::memcpy(array.mutableBytes(), column.data.data(), array.byteCount());
        data->arrays.push_back(std::move(array));
    }
    const std::string textNames = group + "/" + std::string(archive::TEXT_NAMES);
    const io::Hdf5File handle = io::Hdf5File::openReadOnly(file);
    if (handle.hasChild(textNames.substr(1)))
    {
        const std::vector<std::string> textList = readTexts(file, textNames);
        for (std::size_t index = 0; index < textList.size(); ++index)
        {
            const std::string folder = group + "/" + std::string(archive::TEXTS) + "/" + std::to_string(index) + "/";
            std::vector<std::string> dictionary = readTexts(file, folder + std::string(archive::DICTIONARY));
            std::vector<std::uint32_t> codes = readNumbers<std::uint32_t>(file, folder + std::string(archive::CODES));
            data->texts.emplace_back(textList[index], std::move(dictionary),
                                     LabelArray<std::uint32_t>(std::move(codes)));
        }
    }
    return data;
}

} // namespace

bool ArchiveResultFile::isArchive(const std::filesystem::path& path)
{
    static constexpr std::array<unsigned char, 8> HDF5_SIGNATURE = {0x89, 'H', 'D', 'F', '\r', '\n', 0x1a, '\n'};
    std::ifstream stream(path, std::ios::binary);
    std::array<char, 8> start{};
    if (!stream.read(start.data(), start.size()))
        return false;
    if (std::memcmp(start.data(), HDF5_SIGNATURE.data(), start.size()) != 0)
        return false;
    try
    {
        const io::Hdf5File file = io::Hdf5File::openReadOnly(path);
        return readRootText(file, archive::ATTRIBUTE_FORMAT) == archive::FORMAT_NAME;
    }
    catch (const base::OorError&)
    {
        return false;
    }
}

std::shared_ptr<ArchiveResultFile> ArchiveResultFile::open(const std::filesystem::path& path)
{
    std::shared_ptr<ArchiveResultFile> archiveFile(new ArchiveResultFile());
    {
        const io::Hdf5File file = io::Hdf5File::openReadOnly(path);
        if (readRootText(file, archive::ATTRIBUTE_FORMAT) != archive::FORMAT_NAME)
            throw base::IoError("not a results archive (no format attribute '" + std::string(archive::FORMAT_NAME) +
                                "' at its root): " + path.string());
        archiveFile->m_formatVersion = readRootText(file, archive::ATTRIBUTE_FORMAT_VERSION);
        if (archiveFile->m_formatVersion != archive::FORMAT_VERSION)
            throw base::IoError("the archive " + path.string() + " has the format version " +
                                archiveFile->m_formatVersion + "; this library reads version " +
                                std::string(archive::FORMAT_VERSION));
        ResultFileMetadata& metadata = archiveFile->m_metadata;
        metadata.format = "archive";
        metadata.origin = "archive";
        metadata.path = path.string();
        metadata.fileSize = std::filesystem::file_size(path);
        metadata.producer = readRootText(file, archive::ATTRIBUTE_PRODUCER);
        metadata.createdAt = readRootText(file, archive::ATTRIBUTE_CREATED_AT);
        metadata.title = readRootText(file, archive::ATTRIBUTE_TITLE);
        metadata.subtitle = readRootText(file, archive::ATTRIBUTE_SUBTITLE);
        metadata.label = readRootText(file, archive::ATTRIBUTE_LABEL);
    }
    archiveFile->readIndex();
    return archiveFile;
}

void ArchiveResultFile::readIndex()
{
    const std::filesystem::path file(m_metadata.path);
    bool hasTables = false;
    bool hasNamed = false;
    {
        const io::Hdf5File handle = io::Hdf5File::openReadOnly(file);
        hasTables = handle.hasChild(std::string(archive::TABLE_INDEX).substr(1));
        hasNamed = handle.hasChild(std::string(archive::NAMED_INDEX).substr(1));
    }

    if (hasTables)
    {
        const IndexColumns index(file, archive::TABLE_INDEX);
        const std::vector<double> allSteps = readNumbers<double>(file, std::string(archive::INDEX_STEPS));
        const std::vector<double> allEigenvalues = readNumbers<double>(file, std::string(archive::INDEX_EIGENVALUES));
        const std::vector<double> allEigenvaluesImag =
            readNumbers<double>(file, std::string(archive::INDEX_EIGENVALUES_IMAG));
        const std::vector<double> allCycles = readNumbers<double>(file, std::string(archive::INDEX_CYCLES));

        const auto& groups = index.texts("group");
        const auto& kinds = index.texts("kind");
        const auto& variants = index.texts("variant");
        const auto& containers = index.texts("container");
        const auto& layouts = index.texts("layout");
        const auto& tableNames = index.texts("table_name");
        const auto& stepKinds = index.texts("step_kind");
        const auto& valueTypes = index.texts("value_type");
        const auto& storedFormats = index.texts("stored_format");
        const auto& statuses = index.texts("status");
        const auto& storedFrames = index.texts("stored_frame");
        const auto& units = index.texts("units");
        const auto& titles = index.texts("title");
        const auto& subtitles = index.texts("subtitle");
        const auto& labels = index.texts("label");
        const auto subcases = index.integers("subcase");
        const auto analysisCodes = index.integers("analysis_code");
        const auto superelements = index.integers("superelement");
        const auto designCycles = index.integers("design_cycle");
        const auto loadSets = index.integers("load_set");
        const auto sorts = index.integers("sort");
        const auto frameSystems = index.integers("frame_system");
        const auto wordSizes = index.integers("word_size");
        const auto stepCounts = index.integers("step_count");
        const auto rowCounts = index.integers("row_count");
        const auto componentCounts = index.integers("component_count");
        const auto stepOffsets = index.integers("step_offset");
        const auto hasEigenvalues = index.integers("has_eigenvalues");
        const auto hasEigenvaluesImag = index.integers("has_eigenvalues_imag");
        const auto hasCycles = index.integers("has_cycles");

        for (std::uint64_t row = 0; row < index.rowCount(); ++row)
        {
            const std::optional<ResultKind> kind = resultKindFromName(kinds[row]);
            const std::optional<ResultVariant> variant = resultVariantFromName(variants[row]);
            if (!kind || !variant)
            {
                base::Diagnostic diagnostic;
                diagnostic.severity = base::Severity::Warning;
                diagnostic.code = "archive-unknown-kind";
                diagnostic.message = "the table " + groups[row] + " has the kind '" + kinds[row] + "' (variant '" +
                                     variants[row] + "'), which this version does not know: left out";
                m_diagnostics.push_back(std::move(diagnostic));
                continue;
            }
            ResultTableMeta meta;
            meta.kind = *kind;
            meta.variant = *variant;
            if (resultKindInfo(*kind).element)
                meta.elementName = containers[row];
            else
                meta.subName = containers[row];
            meta.layout = layouts[row];
            meta.tableName = tableNames[row];
            meta.subcase = static_cast<std::int32_t>(subcases[row]);
            meta.analysisCode = static_cast<std::int32_t>(analysisCodes[row]);
            meta.superelement = static_cast<std::int32_t>(superelements[row]);
            meta.designCycle = static_cast<std::int32_t>(designCycles[row]);
            meta.loadSet = static_cast<std::int32_t>(loadSets[row]);
            meta.sort = sorts[row] == 2 ? SortOrder::Sort2 : SortOrder::Sort1;
            meta.stepKind = archive::stepKindFromName(stepKinds[row]);
            meta.valueType = archive::valueTypeFromName(valueTypes[row]);
            meta.valueKind = isComplexType(meta.valueType) ? ValueKind::Complex : ValueKind::Real;
            meta.storedFormat = archive::resultFormatFromName(storedFormats[row]);
            meta.status = archive::resultStatusFromName(statuses[row]);
            meta.storedFrame = archive::storedFrameFromName(storedFrames[row]);
            meta.frameSystem = static_cast<std::int32_t>(frameSystems[row]);
            meta.wordSize = static_cast<int>(wordSizes[row]);
            meta.units = units[row];
            meta.title = titles[row];
            meta.subtitle = subtitles[row];
            meta.label = labels[row];
            meta.origin = "archive";
            meta.stepCount = static_cast<std::size_t>(stepCounts[row]);
            meta.rowCount = static_cast<std::size_t>(rowCounts[row]);
            const auto first = static_cast<std::size_t>(stepOffsets[row]);
            if (first + meta.stepCount > allSteps.size())
                throw base::IoError("archive: the steps of " + groups[row] + " are outside " +
                                    std::string(archive::INDEX_STEPS));
            const auto stepsOf = [&](const std::vector<double>& all)
            { return std::vector<double>(all.begin() + first, all.begin() + first + meta.stepCount); };
            meta.steps = stepsOf(allSteps);
            if (hasEigenvalues[row] != 0)
                meta.eigenvalues = stepsOf(allEigenvalues);
            if (hasEigenvaluesImag[row] != 0)
                meta.eigenvaluesImag = stepsOf(allEigenvaluesImag);
            if (hasCycles[row] != 0)
                meta.cycles = stepsOf(allCycles);
            meta.components = readTexts(file, groups[row] + "/" + std::string(archive::COMPONENTS));
            if (meta.components.size() != static_cast<std::size_t>(componentCounts[row]))
                throw base::IoError("archive: the components of " + groups[row] + " are not those its index gives");

            std::vector<std::uint32_t> allPositions(meta.stepCount);
            for (std::size_t step = 0; step < meta.stepCount; ++step)
                allPositions[step] = static_cast<std::uint32_t>(step);
            auto source = std::make_shared<ArchiveTableSource>(file, groups[row], meta, std::move(allPositions));
            m_tables.push_back(std::make_shared<ResultTable>(std::move(meta), std::move(source)));
        }
    }

    if (hasNamed)
    {
        const IndexColumns index(file, archive::NAMED_INDEX);
        const auto& groups = index.texts("group");
        const auto& kinds = index.texts("kind");
        const auto& names = index.texts("name");
        const auto& tableNames = index.texts("table_name");
        const auto& statuses = index.texts("status");
        const auto& titles = index.texts("title");
        const auto& subtitles = index.texts("subtitle");
        const auto& labels = index.texts("label");
        const auto subcases = index.integers("subcase");
        const auto analysisCodes = index.integers("analysis_code");
        const auto superelements = index.integers("superelement");
        const auto designCycles = index.integers("design_cycle");
        const auto wordSizes = index.integers("word_size");
        const io::Hdf5File handle = io::Hdf5File::openReadOnly(file);
        for (std::uint64_t row = 0; row < index.rowCount(); ++row)
        {
            NamedResultMeta meta;
            meta.kind = kinds[row];
            meta.name = names[row];
            meta.tableName = tableNames[row];
            meta.subcase = static_cast<std::int32_t>(subcases[row]);
            meta.analysisCode = static_cast<std::int32_t>(analysisCodes[row]);
            meta.superelement = static_cast<std::int32_t>(superelements[row]);
            meta.designCycle = static_cast<std::int32_t>(designCycles[row]);
            meta.status = archive::resultStatusFromName(statuses[row]);
            meta.wordSize = static_cast<int>(wordSizes[row]);
            meta.title = titles[row];
            meta.subtitle = subtitles[row];
            meta.label = labels[row];
            const std::string attributes = groups[row] + "/" + std::string(archive::ATTRIBUTES);
            if (handle.hasChild(attributes.substr(1)))
            {
                const std::vector<std::string> pairs = readTexts(file, attributes);
                for (std::size_t at = 0; at + 1 < pairs.size(); at += 2)
                    meta.attributes.emplace_back(pairs[at], pairs[at + 1]);
            }
            const std::string group = groups[row];
            auto source = std::make_shared<FunctionNamedSource>([file, group](const DecodeContext&)
                                                                { return loadNamed(file, group); });
            m_named.push_back(std::make_shared<NamedResult>(std::move(meta), std::move(source)));
        }
    }
    m_metadata.tableCount = static_cast<std::uint32_t>(m_tables.size());
    m_metadata.namedCount = static_cast<std::uint32_t>(m_named.size());
}

ResultTables ArchiveResultFile::tables(const ResultFilter& filter) const
{
    return ResultTables::fromTables(m_tables, m_named, filter);
}

} // namespace oor::results
