#pragma once

#include <string_view>

namespace oor::io
{

// Name of this module, used by the build checks.
[[nodiscard]] std::string_view moduleName() noexcept;

} // namespace oor::io
