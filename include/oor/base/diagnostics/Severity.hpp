#pragma once

#include <string_view>

namespace oor::base
{

enum class Severity
{
    Info,
    Warning,
    Error
};

[[nodiscard]] std::string_view severityName(Severity severity) noexcept;

} // namespace oor::base
