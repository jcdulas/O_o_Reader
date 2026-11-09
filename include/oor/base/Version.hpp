#pragma once

#include <string_view>

namespace oor::base
{

// The version of the library, written in the files it makes ("producer") and given to the JavaScript side.
inline constexpr std::string_view LIBRARY_NAME = "oor_reader";
inline constexpr std::string_view LIBRARY_VERSION = "0.2.0";

} // namespace oor::base
