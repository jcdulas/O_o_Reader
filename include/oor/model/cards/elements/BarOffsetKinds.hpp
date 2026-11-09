#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace oor::model
{

// The words of the offset kind of a bar or a beam (Bar2, Beam2 and their defaults Bar2Defaults,
// Beam2Defaults): where its orientation vector and its two offset vectors are given. Each word names three frames:
// the orientation vector in the global (displacement) frame of end A or in the basic frame, then the offset of end A
// and the offset of end B, each in the global (displacement) frame of its node or in the element's own (orient) frame.
inline constexpr std::array<std::string_view, 8> BAR_OFFSET_KIND_WORDS{
    "OffsetsGlobalGlobalGlobal", "OffsetsGlobalGlobalOrient", "OffsetsGlobalOrientGlobal", "OffsetsGlobalOrientOrient",
    "OffsetsBasicGlobalGlobal",  "OffsetsBasicGlobalOrient",  "OffsetsBasicOrientGlobal",  "OffsetsBasicOrientOrient"};

// The same kinds as three letters, in the order of BAR_OFFSET_KIND_WORDS: G (global) or B (basic) for the orientation
// vector, then G (global) or O (orient) for the offsets of ends A and B.
inline constexpr std::array<std::string_view, 8> BAR_OFFSET_KIND_LETTERS{"GGG", "GGO", "GOG", "GOO",
                                                                         "BGG", "BGO", "BOG", "BOO"};

// The old integer codes some models still give (1, 2, 21, 22, 41, 42): no longer defined, kept as written.
inline constexpr std::array<std::string_view, 6> BAR_OFFSET_OLD_CODES{"1", "2", "21", "22", "41", "42"};

// The three letters of an offset kind ("BGO" for OffsetsBasicGlobalOrient); empty for an old integer code or a word
// that is no offset kind.
[[nodiscard]] constexpr std::string_view barOffsetLetters(std::string_view word) noexcept
{
    for (std::size_t index = 0; index < BAR_OFFSET_KIND_WORDS.size(); ++index)
    {
        if (BAR_OFFSET_KIND_WORDS[index] == word)
            return BAR_OFFSET_KIND_LETTERS[index];
    }
    return {};
}

// True for an offset kind word or an old integer code.
[[nodiscard]] constexpr bool isBarOffsetKind(std::string_view word) noexcept
{
    if (!barOffsetLetters(word).empty())
        return true;
    for (const std::string_view code : BAR_OFFSET_OLD_CODES)
    {
        if (code == word)
            return true;
    }
    return false;
}

} // namespace oor::model
