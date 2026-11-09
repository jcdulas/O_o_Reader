#pragma once

#include "oor/base/errors/Errors.hpp"
#include "oor/results/core/NamedResult.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace oor::query
{

// The arrays and texts of a named result as vectors, whatever the type they are stored in (single precision widened to
// double, a complex array gives NaN: no query reads one).

[[nodiscard]] inline std::vector<double> arrayDoubles(const results::ResultArray& array)
{
    std::vector<double> values(array.valueCount());
    switch (array.type)
    {
    case results::ArrayType::Int32:
        for (std::size_t index = 0; index < values.size(); ++index)
            values[index] = array.values<std::int32_t>()[index];
        break;
    case results::ArrayType::Int64:
        for (std::size_t index = 0; index < values.size(); ++index)
            values[index] = static_cast<double>(array.values<std::int64_t>()[index]);
        break;
    case results::ArrayType::Float32:
        for (std::size_t index = 0; index < values.size(); ++index)
            values[index] = array.values<float>()[index];
        break;
    case results::ArrayType::Float64:
        values.assign(array.values<double>().begin(), array.values<double>().end());
        break;
    case results::ArrayType::Complex64:
    case results::ArrayType::Complex128:
        values.assign(values.size(), std::numeric_limits<double>::quiet_NaN());
        break;
    }
    return values;
}

[[nodiscard]] inline std::vector<std::int32_t> arrayInts(const results::ResultArray& array)
{
    std::vector<std::int32_t> values(array.valueCount());
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        switch (array.type)
        {
        case results::ArrayType::Int32:
            values[index] = array.values<std::int32_t>()[index];
            break;
        case results::ArrayType::Int64:
            values[index] = static_cast<std::int32_t>(array.values<std::int64_t>()[index]);
            break;
        case results::ArrayType::Float32:
            values[index] = static_cast<std::int32_t>(array.values<float>()[index]);
            break;
        case results::ArrayType::Float64:
            values[index] = static_cast<std::int32_t>(array.values<double>()[index]);
            break;
        default:
            values[index] = 0;
            break;
        }
    }
    return values;
}

// The array called `name` of a result; a UsageError that names the result when it has none.
[[nodiscard]] inline const results::ResultArray& needArray(const results::NamedResultData& data, const char* name,
                                                           const std::string& what)
{
    const results::ResultArray* array = data.find(name);
    if (array == nullptr)
        throw base::UsageError("the " + what + " has no array '" + name + "'");
    return *array;
}

[[nodiscard]] inline const results::ResultText& needText(const results::NamedResultData& data, const char* name,
                                                         const std::string& what)
{
    const results::ResultText* text = data.findText(name);
    if (text == nullptr)
        throw base::UsageError("the " + what + " has no text '" + name + "'");
    return *text;
}

} // namespace oor::query
