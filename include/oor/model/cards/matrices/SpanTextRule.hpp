#pragma once

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <optional>
#include <string_view>

namespace oor::model
{

// An optional text value occupies one model-file field regardless of its length.
[[nodiscard]] inline std::optional<CardProblem> spanTextProblem(std::string_view text, const char* field)
{
    return text.empty() ? std::nullopt : checkNameWord(text, field);
}

} // namespace oor::model
