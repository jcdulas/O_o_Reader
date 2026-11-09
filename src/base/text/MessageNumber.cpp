#include "oor/base/text/MessageNumber.hpp"

#include <array>
#include <charconv>

namespace oor::base
{

namespace
{

// Enough for any double in either form: a sign, 17 digits, a point and an exponent of five characters.
constexpr std::size_t TEXT_CAPACITY = 32;
constexpr int GENERAL_DIGITS = 6;

} // namespace

std::string MessageNumber::shortest(double value)
{
    std::array<char, TEXT_CAPACITY> text{};
    const std::to_chars_result result = std::to_chars(text.data(), text.data() + text.size(), value);
    return std::string(text.data(), result.ptr);
}

std::string MessageNumber::general(double value)
{
    std::array<char, TEXT_CAPACITY> text{};
    const std::to_chars_result result =
        std::to_chars(text.data(), text.data() + text.size(), value, std::chars_format::general, GENERAL_DIGITS);
    return std::string(text.data(), result.ptr);
}

} // namespace oor::base
