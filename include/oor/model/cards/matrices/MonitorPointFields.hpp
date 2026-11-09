#pragma once

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <optional>
#include <string_view>

namespace oor::model
{

// Monitor names are required; the descriptive label is optional.
[[nodiscard]] inline std::optional<CardProblem> monitorNameProblem(std::string_view name)
{
    return checkNameWord(name, "entryName");
}

[[nodiscard]] inline std::optional<CardProblem> monitorLabelProblem(std::string_view label)
{
    return label.empty() ? std::nullopt : checkNameWord(label, "label");
}

} // namespace oor::model
