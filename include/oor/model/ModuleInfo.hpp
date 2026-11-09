#pragma once

#include <string_view>

namespace oor::model
{

// Name of this module, used by the build checks.
[[nodiscard]] std::string_view moduleName() noexcept;

} // namespace oor::model
