#pragma once

#include "oor/base/text/MessageNumber.hpp"

#include <concepts>
#include <string>
#include <string_view>

namespace oor::testing
{

namespace detail
{

inline std::string recordField(std::string_view text)
{
    return std::string(text);
}

inline std::string recordField(std::integral auto value)
{
    return std::to_string(value);
}

// A real in the shortest text that reads back as the same double, with a decimal point when that text has neither a
// point nor an exponent: in a model file `1` is an integer and `1.` a real.
inline std::string recordField(std::floating_point auto value)
{
    std::string text = base::MessageNumber::shortest(static_cast<double>(value));
    if (text.find_first_of(".eEnN") == std::string::npos)
        text += '.';
    return text;
}

} // namespace detail

// One record of a model file: the fields joined by ';' and a new line at the end. A text is written as it is (""
// is a blank field, "0." a real), an integer in decimal, a real in the shortest text that reads back as the same
// double. modelRecord("Node", 7, "", 0.5, 1.0, "0.") is "Node;7;;0.5;1.;0.\n".
template <typename... Fields>
std::string modelRecord(const Fields&... fields)
{
    std::string line;
    bool first = true;
    ((line += (first ? "" : ";"), line += detail::recordField(fields), first = false), ...);
    line += '\n';
    return line;
}

} // namespace oor::testing
