#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <limits>

namespace oor::base
{

// Internal identity of a card, given when it is created, unique in a model, never reused and never changed, whatever
// happens to its Nastran ID (renumbering, ID reuse after deletion). Handles hold a CardKey.
struct CardKey
{
    static constexpr std::uint32_t INVALID_VALUE = std::numeric_limits<std::uint32_t>::max();

    std::uint32_t value = INVALID_VALUE;

    [[nodiscard]] constexpr bool isValid() const noexcept { return value != INVALID_VALUE; }
    constexpr auto operator<=>(const CardKey&) const noexcept = default;
};

} // namespace oor::base

template <>
struct std::hash<oor::base::CardKey>
{
    std::size_t operator()(oor::base::CardKey key) const noexcept { return std::hash<std::uint32_t>{}(key.value); }
};
