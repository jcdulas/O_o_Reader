#include "oor/query/table/TablePackage.hpp"

#include "oor/base/arrow/ArrowAbi.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/table/ArrowExport.hpp"

#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace oor::query
{

namespace
{

constexpr char MAGIC[4] = {'O', 'O', 'T', 'P'};
constexpr std::uint32_t PACKAGE_VERSION = 1;
constexpr std::uint64_t NULL_BUFFER = std::numeric_limits<std::uint64_t>::max();
constexpr std::uint64_t BUFFER_ALIGNMENT = 8;

std::uint64_t alignUp(std::uint64_t value)
{
    return (value + BUFFER_ALIGNMENT - 1) / BUFFER_ALIGNMENT * BUFFER_ALIGNMENT;
}

// The bytes of an Arrow bitmap that covers `bits` slots.
std::uint64_t bitmapBytes(std::int64_t bits)
{
    return static_cast<std::uint64_t>((bits + 7) / 8);
}

// The width in bytes of a fixed-width primitive format, 0 for any other format.
std::uint64_t primitiveWidth(std::string_view format)
{
    if (format == "c" || format == "C")
        return 1;
    if (format == "s" || format == "S" || format == "e")
        return 2;
    if (format == "i" || format == "I" || format == "f")
        return 4;
    if (format == "l" || format == "L" || format == "g")
        return 8;
    return 0;
}

// The last offset of a variable-size layout (texts or lists): where the data of the last slot ends.
std::uint64_t lastOffset(const void* offsets, std::int64_t slots, bool isWide)
{
    if (offsets == nullptr)
        return 0;
    if (isWide)
    {
        std::int64_t value = 0;
        std::memcpy(&value, static_cast<const std::uint8_t*>(offsets) + slots * 8, sizeof(value));
        return static_cast<std::uint64_t>(value);
    }
    std::int32_t value = 0;
    std::memcpy(&value, static_cast<const std::uint8_t*>(offsets) + slots * 4, sizeof(value));
    return static_cast<std::uint64_t>(value);
}

// The byte length of each buffer of an array, from its format and its length: the Arrow C Data Interface does not
// carry buffer sizes, so they are those the format's layout implies (the slots before `offset` included, since the
// buffers are kept whole and the offset with them).
std::vector<std::uint64_t> bufferLengths(const ArrowSchema& schema, const ArrowArray& array)
{
    const std::string_view format = schema.format;
    const std::int64_t slots = array.offset + array.length;
    std::vector<std::uint64_t> lengths(static_cast<std::size_t>(array.n_buffers), 0);
    if (array.n_buffers == 0)
        return lengths;
    const auto set = [&](std::size_t index, std::uint64_t bytes)
    {
        if (index < lengths.size() && array.buffers[index] != nullptr)
            lengths[index] = bytes;
    };
    set(0, bitmapBytes(slots)); // the validity bitmap of every layout that has buffers
    if (const std::uint64_t width = primitiveWidth(format); width > 0)
    {
        // A dictionary-encoded column has the layout of its index type: the same rule.
        set(1, static_cast<std::uint64_t>(slots) * width);
        return lengths;
    }
    if (format == "b")
    {
        set(1, bitmapBytes(slots));
        return lengths;
    }
    if (format == "u" || format == "z" || format == "U" || format == "Z")
    {
        const bool isWide = format == "U" || format == "Z";
        set(1, static_cast<std::uint64_t>(slots + 1) * (isWide ? 8 : 4));
        set(2, lastOffset(array.buffers[1], slots, isWide));
        return lengths;
    }
    if (format == "+l" || format == "+L")
    {
        set(1, static_cast<std::uint64_t>(slots + 1) * (format == "+L" ? 8 : 4));
        return lengths;
    }
    if (format.rfind("+w:", 0) == 0 || format == "+s" || format == "n")
        return lengths;
    throw QueryError("cannot pack a column of the Arrow format '" + std::string(format) +
                     "': the table package knows the formats of the query tables only");
}

// Writes the header node by node, depth first, and gathers the buffers. The child arrays of a list hold more slots than
// the list itself: each node writes its own length, so nothing is derived from the parent.
class PackageWriter
{
public:
    void writeNode(const ArrowSchema& schema, const ArrowArray& array)
    {
        writeText(schema.name != nullptr ? schema.name : "");
        writeText(schema.format != nullptr ? schema.format : "");
        writeInteger<std::int64_t>(schema.flags);
        writeMetadata(schema.metadata);
        writeInteger<std::int64_t>(array.length);
        writeInteger<std::int64_t>(array.null_count);
        writeInteger<std::int64_t>(array.offset);
        const std::vector<std::uint64_t> lengths = bufferLengths(schema, array);
        writeInteger<std::uint32_t>(static_cast<std::uint32_t>(array.n_buffers));
        for (std::int64_t index = 0; index < array.n_buffers; ++index)
        {
            const void* buffer = array.buffers[index];
            if (buffer == nullptr)
            {
                writeInteger<std::uint64_t>(NULL_BUFFER);
                writeInteger<std::uint64_t>(0);
                continue;
            }
            const std::uint64_t length = lengths[static_cast<std::size_t>(index)];
            writeInteger<std::uint64_t>(addBuffer(buffer, length));
            writeInteger<std::uint64_t>(length);
        }
        if (schema.n_children != array.n_children)
            throw QueryError("cannot pack a table whose schema and array have different children");
        writeInteger<std::uint32_t>(static_cast<std::uint32_t>(array.n_children));
        for (std::int64_t index = 0; index < array.n_children; ++index)
            writeNode(*schema.children[index], *array.children[index]);
        const bool hasDictionary = schema.dictionary != nullptr && array.dictionary != nullptr;
        writeInteger<std::uint8_t>(hasDictionary ? 1 : 0);
        if (hasDictionary)
            writeNode(*schema.dictionary, *array.dictionary);
    }

    // The package: the magic, the header and the buffers.
    [[nodiscard]] std::vector<std::uint8_t> finish() const
    {
        std::vector<std::uint8_t> bytes;
        const std::uint64_t headerStart = sizeof(MAGIC) + sizeof(std::uint32_t) + sizeof(std::uint64_t);
        const std::uint64_t bufferStart = alignUp(headerStart + m_header.size());
        bytes.resize(bufferStart + m_buffers.size(), 0);
        std::memcpy(bytes.data(), MAGIC, sizeof(MAGIC));
        const std::uint32_t version = PACKAGE_VERSION;
        std::memcpy(bytes.data() + 4, &version, sizeof(version));
        const std::uint64_t headerBytes = m_header.size();
        std::memcpy(bytes.data() + 8, &headerBytes, sizeof(headerBytes));
        if (!m_header.empty())
            std::memcpy(bytes.data() + headerStart, m_header.data(), m_header.size());
        if (!m_buffers.empty())
            std::memcpy(bytes.data() + bufferStart, m_buffers.data(), m_buffers.size());
        return bytes;
    }

private:
    template <typename T>
    void writeInteger(T value)
    {
        // WebAssembly and x86-64 are little-endian: the bytes of the value are its little-endian form.
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
        m_header.insert(m_header.end(), bytes, bytes + sizeof(T));
    }

    void writeText(std::string_view text)
    {
        writeInteger<std::uint32_t>(static_cast<std::uint32_t>(text.size()));
        m_header.insert(m_header.end(), text.begin(), text.end());
    }

    // The Arrow metadata (int32 count, then int32-length keys and values) as package strings.
    void writeMetadata(const char* metadata)
    {
        if (metadata == nullptr)
        {
            writeInteger<std::uint32_t>(0);
            return;
        }
        std::int32_t count = 0;
        std::memcpy(&count, metadata, sizeof(count));
        writeInteger<std::uint32_t>(static_cast<std::uint32_t>(count));
        const char* cursor = metadata + sizeof(count);
        for (std::int32_t index = 0; index < 2 * count; ++index)
        {
            std::int32_t length = 0;
            std::memcpy(&length, cursor, sizeof(length));
            cursor += sizeof(length);
            writeText(std::string_view(cursor, static_cast<std::size_t>(length)));
            cursor += length;
        }
    }

    // Copies a buffer to the buffer area at an aligned position; returns that position.
    std::uint64_t addBuffer(const void* buffer, std::uint64_t length)
    {
        const std::uint64_t start = alignUp(m_buffers.size());
        m_buffers.resize(start + length, 0);
        if (length > 0)
            std::memcpy(m_buffers.data() + start, buffer, length);
        return start;
    }

    std::vector<std::uint8_t> m_header;
    std::vector<std::uint8_t> m_buffers;
};

// Releases the exported structures however the packing ends.
struct ExportedTable
{
    ArrowSchema schema{};
    ArrowArray array{};

    ExportedTable() = default;
    ExportedTable(const ExportedTable&) = delete;
    ExportedTable& operator=(const ExportedTable&) = delete;
    ~ExportedTable()
    {
        if (schema.release != nullptr)
            schema.release(&schema);
        if (array.release != nullptr)
            array.release(&array);
    }
};

} // namespace

std::vector<std::uint8_t> packTable(const QueryTable& table)
{
    ExportedTable exported;
    ArrowExport::exportSchema(table, &exported.schema);
    ArrowExport::exportArray(table, &exported.array);
    PackageWriter writer;
    writer.writeNode(exported.schema, exported.array);
    return writer.finish();
}

} // namespace oor::query
