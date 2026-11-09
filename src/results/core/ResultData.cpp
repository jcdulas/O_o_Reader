#include "oor/results/core/ResultData.hpp"

#include "oor/results/core/ResultDecodeStats.hpp"

#include <unordered_map>

namespace oor::results
{

std::string_view resultStatusName(ResultStatus status) noexcept
{
    switch (status)
    {
    case ResultStatus::Validated:
        return "validated";
    case ResultStatus::NotValidated:
        return "notValidated";
    case ResultStatus::Experimental:
        return "experimental";
    }
    return "notValidated";
}

std::string_view valueTypeName(ValueType type) noexcept
{
    switch (type)
    {
    case ValueType::Float32:
        return "float32";
    case ValueType::Float64:
        return "float64";
    case ValueType::Complex64:
        return "complex64";
    case ValueType::Complex128:
        return "complex128";
    }
    return "float32";
}

std::size_t valueTypeSize(ValueType type) noexcept
{
    switch (type)
    {
    case ValueType::Float32:
        return 4;
    case ValueType::Float64:
    case ValueType::Complex64:
        return 8;
    case ValueType::Complex128:
        return 16;
    }
    return 4;
}

bool isComplexType(ValueType type) noexcept
{
    return type == ValueType::Complex64 || type == ValueType::Complex128;
}

ResultText::ResultText(std::string labelName, const std::vector<std::string>& values) : name(std::move(labelName))
{
    codes.reserve(values.size());
    std::unordered_map<std::string, std::uint32_t> seen;
    for (const std::string& value : values)
    {
        const auto [position, inserted] = seen.try_emplace(value, static_cast<std::uint32_t>(dictionary.size()));
        if (inserted)
            dictionary.push_back(value);
        codes.push_back(position->second);
    }
}

std::vector<std::string> ResultText::values() const
{
    std::vector<std::string> expanded;
    expanded.reserve(codes.size());
    for (std::size_t row = 0; row < codes.size(); ++row)
        expanded.push_back(at(row));
    return expanded;
}

ResultText ResultText::gather(const std::vector<std::uint32_t>& positions) const
{
    ResultText result;
    result.name = name;
    result.dictionary = dictionary;
    result.codes.reserve(positions.size());
    for (const std::uint32_t position : positions)
        result.codes.push_back(codes[position]);
    return result;
}

void ResultData::allocate()
{
    const std::size_t doubles = (byteCount() + sizeof(double) - 1) / sizeof(double);
    storage = std::make_shared_for_overwrite<double[]>(doubles == 0 ? 1 : doubles);
    decodeStats().bytesAllocated.fetch_add(byteCount(), std::memory_order_relaxed);
}

} // namespace oor::results
