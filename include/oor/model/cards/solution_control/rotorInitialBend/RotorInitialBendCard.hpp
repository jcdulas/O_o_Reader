#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

// The UNBCORD row: the components x1, x2 and x3 of the vector that, with the first two grid points of the AXIS of the
// ROTOR, fixes the X-Y plane of the rotor system.
struct RotorPlaneVector
{
    std::array<double, 3> components{};

    [[nodiscard]] bool operator==(const RotorPlaneVector&) const noexcept = default;
};

// A KINK row: where along the rotor axis, counted from the first grid point of the ROTORID, a kink starts (the
// station), the kink angle in degrees about the Z axis of the rotor system, and the phase angle in degrees about the
// shaft axis, from the X-Y plane.
struct RotorKink
{
    double station = 0.0;
    double angle = 0.0;
    double phase = 0.0;

    [[nodiscard]] bool operator==(const RotorKink&) const noexcept = default;
};

// An OFFSET row: the station where an offset starts, the offset value on the axis of the rotor and the phase angle in
// degrees (a phase angle of 0 gives an offset in the positive Y axis of the rotor).
struct RotorOffset
{
    double station = 0.0;
    double value = 0.0;
    double phase = 0.0;

    [[nodiscard]] bool operator==(const RotorOffset&) const noexcept = default;
};

// A BRGDPR row: a grid point of the rotor and the grid point of the stator that it is connected to once the rotor is
// pulled onto its bearing.
struct RotorBearingPair
{
    base::EntityId rotorNode = 0;
    base::EntityId statorNode = 0;

    [[nodiscard]] bool operator==(const RotorBearingPair&) const noexcept = default;
};

// One ROTBENT card: the initial deformation of a rotor (SOL 400 only), made of kinks and offsets, which the rotor
// takes on before it is pulled onto its bearing and analysed.
//
// Card layout: ROTBENT  BENTID  ROTORID  LOADID
//                      UNBCORD  x1  x2  x3                    each row is a line of its own: the keyword is the first
//                      KINK  STATION  ANGLE  PHASE            field of the line (field 9, 17, ...), its values the
//                      OFFSET  STATION  VALUE  PHASE          fields that follow, and a line with a blank first field
//                      BRGDPR  ROTORGRID  STATORGRID          is one more row of the keyword above
// Each keyword is given once and its rows follow it (a second kink is on the line below the KINK line, with a
// blank first field); UNBCORD has a single row. KINK, OFFSET and UNBCORD are optional;
// BRGDPR is required, with at least one pair of grid points (the format marks the keyword and its grid points as
// required). The rows are written in the order UNBCORD, KINK, OFFSET, BRGDPR, the keywords in upper case. The ROTORID
// is a rotor defined by a ROTOR card, the LOADID the set of loads that the first step of the analysis applies (the Case
// Control command LOAD selects it) and the grid points are grid points of the model. One ROTBENT is allowed in a model
// (a check between cards that is not made here).
struct RotorInitialBendCard
{
    base::EntityId id = 0;
    base::EntityId rotorId = 0;
    base::EntityId loadCombinationId = 0;
    std::optional<RotorPlaneVector> planeVector;
    std::vector<RotorKink> kinks;
    std::vector<RotorOffset> offsets;
    std::vector<RotorBearingPair> bearings;

    [[nodiscard]] bool operator==(const RotorInitialBendCard&) const noexcept = default;
};

} // namespace oor::model
