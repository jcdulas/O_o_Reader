#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct SolutionSequence
{
    std::int64_t number;
    std::string_view name;       // "LinearStatics"
    std::string_view numberName; // "LinearStaticsNumber"
};

// The exact neutral names and numeric identifiers of the supported solution sequences.
struct SolutionSequences
{
    [[nodiscard]] static const std::vector<SolutionSequence>& all() noexcept;
    // The number of a neutral name (exact), or nullopt.
    [[nodiscard]] static std::optional<std::int64_t> numberOf(std::string_view name) noexcept;
    // The neutral name of a number, or an empty text.
    [[nodiscard]] static std::string_view nameOf(std::int64_t number) noexcept;
};

} // namespace oor::model
