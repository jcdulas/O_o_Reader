#pragma once

#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <optional>
#include <string>

namespace oor::model
{

// A neutral field descriptor, optionally followed by one-based item or station selectors.
struct DesignField
{
    std::string name;
    [[nodiscard]] bool operator==(const DesignField&) const noexcept = default;
};

[[nodiscard]] DesignField readDesignField(FieldReader& reader, int field, const char* fieldName);
void writeDesignField(FieldWriter& writer, int field, const DesignField& value);
[[nodiscard]] std::optional<CardProblem> checkDesignField(const DesignField& value, const char* fieldName);
[[nodiscard]] std::optional<CardProblem> checkDesignFieldName(const std::string& name, const char* fieldName);
[[nodiscard]] std::string describeDesignField(const DesignField& value);

} // namespace oor::model
