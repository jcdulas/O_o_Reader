#include "oor/results/archive/ArchiveModel.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/NumberFormatter.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/io/hdf5/Hdf5File.hpp"
#include "oor/io/hdf5/Hdf5StreamWriter.hpp"
#include "oor/io/hdf5/Hdf5Table.hpp"
#include "oor/model/control/ControlRecords.hpp"
#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/beam2/Beam2Table.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/core/UnknownCardTable.hpp"
#include "oor/results/archive/ArchiveFormat.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <deque>
#include <limits>
#include <map>
#include <span>
#include <unordered_map>
#include <unordered_set>

namespace oor::results::archive
{
namespace
{

using model::FieldValue;
using Kind = FieldValue::Kind;
using io::Hdf5ValueKind;
constexpr std::uint64_t DENSE = std::numeric_limits<std::uint64_t>::max();
constexpr std::uint64_t CONSTANT = DENSE - 1;
constexpr std::size_t READ_BATCH = 4096;
constexpr std::uint32_t MAX_FIELDS = 1u << 24;

// All model datasets are one-dimensional. Large typed buffers are chunked at about one MiB.
class Output
{
public:
    Output(int compression, bool shuffle) : m_compression(compression), m_shuffle(shuffle) {}

    template <typename T>
    void numbers(const std::string& path, std::vector<T> values)
    {
        auto kept = std::make_shared<std::vector<T>>(std::move(values));
        io::Hdf5WriteDataset dataset;
        dataset.path = path;
        if constexpr (std::is_same_v<T, double>)
            dataset.array.kind = Hdf5ValueKind::Float64;
        else if constexpr (std::is_same_v<T, std::int64_t>)
            dataset.array.kind = Hdf5ValueKind::Int64;
        else if constexpr (std::is_same_v<T, std::int32_t>)
            dataset.array.kind = Hdf5ValueKind::Int32;
        else if constexpr (std::is_same_v<T, std::uint64_t>)
            dataset.array.kind = Hdf5ValueKind::UInt64;
        else if constexpr (std::is_same_v<T, std::uint32_t>)
            dataset.array.kind = Hdf5ValueKind::UInt32;
        else
        {
            static_assert(std::is_same_v<T, std::uint8_t>);
            dataset.array.kind = Hdf5ValueKind::UInt8;
        }
        dataset.array.shape = {kept->size()};
        dataset.array.data = kept->data();
        if (m_compression > 0 && kept->size() * sizeof(T) >= 4096)
        {
            dataset.deflate = m_compression;
            dataset.shuffle = m_shuffle;
            dataset.chunkRows = std::min<std::uint64_t>(kept->size(), (1u << 20) / sizeof(T));
        }
        m_storage.push_back(kept);
        m_request.datasets.push_back(std::move(dataset));
    }

    // UTF-8 is stored in a contiguous byte buffer, never in the HDF5 variable-length heap.
    void strings(const std::string& path, const std::vector<std::string>& strings)
    {
        std::vector<std::uint64_t> offsets{0};
        std::vector<std::uint8_t> bytes;
        for (const std::string& text : strings)
        {
            bytes.insert(bytes.end(), text.begin(), text.end());
            offsets.push_back(bytes.size());
        }
        numbers(path + "/offsets", std::move(offsets));
        numbers(path + "/bytes", std::move(bytes));
    }

    void group(const std::string& path, std::initializer_list<std::pair<std::string_view, std::string_view>> attributes)
    {
        io::Hdf5WriteGroup group;
        group.path = path;
        for (const auto& [name, value] : attributes)
        {
            const std::string& kept = m_text.emplace_back(value.empty() ? std::string(1, '\0') : std::string(value));
            io::Hdf5WriteAttribute attribute;
            attribute.name = std::string(name);
            attribute.array.kind = Hdf5ValueKind::FixedString;
            attribute.array.utf8 = true;
            attribute.array.stringBytes = kept.size();
            attribute.array.data = kept.data();
            group.attributes.push_back(std::move(attribute));
        }
        m_request.groups.push_back(std::move(group));
    }

    void append(const std::filesystem::path& file) { io::appendHdf5(file, m_request); }

private:
    int m_compression;
    bool m_shuffle;
    io::Hdf5WriteRequest m_request;
    std::vector<std::shared_ptr<const void>> m_storage;
    std::deque<std::string> m_text;
};

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw base::IoError("archive model: " + message);
}

// Keep diagnostic strings out of the per-field hot path.
void require(bool condition, const std::string& path, std::string_view message)
{
    if (!condition)
        throw base::IoError("archive model: " + path + std::string(message));
}

template <typename T>
std::vector<T> numbers(const std::filesystem::path& file, const std::string& path)
{
    const io::Hdf5Table table = io::readHdf5Table(file, path);
    require(table.columns.size() == 1, path + " must be a plain array");
    const io::Hdf5Column& column = table.columns.front();
    constexpr auto expected = std::is_floating_point_v<T> ? io::Hdf5ColumnKind::Float
                              : std::is_signed_v<T>        ? io::Hdf5ColumnKind::Signed
                                                          : io::Hdf5ColumnKind::Unsigned;
    require(column.kind == expected && column.itemSize == sizeof(T) && column.shape.empty(),
            path + " has an unexpected type or shape");
    require(table.rowCount <= std::numeric_limits<std::size_t>::max() / sizeof(T) &&
                column.data.size() == table.rowCount * sizeof(T), path + " has an invalid length");
    std::vector<T> result(static_cast<std::size_t>(table.rowCount));
    if (!result.empty())
        std::memcpy(result.data(), column.data.data(), column.data.size());
    return result;
}

void offsetsValid(const std::vector<std::uint64_t>& offsets, std::size_t size, const std::string& path)
{
    require(!offsets.empty() && offsets.front() == 0 && offsets.back() == size &&
                std::is_sorted(offsets.begin(), offsets.end()), path + " has invalid offsets");
}

std::vector<std::string> strings(const std::filesystem::path& file, const std::string& path)
{
    const auto offsets = numbers<std::uint64_t>(file, path + "/offsets");
    const auto bytes = numbers<std::uint8_t>(file, path + "/bytes");
    offsetsValid(offsets, bytes.size(), path);
    std::vector<std::string> result;
    require(offsets.size() - 1 <= io::Hdf5Limits{}.maxReadBytes / sizeof(std::string),
            "a string vector exceeds the byte limit");
    result.reserve(offsets.size() - 1);
    for (std::size_t i = 1; i < offsets.size(); ++i)
    {
        const auto count = static_cast<std::size_t>(offsets[i] - offsets[i - 1]);
        require(count <= io::Hdf5Limits{}.maxStringBytes, path, " has a string exceeding the byte limit");
        if (count == 0)
            result.emplace_back();
        else
            result.emplace_back(reinterpret_cast<const char*>(bytes.data() + offsets[i - 1]), count);
    }
    return result;
}

std::string partPath(std::size_t index)
{
    return std::string(MODEL_PARTS) + "/" + std::to_string(index);
}

// Only the dictionary is deduplicated; numeric values remain directly addressable by column slices.
class Dictionary
{
public:
    std::uint32_t add(const std::string& text)
    {
        if (const auto found = m_codes.find(text); found != m_codes.end())
            return found->second;
        if (values.size() >= std::numeric_limits<std::uint32_t>::max())
            throw base::UsageError("archive model: too many distinct strings");
        const auto code = static_cast<std::uint32_t>(values.size());
        m_codes.emplace(text, code);
        values.push_back(text);
        return code;
    }
    std::vector<std::string> values;

private:
    std::unordered_map<std::string, std::uint32_t> m_codes;
};

struct Column
{
    std::vector<std::uint32_t> rows;
    std::vector<std::int64_t> integers;
    std::vector<double> reals;
    std::vector<std::uint32_t> texts;

    [[nodiscard]] bool isConstant(Kind kind) const
    {
        if (kind == Kind::Integer)
            return std::all_of(integers.begin(), integers.end(), [&](auto value) { return value == integers.front(); });
        if (kind == Kind::Text)
            return std::all_of(texts.begin(), texts.end(), [&](auto value) { return value == texts.front(); });
        return std::all_of(reals.begin(), reals.end(), [&](double value)
                           {
                               return std::bit_cast<std::uint64_t>(value) ==
                                      std::bit_cast<std::uint64_t>(reals.front());
                           });
    }
};

std::uint8_t defaultMask(const model::CardTable& table, std::uint32_t row)
{
    const auto name = table.info().name;
    if (name == model::NodeTable::NAME)
        return static_cast<const model::NodeTable&>(table).card(row).blankFields;
    if (name == model::Bar2Table::NAME)
        return static_cast<const model::Bar2Table&>(table).card(row).blankFields;
    if (name == model::Beam2Table::NAME)
        return static_cast<const model::Beam2Table&>(table).card(row).blankFields;
    return 0;
}

template <typename Table>
void restoreDefaultMask(model::CardTable& base, std::uint32_t row, std::uint8_t mask)
{
    auto& table = static_cast<Table&>(base);
    auto card = table.card(row);
    card.blankFields = mask;
    table.store(row, card);
}

void restoreDefaultMask(model::CardTable& table, std::uint32_t row, std::uint8_t mask)
{
    // A blank that has not yet received a defaults card must still receive one added after reloading.
    if (mask == 0)
        return;
    const auto name = table.info().name;
    if (name == model::NodeTable::NAME && mask <= 15)
        restoreDefaultMask<model::NodeTable>(table, row, mask);
    else if (name == model::Bar2Table::NAME && mask <= 31)
        restoreDefaultMask<model::Bar2Table>(table, row, mask);
    else if (name == model::Beam2Table::NAME && mask <= 31)
        restoreDefaultMask<model::Beam2Table>(table, row, mask);
    else
        require(false, "invalid pending defaults mask");
}

// Columns are identified by (1-based slot, kind). An integer/real alternative has two sparse columns.
void writeCards(const std::filesystem::path& file, const std::string& path, const model::CardTable& table,
                int compression, bool shuffle)
{
    std::map<std::pair<std::uint32_t, Kind>, Column> columns;
    std::vector<std::uint32_t> keys, counts;
    std::vector<std::int32_t> ids;
    std::vector<std::uint64_t> digitOffsets{0};
    std::vector<std::uint8_t> digits;
    std::vector<std::uint8_t> defaultMasks;
    Dictionary dictionary;
    const auto add = [&](std::uint32_t row, std::uint32_t slot, const FieldValue& value)
    {
        if (value.kind == Kind::Blank)
            return;
        const Kind kind = value.kind == Kind::SpanText || value.kind == Kind::NamedReal ? Kind::Text : value.kind;
        Column& column = columns[{slot, kind}];
        column.rows.push_back(row);
        if (kind == Kind::Integer)
            column.integers.push_back(value.integer);
        else if (kind == Kind::Real)
            column.reals.push_back(value.real);
        else
        {
            const std::string text = value.kind == Kind::NamedReal
                                         ? value.text + "=" + base::NumberFormatter::formatRealFree(value.real)
                                         : value.text;
            column.texts.push_back(dictionary.add(text));
        }
    };
    const auto live = table.rows().liveRows();
    for (const std::uint32_t source : live)
    {
        const auto row = static_cast<std::uint32_t>(keys.size());
        keys.push_back(table.rows().keyAt(source).value);
        ids.push_back(table.idAt(source));
        defaultMasks.push_back(defaultMask(table, source));
        const auto precision = table.rows().significantDigits(source);
        digits.insert(digits.end(), precision.begin(), precision.end());
        digitOffsets.push_back(digits.size());
        if (table.info().name == model::UnknownCardTable::NAME)
        {
            const auto& unknown = static_cast<const model::UnknownCardTable&>(table);
            FieldValue value;
            value.kind = Kind::Text;
            value.text = std::string(unknown.nameAt(source).view());
            add(row, 1, value);
            const auto fields = unknown.fieldsAt(source);
            counts.push_back(static_cast<std::uint32_t>(fields.size() + 1));
            for (std::size_t slot = 0; slot < fields.size(); ++slot)
            {
                value.text = fields[slot];
                add(row, static_cast<std::uint32_t>(slot + 2), value);
            }
        }
        else
        {
            // Explicit defaults make columns self-contained and independent of defaults cards and reader order.
            model::FieldWriter writer(false, 0, false, true);
            writer.keepSignOfZero(true);
            table.writeCard(source, writer);
            const auto fields = writer.typedFields();
            if (fields.size() > MAX_FIELDS)
                throw base::UsageError("archive model: a card has too many fields");
            const auto used = static_cast<std::uint32_t>(fields.size());
            counts.push_back(std::max<std::uint32_t>(8, (used + 7) / 8 * 8));
            for (std::size_t slot = 0; slot < fields.size(); ++slot)
                add(row, static_cast<std::uint32_t>(slot + 1), fields[slot]);
        }
    }
    std::vector<std::uint32_t> slots, sparseRows, textCodes;
    std::vector<std::uint8_t> kinds;
    std::vector<std::uint64_t> starts, lengths, rowStarts;
    std::vector<std::int64_t> integers;
    std::vector<double> reals;
    for (auto& [key, column] : columns)
    {
        const auto [slot, kind] = key;
        slots.push_back(slot);
        kinds.push_back(static_cast<std::uint8_t>(kind));
        const bool dense = column.rows.size() == keys.size();
        const bool constant = dense && !column.rows.empty() && column.isConstant(kind);
        lengths.push_back(constant ? 1 : column.rows.size());
        if (dense)
            rowStarts.push_back(constant ? CONSTANT : DENSE);
        else
        {
            rowStarts.push_back(sparseRows.size());
            sparseRows.insert(sparseRows.end(), column.rows.begin(), column.rows.end());
        }
        if (kind == Kind::Integer)
        {
            starts.push_back(integers.size());
            if (constant)
                integers.push_back(column.integers.front());
            else
                integers.insert(integers.end(), column.integers.begin(), column.integers.end());
        }
        else if (kind == Kind::Real)
        {
            starts.push_back(reals.size());
            if (constant)
                reals.push_back(column.reals.front());
            else
                reals.insert(reals.end(), column.reals.begin(), column.reals.end());
        }
        else
        {
            starts.push_back(textCodes.size());
            if (constant)
                textCodes.push_back(column.texts.front());
            else
                textCodes.insert(textCodes.end(), column.texts.begin(), column.texts.end());
        }
        column = {}; // release each source column as soon as its packed values are ready
    }
    Output out(compression, shuffle);
    out.numbers(path + "/keys", std::move(keys));
    out.numbers(path + "/ids", std::move(ids));
    out.numbers(path + "/field_counts", std::move(counts));
    out.numbers(path + "/columns/slots", std::move(slots));
    out.numbers(path + "/columns/kinds", std::move(kinds));
    out.numbers(path + "/columns/value_offsets", std::move(starts));
    out.numbers(path + "/columns/counts", std::move(lengths));
    out.numbers(path + "/columns/row_offsets", std::move(rowStarts));
    out.numbers(path + "/row_indices", std::move(sparseRows));
    out.numbers(path + "/integers", std::move(integers));
    out.numbers(path + "/reals", std::move(reals));
    out.numbers(path + "/text_codes", std::move(textCodes));
    out.strings(path + "/strings", dictionary.values);
    out.numbers(path + "/precision/offsets", std::move(digitOffsets));
    out.numbers(path + "/precision/digits", std::move(digits));
    if (std::all_of(defaultMasks.begin(), defaultMasks.end(), [](auto mask) { return mask == 0; }))
        defaultMasks.clear();
    out.numbers(path + "/default_masks", std::move(defaultMasks));
    out.append(file);
}

class ControlOutput final : public model::RecordOutput
{
public:
    std::vector<deck::ModelItem> items;
    void comments(const std::string& text) override
    {
        if (!m_pending.empty())
            m_pending.push_back('\n');
        m_pending += text;
    }
    void record(const std::vector<std::string>& values) override
    {
        deck::ModelItem item;
        item.values = values;
        item.comments = std::exchange(m_pending, {});
        items.push_back(std::move(item));
    }
    void includeBegin(std::uint32_t index) override
    {
        deck::ModelItem item;
        item.kind = deck::ModelItemKind::IncludeBegin;
        item.includedFileIndex = index;
        item.comments = std::exchange(m_pending, {});
        items.push_back(std::move(item));
    }
    void includeEnd() override
    {
        finish();
        deck::ModelItem item;
        item.kind = deck::ModelItemKind::IncludeEnd;
        items.push_back(std::move(item));
    }
    void dropped(const std::string& what) override
    {
        throw base::UsageError("archive model: control data cannot be stored: " + what);
    }
    void sectionComment(const std::string& text)
    {
        deck::ModelItem item;
        item.kind = deck::ModelItemKind::Comments;
        item.comments = text;
        items.push_back(std::move(item));
    }
    void finish()
    {
        if (!m_pending.empty())
            sectionComment(std::exchange(m_pending, {}));
    }

private:
    std::string m_pending;
};

void writeControl(Output& out, const std::string& path, const std::vector<deck::ModelItem>& items)
{
    std::vector<std::uint8_t> kinds;
    std::vector<std::uint32_t> files;
    std::vector<std::string> comments, values;
    std::vector<std::uint64_t> offsets{0};
    for (const auto& item : items)
    {
        kinds.push_back(static_cast<std::uint8_t>(item.kind));
        files.push_back(item.includedFileIndex);
        comments.push_back(item.comments);
        values.insert(values.end(), item.values.begin(), item.values.end());
        offsets.push_back(values.size());
    }
    out.numbers(path + "/kinds", std::move(kinds));
    out.numbers(path + "/files", std::move(files));
    out.strings(path + "/comments", comments);
    out.strings(path + "/values", values);
    out.numbers(path + "/value_offsets", std::move(offsets));
}

std::vector<deck::ModelItem> readControl(const std::filesystem::path& file, const std::string& path,
                                        std::size_t fileCount)
{
    const auto kinds = numbers<std::uint8_t>(file, path + "/kinds");
    const auto files = numbers<std::uint32_t>(file, path + "/files");
    const auto comments = strings(file, path + "/comments");
    const auto values = strings(file, path + "/values");
    const auto offsets = numbers<std::uint64_t>(file, path + "/value_offsets");
    offsetsValid(offsets, values.size(), path);
    require(kinds.size() == files.size() && kinds.size() == comments.size() && offsets.size() == kinds.size() + 1,
            path + " has inconsistent record lengths");
    std::vector<deck::ModelItem> items(kinds.size());
    std::size_t depth = 0;
    for (std::size_t i = 0; i < items.size(); ++i)
    {
        auto& item = items[i];
        item.kind = static_cast<deck::ModelItemKind>(kinds[i]);
        item.comments = comments[i];
        item.includedFileIndex = files[i];
        item.values.assign(values.begin() + offsets[i], values.begin() + offsets[i + 1]);
        if (item.kind == deck::ModelItemKind::IncludeBegin)
        {
            require(files[i] < fileCount, path + " has an invalid included file");
            ++depth;
        }
        else if (item.kind == deck::ModelItemKind::IncludeEnd)
        {
            require(depth > 0, path + " has an unmatched Include end");
            --depth;
        }
        else
            require(item.kind == deck::ModelItemKind::Comments ||
                        (item.kind == deck::ModelItemKind::Record && !item.values.empty()),
                    path + " has an invalid record");
    }
    require(depth == 0, path + " has an unclosed Include");
    return items;
}

void writePart(const std::filesystem::path& file, const std::string& path, const model::Model& part,
               int compression, bool shuffle)
{
    Output out(compression, shuffle);
    out.numbers<std::uint32_t>(path + "/next_key", {part.nextKeyValue()});
    std::vector<std::string> names;
    std::vector<std::uint16_t> types;
    const auto& registry = model::CardRegistry::instance();
    for (std::uint16_t type = 0; type < registry.size(); ++type)
        if (const auto* table = part.findTable(type); table && table->rows().aliveCount())
        {
            names.emplace_back(registry.entry(type).info.name);
            types.push_back(type);
        }
    out.strings(path + "/card_types", names);
    std::vector<std::uint32_t> commentKeys, markerKeys, markerFiles, placementKeys, placementFiles;
    std::vector<std::uint8_t> markerKinds;
    std::vector<std::string> comments, markerTexts;
    for (const auto& comment : part.comments())
    {
        commentKeys.push_back(comment.key.value);
        comments.push_back(comment.text);
    }
    for (const auto& marker : part.markers())
    {
        markerKeys.push_back(marker.atKey);
        markerFiles.push_back(marker.fileIndex);
        markerKinds.push_back(static_cast<std::uint8_t>(marker.kind));
        markerTexts.push_back(marker.text);
    }
    // Stable order makes repeated exports deterministic apart from the root timestamp.
    const std::map<std::uint32_t, std::uint32_t> placements(part.placements().begin(), part.placements().end());
    for (const auto& [key, index] : placements)
    {
        placementKeys.push_back(key);
        placementFiles.push_back(index);
    }
    out.numbers(path + "/comments/keys", std::move(commentKeys));
    out.strings(path + "/comments/text", comments);
    std::vector<std::string> documentNames, documentTexts;
    for (const auto& [name, text] : part.documents())
    {
        documentNames.push_back(name);
        documentTexts.push_back(text);
    }
    out.strings(path + "/documents/names", documentNames);
    out.strings(path + "/documents/text", documentTexts);
    out.numbers(path + "/markers/keys", std::move(markerKeys));
    out.numbers(path + "/markers/files", std::move(markerFiles));
    out.numbers(path + "/markers/kinds", std::move(markerKinds));
    out.strings(path + "/markers/text", markerTexts);
    out.numbers(path + "/placements/keys", std::move(placementKeys));
    out.numbers(path + "/placements/files", std::move(placementFiles));
    ControlOutput executive, caseControl;
    auto executiveSection = part.control().executive;
    if (auto& entries = executiveSection.entries();
        !entries.empty() && entries.front().kind == model::ControlKind::Comment)
    {
        executive.sectionComment(entries.front().text);
        entries.erase(entries.begin());
    }
    auto caseSection = part.control().caseControl;
    if (auto& entries = caseSection.global().entries;
        !entries.empty() && entries.front().kind == model::EntryKind::Comment)
    {
        caseControl.sectionComment(entries.front().text);
        entries.erase(entries.begin());
    }
    model::ControlRecords::writeFileAssignments(part.control().fileManagement, executive);
    model::ControlRecords::writeExecutive(executiveSection, executive);
    model::ControlRecords::writeCase(caseSection, caseControl);
    executive.finish();
    caseControl.finish();
    writeControl(out, path + "/control/executive", executive.items);
    writeControl(out, path + "/control/case", caseControl.items);
    out.append(file);
    for (std::size_t i = 0; i < types.size(); ++i)
        writeCards(file, path + "/cards/" + std::to_string(i), *part.findTable(types[i]), compression, shuffle);
}

struct SavedKey final : model::KeySource
{
    base::CardKey key;
    bool used = false;
    base::CardKey next() override
    {
        require(!used, "one stored card expands to several rows");
        used = true;
        return key;
    }
};

void readCards(const std::filesystem::path& file, const std::string& path, model::Model& part, std::uint16_t type,
               std::vector<std::uint32_t>& allKeys)
{
    const auto keys = numbers<std::uint32_t>(file, path + "/keys");
    const auto ids = numbers<std::int32_t>(file, path + "/ids");
    const auto counts = numbers<std::uint32_t>(file, path + "/field_counts");
    const auto slots = numbers<std::uint32_t>(file, path + "/columns/slots");
    const auto kinds = numbers<std::uint8_t>(file, path + "/columns/kinds");
    const auto starts = numbers<std::uint64_t>(file, path + "/columns/value_offsets");
    const auto lengths = numbers<std::uint64_t>(file, path + "/columns/counts");
    const auto rowStarts = numbers<std::uint64_t>(file, path + "/columns/row_offsets");
    const auto sparseRows = numbers<std::uint32_t>(file, path + "/row_indices");
    const auto integers = numbers<std::int64_t>(file, path + "/integers");
    const auto reals = numbers<double>(file, path + "/reals");
    const auto codes = numbers<std::uint32_t>(file, path + "/text_codes");
    const auto dictionary = strings(file, path + "/strings");
    const auto digitOffsets = numbers<std::uint64_t>(file, path + "/precision/offsets");
    const auto digits = numbers<std::uint8_t>(file, path + "/precision/digits");
    const auto defaultMasks = numbers<std::uint8_t>(file, path + "/default_masks");
    require(keys.size() == ids.size() && keys.size() == counts.size() && keys.size() < base::CardKey::INVALID_VALUE,
            path + " has inconsistent row lengths");
    require(slots.size() == kinds.size() && slots.size() == starts.size() && slots.size() == lengths.size() &&
                slots.size() == rowStarts.size(), path + " has inconsistent column lengths");
    offsetsValid(digitOffsets, digits.size(), path + "/precision");
    require(digitOffsets.size() == keys.size() + 1, path + " has inconsistent precision rows");
    require(defaultMasks.empty() || defaultMasks.size() == keys.size(), path + " has inconsistent defaults rows");
    for (std::size_t row = 0; row < keys.size(); ++row)
    {
        require(keys[row] != base::CardKey::INVALID_VALUE && (row == 0 || keys[row] > keys[row - 1]),
                path, " has unordered or duplicate card keys");
        require(counts[row] <= MAX_FIELDS, path, " has too many fields in a card");
    }
    allKeys.insert(allKeys.end(), keys.begin(), keys.end());
    std::pair<std::uint32_t, std::uint8_t> previous{0, 0};
    for (std::size_t c = 0; c < slots.size(); ++c)
    {
        const auto current = std::pair(slots[c], kinds[c]);
        require(slots[c] > 0 && slots[c] <= MAX_FIELDS && current > previous, path + " has invalid column ordering");
        previous = current;
        const Kind kind = static_cast<Kind>(kinds[c]);
        require(kind == Kind::Integer || kind == Kind::Real || kind == Kind::Text, path + " has an unknown field kind");
        const std::size_t size = kind == Kind::Integer ? integers.size()
                                : kind == Kind::Real  ? reals.size()
                                                      : codes.size();
        require(starts[c] <= size && lengths[c] <= size - starts[c], path + " has an invalid value slice");
        if (rowStarts[c] == DENSE)
            require(lengths[c] == keys.size(), path + " has an invalid dense column");
        else if (rowStarts[c] == CONSTANT)
            require(lengths[c] == 1 && !keys.empty(), path + " has an invalid constant column");
        else
        {
            require(rowStarts[c] <= sparseRows.size() && lengths[c] <= sparseRows.size() - rowStarts[c],
                    path + " has an invalid row slice");
            for (std::uint64_t i = 0; i < lengths[c]; ++i)
            {
                const auto row = sparseRows[rowStarts[c] + i];
                require(row < keys.size() && (i == 0 || row > sparseRows[rowStarts[c] + i - 1]),
                        path, " has unordered or invalid sparse rows");
            }
        }
    }
    model::CardTable& table = part.ensureTable(type);
    part.reserveIds(type, keys.size());
    const model::ColumnGrowth::Batch growth(keys.size());
    std::vector<std::uint64_t> cursors(slots.size(), 0);
    const auto name = model::CardRegistry::instance().entry(type).info.name;
    const std::string fileName = base::pathToUtf8(file);
    deck::RawCard raw;
    raw.name = *deck::CardName::fromText(name);
    std::vector<std::uint32_t> addedRows;
    for (std::size_t first = 0; first < keys.size(); first += READ_BATCH)
    {
        const std::size_t last = std::min(first + READ_BATCH, keys.size());
        std::vector<std::uint64_t> batchOffsets{0};
        std::uint64_t batchFields = 0;
        for (std::size_t row = first; row < last; ++row)
        {
            batchFields += counts[row];
            require(batchFields <= io::Hdf5Limits{}.maxReadBytes / sizeof(FieldValue),
                    "a card batch exceeds the byte limit");
            batchOffsets.push_back(batchFields);
        }
        std::vector<FieldValue> fields(static_cast<std::size_t>(batchFields));
        for (std::size_t c = 0; c < slots.size(); ++c)
        {
            auto& cursor = cursors[c];
            const bool constant = rowStarts[c] == CONSTANT;
            const std::uint64_t count = constant ? keys.size() : lengths[c];
            while (cursor < count)
            {
                const std::size_t row = rowStarts[c] >= CONSTANT ? cursor : sparseRows[rowStarts[c] + cursor];
                if (row >= last)
                    break;
                require(row >= first && slots[c] <= counts[row], path, " has a field outside its row");
                FieldValue& value = fields[batchOffsets[row - first] + slots[c] - 1];
                require(value.kind == Kind::Blank, path, " gives several values to one field");
                value.kind = static_cast<Kind>(kinds[c]);
                const auto index = starts[c] + (constant ? 0 : cursor);
                ++cursor;
                if (value.kind == Kind::Integer)
                    value.integer = integers[index];
                else if (value.kind == Kind::Real)
                    value.real = reals[index];
                else
                {
                    require(codes[index] < dictionary.size(), path, " has an invalid text code");
                    value.text = dictionary[codes[index]];
                }
            }
        }
        for (std::size_t row = first; row < last; ++row)
        {
            const auto rowFields = std::span<const FieldValue>(fields).subspan(batchOffsets[row - first], counts[row]);
            std::uint32_t added = 0;
            if (type == model::CardRegistry::UNKNOWN_TYPE)
            {
                require(!rowFields.empty() && rowFields.front().kind == Kind::Text && !rowFields.front().text.empty(),
                        path + " has an unknown card without its name");
                std::vector<std::string_view> texts;
                for (std::size_t i = 1; i < rowFields.size(); ++i)
                {
                    require(rowFields[i].kind == Kind::Text || rowFields[i].kind == Kind::Blank,
                            path + " has a numeric field in an unknown card");
                    texts.push_back(rowFields[i].text);
                }
                const auto unknownName = deck::CardName::fromText(rowFields.front().text);
                require(unknownName.has_value(), path + " has an invalid unknown card name");
                added = static_cast<model::UnknownCardTable&>(table).append(
                    base::CardKey{keys[row]}, *unknownName, texts);
            }
            else
            {
                model::FieldReader reader(rowFields, raw, part.diagnostics(), fileName);
                SavedKey key;
                key.key = base::CardKey{keys[row]};
                require(table.readCards(reader, key, addedRows) && !reader.failed() && addedRows.size() == 1,
                        path, " contains an invalid typed card");
                added = addedRows.front();
                part.registerRead(model::CardLocation{type, added}, table.idAt(added), part.diagnostics(),
                                  fileName, 0);
            }
            require(table.idAt(added) == ids[row], path, " has an ID inconsistent with its fields");
            if (!defaultMasks.empty())
                restoreDefaultMask(table, added, defaultMasks[row]);
            table.rows().setSource(added, std::span<const std::uint8_t>(digits).subspan(
                                              digitOffsets[row], digitOffsets[row + 1] - digitOffsets[row]));
        }
    }
}

void readPart(const std::filesystem::path& file, const std::string& path, model::Model& part,
              const model::ModelReadOptions& options)
{
    part.setStrict(options.strict);
    const auto next = numbers<std::uint32_t>(file, path + "/next_key");
    require(next.size() == 1, path + " must have one key counter");
    const auto types = strings(file, path + "/card_types");
    std::vector<std::uint32_t> keys;
    std::unordered_set<std::uint16_t> seenTypes;
    for (std::size_t i = 0; i < types.size(); ++i)
    {
        const auto type = model::CardRegistry::instance().typeIndexOf(types[i]);
        require(type && seenTypes.insert(*type).second, path + " has an unknown or duplicate card type");
        readCards(file, path + "/cards/" + std::to_string(i), part, *type, keys);
    }
    std::sort(keys.begin(), keys.end());
    require(std::adjacent_find(keys.begin(), keys.end()) == keys.end(), path + " has duplicate card keys");
    require(keys.empty() || keys.back() < next.front(), path + " has an invalid key counter");
    part.restoreNextKey(next.front());
    const auto commentKeys = numbers<std::uint32_t>(file, path + "/comments/keys");
    const auto comments = strings(file, path + "/comments/text");
    require(commentKeys.size() == comments.size() && std::is_sorted(commentKeys.begin(), commentKeys.end()),
            path + " has invalid comments");
    for (std::size_t i = 0; i < comments.size(); ++i)
    {
        require(commentKeys[i] < next.front(), path + " has an invalid comment key");
        part.comments().push_back({base::CardKey{commentKeys[i]}, comments[i]});
    }
    const auto markerKeys = numbers<std::uint32_t>(file, path + "/markers/keys");
    const auto markerFiles = numbers<std::uint32_t>(file, path + "/markers/files");
    const auto markerKinds = numbers<std::uint8_t>(file, path + "/markers/kinds");
    const auto markerTexts = strings(file, path + "/markers/text");
    require(markerKeys.size() == markerFiles.size() && markerKeys.size() == markerKinds.size() &&
                markerKeys.size() == markerTexts.size() && std::is_sorted(markerKeys.begin(), markerKeys.end()),
            path + " has invalid source markers");
    for (std::size_t i = 0; i < markerKeys.size(); ++i)
    {
        require(markerKeys[i] <= next.front() &&
                    markerKinds[i] <= static_cast<std::uint8_t>(model::SourceMarkerKind::Comments) &&
                    (markerKinds[i] != static_cast<std::uint8_t>(model::SourceMarkerKind::IncludeBegin) ||
                     markerFiles[i] < part.root().files().size()), path + " has an invalid source marker");
        part.markers().push_back({markerKeys[i], static_cast<model::SourceMarkerKind>(markerKinds[i]),
                                  markerTexts[i], markerFiles[i]});
    }
    const auto placementKeys = numbers<std::uint32_t>(file, path + "/placements/keys");
    const auto placementFiles = numbers<std::uint32_t>(file, path + "/placements/files");
    require(placementKeys.size() == placementFiles.size(), path + " has invalid placements");
    for (std::size_t i = 0; i < placementKeys.size(); ++i)
    {
        require(placementKeys[i] < next.front() && placementFiles[i] < part.root().files().size(),
                path + " has an invalid placement");
        part.placements().emplace(placementKeys[i], placementFiles[i]);
    }
    std::vector<std::string> fileNames;
    for (const auto& source : part.root().files())
        fileNames.push_back(base::pathToUtf8(source.path));
    auto& control = part.control();
    // Optional extension: archives written before application documents have no such group.
    {
        const auto handle = io::Hdf5File::openReadOnly(file);
        if (handle.hasChild(path.substr(1) + "/documents"))
        {
            const auto names = strings(file, path + "/documents/names");
            const auto texts = strings(file, path + "/documents/text");
            require(names.size() == texts.size(), path + " has invalid documents");
            for (std::size_t i = 0; i < names.size(); ++i)
            {
                require(!part.documents().contains(names[i]), path + " has duplicate document names");
                part.setDocument(names[i], texts[i]);
            }
        }
    }
    const auto executive = readControl(file, path + "/control/executive", fileNames.size());
    const auto caseControl = readControl(file, path + "/control/case", fileNames.size());
    control.executive =
        model::ControlRecords::readExecutive(executive, fileNames, part.diagnostics(), control.fileManagement);
    control.caseControl = model::ControlRecords::readCase(caseControl, fileNames, part.diagnostics());
}

} // namespace

void writeModel(const std::filesystem::path& file, const model::Model& model, int compression, bool shuffle)
{
    std::vector<const model::Model*> parts{&model};
    std::vector<std::int32_t> parents{-1};
    std::vector<std::string> kinds{std::string{}};
    std::vector<std::int64_t> ids{0}, massIds{0};
    std::vector<std::uint8_t> appends{0};
    for (std::size_t parent = 0; parent < parts.size(); ++parent)
        for (const auto& part : parts[parent]->parts())
        {
            const auto& info = *part->partInfo();
            parts.push_back(part.get());
            parents.push_back(static_cast<std::int32_t>(parent));
            kinds.push_back(info.kind);
            ids.push_back(info.id);
            massIds.push_back(info.massId);
            appends.push_back(info.appends ? 1 : 0);
        }
    Output out(compression, shuffle);
    out.group(std::string(MODEL_GROUP), {{ATTRIBUTE_FORMAT, MODEL_FORMAT}, {ATTRIBUTE_FORMAT_VERSION, MODEL_VERSION}});
    // The small schema describes neutral field names, alternatives and repeated lists for independent readers.
    // Loading a model uses the versioned slot contract directly and never parses this JSON.
    out.strings(std::string(MODEL_CARD_SCHEMA), {std::string(model::cardLayoutsText())});
    const std::string root(MODEL_GROUP);
    out.numbers(root + "/index/parents", std::move(parents));
    out.numbers(root + "/index/ids", std::move(ids));
    out.numbers(root + "/index/mass_ids", std::move(massIds));
    out.numbers(root + "/index/appends", std::move(appends));
    out.strings(root + "/index/kinds", kinds);
    std::vector<std::string> paths, includeText;
    std::vector<std::int32_t> fileParents;
    std::vector<std::uint32_t> lines;
    for (const auto& source : model.files())
    {
        paths.push_back(base::pathToUtf8(source.path));
        includeText.push_back(source.includeText);
        fileParents.push_back(source.parent);
        lines.push_back(source.includeLine);
    }
    out.strings(root + "/sources/paths", paths);
    out.strings(root + "/sources/includes", includeText);
    out.numbers(root + "/sources/parents", std::move(fileParents));
    out.numbers(root + "/sources/lines", std::move(lines));
    out.append(file);
    for (std::size_t i = 0; i < parts.size(); ++i)
        writePart(file, partPath(i), *parts[i], compression, shuffle);
}

std::shared_ptr<model::Model> readModel(const std::filesystem::path& file, const model::ModelReadOptions& options)
{
    {
        const auto handle = io::Hdf5File::openReadOnly(file);
        require(handle.readStringAttribute(std::string(ATTRIBUTE_FORMAT)) == FORMAT_NAME,
                "the file is not an O_o archive");
        const auto version = handle.readStringAttribute(std::string(ATTRIBUTE_FORMAT_VERSION));
        require(version == FORMAT_VERSION || version == LEGACY_FORMAT_VERSION, "unsupported archive version");
        require(handle.hasChild(std::string(MODEL_GROUP).substr(1)), "the archive contains no model");
        const auto group = handle.openGroup(std::string(MODEL_GROUP));
        require(group.readStringAttribute(std::string(ATTRIBUTE_FORMAT)) == MODEL_FORMAT &&
                    group.readStringAttribute(std::string(ATTRIBUTE_FORMAT_VERSION)) == MODEL_VERSION,
                "unsupported model schema");
    }
    const std::string root(MODEL_GROUP);
    const auto parents = numbers<std::int32_t>(file, root + "/index/parents");
    const auto ids = numbers<std::int64_t>(file, root + "/index/ids");
    const auto massIds = numbers<std::int64_t>(file, root + "/index/mass_ids");
    const auto appends = numbers<std::uint8_t>(file, root + "/index/appends");
    const auto kinds = strings(file, root + "/index/kinds");
    require(!parents.empty() && parents.front() == -1 && parents.size() == ids.size() &&
                parents.size() == massIds.size() &&
                parents.size() == appends.size() && parents.size() == kinds.size(), "invalid part index");
    auto model = std::make_shared<model::Model>();
    std::vector<model::Model*> parts{model.get()};
    for (std::size_t i = 1; i < parents.size(); ++i)
    {
        require(parents[i] >= 0 && static_cast<std::size_t>(parents[i]) < i && !kinds[i].empty() && appends[i] <= 1,
                "invalid part parent or kind");
        auto& part = parts[parents[i]]->addPart(model::PartInfo{kinds[i], ids[i], massIds[i], appends[i] != 0});
        parts.push_back(&part);
    }
    const auto paths = strings(file, root + "/sources/paths");
    const auto includes = strings(file, root + "/sources/includes");
    const auto fileParents = numbers<std::int32_t>(file, root + "/sources/parents");
    const auto lines = numbers<std::uint32_t>(file, root + "/sources/lines");
    require(paths.size() == includes.size() && paths.size() == fileParents.size() && paths.size() == lines.size(),
            "invalid source index");
    for (std::size_t i = 0; i < paths.size(); ++i)
    {
        require((i == 0 && fileParents[i] == -1) ||
                    (i > 0 && fileParents[i] >= 0 && static_cast<std::size_t>(fileParents[i]) < i),
                "invalid source parent");
        model->files().push_back({base::pathFromUtf8(paths[i]), includes[i], fileParents[i], lines[i]});
    }
    for (std::size_t i = 0; i < parts.size(); ++i)
        readPart(file, partPath(i), *parts[i], options);
    return model;
}

} // namespace oor::results::archive
