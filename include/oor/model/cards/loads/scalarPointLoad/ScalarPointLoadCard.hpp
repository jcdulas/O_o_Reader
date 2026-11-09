#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// The load at one scalar or grid point of an SLOAD card.
struct PointLoad
{
    base::EntityId node = 0; // Si: a scalar point, or a grid point (the load acts on its component T1)
    double magnitude = 0.0;  // Fi

    [[nodiscard]] bool operator==(const PointLoad&) const noexcept = default;
};

// One SLOAD card: concentrated static loads on scalar or grid points.
//
// Card layout: SLOAD  SID  S1  F1  S2  F2  S3  F3
// One to three loads per card; a card with two or three of them is still one card here, written back as
// it was read.
struct ScalarPointLoadCard
{
    static constexpr std::size_t MAX_LOADS = 3;

    base::EntityId id = 0;
    std::vector<PointLoad> loads;
};

} // namespace oor::model
