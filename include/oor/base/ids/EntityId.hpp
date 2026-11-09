#pragma once

#include <cstdint>
#include <limits>
#include <optional>

namespace oor::base
{

// A Nastran identifier (grid, element, property, set, ...). 32-bit signed, in memory and in every exported table. An ID
// above MAX_ENTITY_ID in a file is a read error, never a silent truncation.
using EntityId = std::int32_t;

inline constexpr EntityId MAX_ENTITY_ID = std::numeric_limits<EntityId>::max();

// The ID if it fits in 32 bits.
[[nodiscard]] constexpr std::optional<EntityId> toEntityId(std::int64_t value) noexcept
{
    if (value < std::numeric_limits<EntityId>::min() || value > MAX_ENTITY_ID)
        return std::nullopt;
    return static_cast<EntityId>(value);
}

} // namespace oor::base
