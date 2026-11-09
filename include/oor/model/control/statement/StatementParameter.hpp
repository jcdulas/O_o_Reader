#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// A neutral named parameter of an executive record. Names and values preserve exact case.
struct StatementParameter
{
    std::string name;
    std::string options;
    bool hasOptions = false;
    std::string value;
    bool hasValue = false;
    char quote = '\0';
    [[nodiscard]] bool operator==(const StatementParameter&) const = default;
    [[nodiscard]] std::string format() const;
    [[nodiscard]] bool isNamed(std::string_view word) const noexcept;
    [[nodiscard]] static StatementParameter flag(std::string name);
    [[nodiscard]] static StatementParameter withValue(std::string name, std::string value);
    [[nodiscard]] static StatementParameter withQuotedValue(std::string name, std::string value);
};
[[nodiscard]] const StatementParameter* findParameter(const std::vector<StatementParameter>& parameters,
                                                      std::string_view word) noexcept;
[[nodiscard]] StatementParameter* findParameter(std::vector<StatementParameter>& parameters,
                                                std::string_view word) noexcept;
} // namespace oor::model
