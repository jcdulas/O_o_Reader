#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace oor::model
{

// The temperature of one grid point of a heat shell element, and where in the thickness it acts.
struct HeatShellInitialTemperaturePoint
{
    base::EntityId node = 0;     // Gi: a grid point of a heat shell element
    std::int32_t components = 1; // Ci: the digits 1 (top), 2 (bottom), 3 (middle) in ascending order; 1 when blank
    double temperature = 0.0;    // Ti

    [[nodiscard]] bool operator==(const HeatShellInitialTemperaturePoint&) const noexcept = default;
};

// One TEMPN1 card (SOL 400): the initial temperature of the grid points of heat shell elements (those of a PSHLN1 heat
// transfer property) with a linear or quadratic temperature distribution across the thickness. It complements the TEMP
// card, which only gives the temperature of the top points.
//
// Card layout: TEMPN1  SID  G1  C1  T1  G2  C2  T2        fields 1 to 7
// The format draws two points on the line and no continuation, so a card holds one or two points, written back as it
// was read. Ci is a set of the digits 1, 2 and 3 with no blank (1 is the top, 2 the bottom, 3 the middle), 1 when blank
// (`LoadClosureSupport.hpp`). Gi and Ti are required. SID is a temperature set: the TEMP(INIT) and IC Case Control
// commands select it, like the TEMP and TEMPD cards.
struct HeatShellInitialTemperatureCard
{
    static constexpr std::size_t MAX_POINTS = 2;

    base::EntityId id = 0;
    std::vector<HeatShellInitialTemperaturePoint> points;
};

} // namespace oor::model
