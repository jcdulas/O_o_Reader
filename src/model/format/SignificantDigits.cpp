#include "oor/model/format/SignificantDigits.hpp"

namespace oor::model
{

std::uint8_t significantDigitsOf(std::string_view text) noexcept
{
    constexpr unsigned MAX_DIGITS = 255;
    unsigned count = 0;
    bool seenNonZero = false;
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        const char character = text[index];
        if (character == 'E' || character == 'e')
            break;
        if (character < '0' || character > '9')
            continue;
        if (character != '0')
            seenNonZero = true;
        if (seenNonZero && count < MAX_DIGITS)
            ++count;
    }
    return static_cast<std::uint8_t>(seenNonZero ? count : 1);
}

} // namespace oor::model
