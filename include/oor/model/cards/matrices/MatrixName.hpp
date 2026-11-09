#pragma once

#include "oor/base/text/TextUtilities.hpp"

#include <array>
#include <string>
#include <string_view>

namespace oor::model
{

// The fixed matrix names have neutral names, like other words of the model format. User matrix names keep the
// length and character rules of identifiers.
inline constexpr std::array<std::string_view, 4> FIXED_MATRIX_NAMES{"DownwashMatrix", "AeroPressureMatrix",
                                                                    "AeroWeightingMatrix", "AeroWeightingFactor"};

inline bool isFixedMatrixName(std::string_view text) noexcept
{
    for (const std::string_view name : FIXED_MATRIX_NAMES)
        if (text == name)
            return true;
    return false;
}

inline std::string matrixNameText(std::string_view text)
{
    return std::string(text);
}

// A required literal identifier; spelling and case are preserved.
inline bool isMatrixName(std::string_view text) noexcept
{
    if (text.empty())
        return false;
    for (const unsigned char character : text)
        if (character < 0x20 || character == 0x7F)
            return false;
    return true;
}

} // namespace oor::model
