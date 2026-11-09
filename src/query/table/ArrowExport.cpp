#include "oor/query/table/ArrowExport.hpp"

#include "oor/query/QueryErrors.hpp"

#include <cstring>
#include <vector>

namespace oor::query
{

namespace
{

// A pointer for a buffer that has no bytes: consumers read the pointer of a zero-length buffer (they must not read
// through it), and some of them refuse a null one.
const std::uint8_t ZERO_BYTE[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// --- schema ----------------------------------------------------------------------------------------------------------

struct SchemaPrivate
{
    std::string name;
    std::string format;
    std::string metadata; // the binary form of the Arrow key/value metadata, empty for none
    std::vector<ArrowSchema*> children;
    ArrowSchema* dictionary = nullptr;
};

void releaseSchema(ArrowSchema* schema)
{
    if (schema == nullptr || schema->release == nullptr)
        return;
    auto* data = static_cast<SchemaPrivate*>(schema->private_data);
    for (ArrowSchema* child : data->children)
    {
        if (child->release != nullptr)
            child->release(child);
        delete child;
    }
    if (data->dictionary != nullptr)
    {
        if (data->dictionary->release != nullptr)
            data->dictionary->release(data->dictionary);
        delete data->dictionary;
    }
    delete data;
    schema->release = nullptr;
}

void appendInt32(std::string& bytes, std::int32_t value)
{
    bytes.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

// The Arrow binary form of metadata: the number of pairs, then for each key and value its length and its bytes.
std::string encodeMetadata(const std::vector<std::pair<std::string, std::string>>& pairs)
{
    std::string bytes;
    if (pairs.empty())
        return bytes;
    appendInt32(bytes, static_cast<std::int32_t>(pairs.size()));
    for (const auto& [key, value] : pairs)
    {
        appendInt32(bytes, static_cast<std::int32_t>(key.size()));
        bytes += key;
        appendInt32(bytes, static_cast<std::int32_t>(value.size()));
        bytes += value;
    }
    return bytes;
}

void fillSchema(ArrowSchema* out, std::string name, std::string format, std::string metadata,
                std::vector<ArrowSchema*> children, ArrowSchema* dictionary)
{
    auto* data = new SchemaPrivate;
    data->name = std::move(name);
    data->format = std::move(format);
    data->metadata = std::move(metadata);
    data->children = std::move(children);
    data->dictionary = dictionary;
    out->format = data->format.c_str();
    out->name = data->name.c_str();
    out->metadata = data->metadata.empty() ? nullptr : data->metadata.data();
    out->flags = ARROW_FLAG_NULLABLE;
    out->n_children = static_cast<std::int64_t>(data->children.size());
    out->children = data->children.empty() ? nullptr : data->children.data();
    out->dictionary = dictionary;
    out->release = releaseSchema;
    out->private_data = data;
}

std::string primitiveFormat(ColumnType type)
{
    switch (type)
    {
    case ColumnType::Bool:
        return "b";
    case ColumnType::UInt8:
        return "C";
    case ColumnType::Int32:
        return "i";
    case ColumnType::UInt32:
        return "I";
    case ColumnType::Int64:
        return "l";
    case ColumnType::Float32:
        return "f";
    case ColumnType::Float64:
        return "g";
    case ColumnType::Utf8:
        return "u";
    default:
        return "";
    }
}

// --- array -----------------------------------------------------------------------------------------------------------

struct ArrayPrivate
{
    std::vector<std::shared_ptr<const void>> owners; // what keeps the buffers alive
    std::vector<const void*> buffers;
    std::vector<ArrowArray*> children;
    ArrowArray* dictionary = nullptr;
};

void releaseArray(ArrowArray* array)
{
    if (array == nullptr || array->release == nullptr)
        return;
    auto* data = static_cast<ArrayPrivate*>(array->private_data);
    for (ArrowArray* child : data->children)
    {
        if (child->release != nullptr)
            child->release(child);
        delete child;
    }
    if (data->dictionary != nullptr)
    {
        if (data->dictionary->release != nullptr)
            data->dictionary->release(data->dictionary);
        delete data->dictionary;
    }
    delete data;
    array->release = nullptr;
}

const void* pointerOf(const ColumnBuffer& buffer)
{
    return buffer.data() != nullptr ? buffer.data() : static_cast<const void*>(ZERO_BYTE);
}

void keep(ArrayPrivate& data, const ColumnBuffer& buffer)
{
    if (buffer.owner())
        data.owners.push_back(buffer.owner());
}

void fillArray(ArrowArray* out, std::int64_t length, std::int64_t nullCount, ArrayPrivate* data)
{
    out->length = length;
    out->null_count = nullCount;
    out->offset = 0;
    out->n_buffers = static_cast<std::int64_t>(data->buffers.size());
    out->n_children = static_cast<std::int64_t>(data->children.size());
    out->buffers = data->buffers.data();
    out->children = data->children.empty() ? nullptr : data->children.data();
    out->dictionary = data->dictionary;
    out->release = releaseArray;
    out->private_data = data;
}

} // namespace

std::string ArrowExport::formatOf(const QueryColumn& column)
{
    switch (column.type())
    {
    case ColumnType::Dictionary:
        return "i";
    case ColumnType::FixedList:
        return "+w:" + std::to_string(column.width());
    case ColumnType::List:
        return "+L";
    default:
        return primitiveFormat(column.type());
    }
}

void ArrowExport::exportColumnSchema(const std::string& name, const QueryColumn& column, ArrowSchema* out)
{
    std::vector<ArrowSchema*> children;
    ArrowSchema* dictionary = nullptr;
    if (column.type() == ColumnType::FixedList || column.type() == ColumnType::List)
    {
        auto* child = new ArrowSchema{};
        exportColumnSchema("item", *column.child(), child);
        children.push_back(child);
    }
    else if (column.type() == ColumnType::Dictionary)
    {
        dictionary = new ArrowSchema{};
        exportColumnSchema("", *column.child(), dictionary);
    }
    fillSchema(out, name, formatOf(column), {}, std::move(children), dictionary);
}

void ArrowExport::exportSchema(const QueryTable& table, ArrowSchema* out)
{
    std::vector<ArrowSchema*> children;
    children.reserve(table.columnCount());
    try
    {
        for (std::size_t index = 0; index < table.columnCount(); ++index)
        {
            auto* child = new ArrowSchema{};
            exportColumnSchema(table.name(index), table.column(index), child);
            children.push_back(child);
        }
    }
    catch (...)
    {
        for (ArrowSchema* child : children)
        {
            if (child->release != nullptr)
                child->release(child);
            delete child;
        }
        throw;
    }
    std::vector<std::pair<std::string, std::string>> metadata;
    if (table.contractVersion() > 0)
        metadata.emplace_back("oor_reader.data_contract_version", std::to_string(table.contractVersion()));
    fillSchema(out, "", "+s", encodeMetadata(metadata), std::move(children), nullptr);
}

void ArrowExport::exportColumnArray(const QueryColumn& column, ArrowArray* out)
{
    auto* data = new ArrayPrivate;
    try
    {
        // Buffer 0 of every type is the validity bitmap; it is null for a column that has no null.
        data->buffers.push_back(column.hasNulls() ? pointerOf(column.validity()) : nullptr);
        keep(*data, column.validity());
        switch (column.type())
        {
        case ColumnType::Utf8:
            data->buffers.push_back(pointerOf(column.offsets()));
            data->buffers.push_back(pointerOf(column.values()));
            keep(*data, column.offsets());
            keep(*data, column.values());
            break;
        case ColumnType::FixedList:
        {
            auto* child = new ArrowArray{};
            data->children.push_back(child);
            exportColumnArray(*column.child(), child);
            break;
        }
        case ColumnType::List:
        {
            data->buffers.push_back(pointerOf(column.offsets()));
            keep(*data, column.offsets());
            auto* child = new ArrowArray{};
            data->children.push_back(child);
            exportColumnArray(*column.child(), child);
            break;
        }
        case ColumnType::Dictionary:
        {
            data->buffers.push_back(pointerOf(column.values()));
            keep(*data, column.values());
            data->dictionary = new ArrowArray{};
            exportColumnArray(*column.child(), data->dictionary);
            break;
        }
        default:
            data->buffers.push_back(pointerOf(column.values()));
            keep(*data, column.values());
            break;
        }
    }
    catch (...)
    {
        ArrowArray partial{};
        fillArray(&partial, 0, 0, data);
        releaseArray(&partial);
        throw;
    }
    fillArray(out, column.length(), column.nullCount(), data);
}

void ArrowExport::exportArray(const QueryTable& table, ArrowArray* out)
{
    auto* data = new ArrayPrivate;
    data->buffers.push_back(nullptr); // a struct array has a validity buffer and no null
    try
    {
        for (std::size_t index = 0; index < table.columnCount(); ++index)
        {
            auto* child = new ArrowArray{};
            data->children.push_back(child);
            exportColumnArray(table.column(index), child);
        }
    }
    catch (...)
    {
        ArrowArray partial{};
        fillArray(&partial, 0, 0, data);
        releaseArray(&partial);
        throw;
    }
    fillArray(out, table.rowCount(), 0, data);
}

namespace
{

struct StreamPrivate
{
    std::shared_ptr<const QueryTable> table;
    bool delivered = false;
    std::string lastError;
};

int streamSchema(ArrowArrayStream* stream, ArrowSchema* out)
{
    auto* data = static_cast<StreamPrivate*>(stream->private_data);
    try
    {
        ArrowExport::exportSchema(*data->table, out);
        return 0;
    }
    catch (const std::exception& error)
    {
        data->lastError = error.what();
        return 5; // EIO
    }
}

int streamNext(ArrowArrayStream* stream, ArrowArray* out)
{
    auto* data = static_cast<StreamPrivate*>(stream->private_data);
    if (data->delivered)
    {
        out->release = nullptr; // the end of the stream
        return 0;
    }
    try
    {
        ArrowExport::exportArray(*data->table, out);
        data->delivered = true;
        return 0;
    }
    catch (const std::exception& error)
    {
        data->lastError = error.what();
        return 5;
    }
}

const char* streamError(ArrowArrayStream* stream)
{
    auto* data = static_cast<StreamPrivate*>(stream->private_data);
    return data->lastError.empty() ? nullptr : data->lastError.c_str();
}

void streamRelease(ArrowArrayStream* stream)
{
    if (stream == nullptr || stream->release == nullptr)
        return;
    delete static_cast<StreamPrivate*>(stream->private_data);
    stream->release = nullptr;
}

} // namespace

void ArrowExport::exportStream(std::shared_ptr<const QueryTable> table, ArrowArrayStream* out)
{
    auto* data = new StreamPrivate;
    data->table = std::move(table);
    out->get_schema = streamSchema;
    out->get_next = streamNext;
    out->get_last_error = streamError;
    out->release = streamRelease;
    out->private_data = data;
}

} // namespace oor::query
