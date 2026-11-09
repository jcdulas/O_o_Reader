#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace oor::model
{

// The power at one grid point of a heat shell element, and where in the thickness it acts.
struct HeatShellNodalLoadLoad
{
    base::EntityId node = 0;     // Gi: a grid point of a heat shell element
    std::int32_t components = 1; // Ci: the digits 1 (top), 2 (bottom), 3 (middle) in ascending order; 1 when blank
    double power = 0.0;          // Qi

    [[nodiscard]] bool operator==(const HeatShellNodalLoadLoad&) const noexcept = default;
};

// One SLOADN1 card (SOL 400): concentrated static loads, a power input, on the grid points of heat shell elements
// (those of a PSHLN1 heat transfer property) with a linear or quadratic temperature distribution through the thickness.
//
// Card layout: SLOADN1  SID  G1  C1  Q1  G2  C2  Q2        fields 1 to 7
// The format draws two loads on the line and no continuation, so a card holds one or two loads, written back as it was
// read (the SLOAD of structural analysis has up to three on its line). Ci is a set of the digits 1, 2 and 3 with no
// blank (1 is the top, 2 the bottom, 3 the middle), 1 when blank (`LoadClosureSupport.hpp`). Gi and Qi are required.
// SID is a load set of a heat transfer analysis: the LOAD Case Control command, an LSEQ or the EXCITEID of a dynamic
// load selects it, as it does an SLOAD.
struct HeatShellNodalLoadCard
{
    static constexpr std::size_t MAX_LOADS = 2;

    base::EntityId id = 0;
    std::vector<HeatShellNodalLoadLoad> loads;
};

} // namespace oor::model
