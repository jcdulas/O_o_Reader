#pragma once

#include <cstdint>
#include <string>

namespace oor::model
{

// A typed card slot. Numeric archive input and output use these values without decimal conversion.
struct FieldValue
{
    enum class Kind : std::uint8_t
    {
        Blank,
        Integer,
        Real,
        Text,
        SpanText,
        NamedReal
    };
    Kind kind = Kind::Blank;
    std::int64_t integer = 0;
    double real = 0.0;
    std::string text;
};

} // namespace oor::model
