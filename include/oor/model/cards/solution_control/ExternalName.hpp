#pragma once

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <optional>
#include <string_view>

namespace oor::model
{

// External service identifiers retain their exact spelling.
[[nodiscard]] inline std::optional<CardProblem> checkExternalName(std::string_view name, const char* field)
{
    return checkNameWord(name, field);
}

} // namespace oor::model
