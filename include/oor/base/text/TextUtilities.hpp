#pragma once

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace oor::base
{

// Small locale-independent text helpers. Everything here is ASCII only: Nastran decks are ASCII, and a locale
// dependent toupper() would make results differ between machines.

[[nodiscard]] constexpr bool isAsciiSpace(char character) noexcept
{
    return character == ' ' || character == '\t' || character == '\r' || character == '\n' || character == '\f' ||
           character == '\v';
}

[[nodiscard]] constexpr char toUpperAscii(char character) noexcept
{
    return (character >= 'a' && character <= 'z') ? static_cast<char>(character - 'a' + 'A') : character;
}

[[nodiscard]] constexpr std::string_view trimAscii(std::string_view text) noexcept
{
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && isAsciiSpace(text[begin]))
        ++begin;
    while (end > begin && isAsciiSpace(text[end - 1]))
        --end;
    return text.substr(begin, end - begin);
}

[[nodiscard]] constexpr bool isBlank(std::string_view text) noexcept
{
    return trimAscii(text).empty();
}

[[nodiscard]] inline std::string toUpperAscii(std::string_view text)
{
    std::string upper(text);
    std::transform(upper.begin(), upper.end(), upper.begin(), [](char character) { return toUpperAscii(character); });
    return upper;
}

[[nodiscard]] constexpr bool equalsIgnoreCase(std::string_view left, std::string_view right) noexcept
{
    if (left.size() != right.size())
        return false;
    for (std::size_t index = 0; index < left.size(); ++index)
    {
        if (toUpperAscii(left[index]) != toUpperAscii(right[index]))
            return false;
    }
    return true;
}

[[nodiscard]] constexpr bool startsWithIgnoreCase(std::string_view text, std::string_view prefix) noexcept
{
    return text.size() >= prefix.size() && equalsIgnoreCase(text.substr(0, prefix.size()), prefix);
}

// Paths are std::filesystem::path inside; text that crosses the boundary (messages, Python) is UTF-8.
[[nodiscard]] inline std::string pathToUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

[[nodiscard]] inline std::filesystem::path pathFromUtf8(std::string_view text)
{
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

} // namespace oor::base
