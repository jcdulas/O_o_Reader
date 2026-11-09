#include "oor/results/core/NamedResult.hpp"

#include "oor/results/core/ResultDecodeStats.hpp"

namespace oor::results
{

std::string_view arrayTypeName(ArrayType type) noexcept
{
    switch (type)
    {
    case ArrayType::Int32:
        return "int32";
    case ArrayType::Int64:
        return "int64";
    case ArrayType::Float32:
        return "float32";
    case ArrayType::Float64:
        return "float64";
    case ArrayType::Complex64:
        return "complex64";
    case ArrayType::Complex128:
        return "complex128";
    }
    return "float32";
}

std::size_t arrayTypeSize(ArrayType type) noexcept
{
    switch (type)
    {
    case ArrayType::Int32:
    case ArrayType::Float32:
        return 4;
    case ArrayType::Int64:
    case ArrayType::Float64:
    case ArrayType::Complex64:
        return 8;
    case ArrayType::Complex128:
        return 16;
    }
    return 4;
}

std::size_t ResultArray::valueCount() const noexcept
{
    std::size_t count = 1;
    for (const std::size_t extent : shape)
        count *= extent;
    return count;
}

void ResultArray::allocate()
{
    const std::size_t doubles = (byteCount() + sizeof(double) - 1) / sizeof(double);
    storage = std::make_shared_for_overwrite<double[]>(doubles == 0 ? 1 : doubles);
    decodeStats().bytesAllocated.fetch_add(byteCount(), std::memory_order_relaxed);
}

const ResultArray* NamedResultData::find(std::string_view name) const noexcept
{
    for (const ResultArray& array : arrays)
        if (array.name == name)
            return &array;
    return nullptr;
}

const ResultText* NamedResultData::findText(std::string_view name) const noexcept
{
    for (const ResultText& text : texts)
        if (text.name == name)
            return &text;
    return nullptr;
}

const std::string* NamedResultMeta::attribute(std::string_view key) const noexcept
{
    for (const auto& item : attributes)
        if (item.first == key)
            return &item.second;
    return nullptr;
}

NamedResult::NamedResult(NamedResultMeta meta, std::shared_ptr<const NamedResultSource> source)
    : m_meta(std::move(meta)), m_source(std::move(source))
{
}

NamedResult::NamedResult(NamedResultMeta meta, std::shared_ptr<const NamedResultData> data)
    : m_meta(std::move(meta)), m_data(std::move(data))
{
}

bool NamedResult::isDecoded() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_data != nullptr;
}

std::shared_ptr<const NamedResultData> NamedResult::data(const DecodeContext& context) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (m_data)
        return m_data;
    decodeStats().tablesDecoded.fetch_add(1, std::memory_order_relaxed);
    m_data = m_source->load(context);
    return m_data;
}

} // namespace oor::results
