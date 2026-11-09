#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

enum class LineInterfaceType : std::uint8_t
{
    GridList
};

inline constexpr std::array<std::string_view, 1> LINE_INTERFACE_TYPE_WORDS{"NodeList"};

// Boundary interpolation names are exact values of the expression grammar.
enum class LineInterfaceInterpolation : std::uint8_t
{
    Linear,
    Quadratic
};

inline constexpr std::array<std::string_view, 2> LINE_INTERFACE_INTERPOLATION_WORDS{"Linear", "Quadratic"};

// The sign of a boundary curve identifier identifies the independent boundary.
struct LineInterfaceBoundary
{
    base::EntityId boundaryId = 0;
    LineInterfaceInterpolation interpolation = LineInterfaceInterpolation::Linear;

    [[nodiscard]] bool operator==(const LineInterfaceBoundary&) const noexcept = default;
};

// An interface ties at least two geometric boundary curves together. Its boundaryList is one text value:
// boundaries=(1,2(Quadratic),-3(Linear)). A bare identifier selects Linear interpolation.
struct LineInterfaceCard
{
    base::EntityId id = 0;
    LineInterfaceType type = LineInterfaceType::GridList;
    std::vector<LineInterfaceBoundary> boundaries;
};

} // namespace oor::model
