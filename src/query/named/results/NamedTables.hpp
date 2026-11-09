#pragma once

#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/results/NamedArrays.hpp"

#include <cmath>
#include <memory>
#include <optional>
#include <string>

namespace oor::query
{

// What the queries on named results (aeroelastic, thermal and optimisation tables) share.

// A numeric attribute of a named result ("mach", "dynamicPressure"): NaN when it is not there.
[[nodiscard]] inline double attributeOf(const results::NamedResult& result, const char* key)
{
    const std::string* text = result.meta().attribute(key);
    if (text == nullptr)
        return std::nan("");
    try
    {
        return std::stod(*text);
    }
    catch (const std::exception&)
    {
        return std::nan("");
    }
}

// The precision of a named result: single when the file is 32-bit.
[[nodiscard]] inline Precision precisionOfNamed(const results::NamedResultData& data) noexcept
{
    for (const results::ResultArray& array : data.arrays)
        if (array.type == results::ArrayType::Float64)
            return Precision::Double;
    return Precision::Single;
}

// The status of a named result as the text the provenance gives.
[[nodiscard]] inline std::string statusOfNamed(const results::NamedResult& result)
{
    return std::string(results::resultStatusName(result.meta().status));
}

} // namespace oor::query
