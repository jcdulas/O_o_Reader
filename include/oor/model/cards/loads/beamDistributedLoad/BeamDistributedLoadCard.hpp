#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>

namespace oor::model
{

// TYPE: the direction of a PLOAD1 load. F is a force and M a moment; X, Y, Z is the axis in the basic coordinate
// system, and an E at the end puts the axis in the element coordinate system.
enum class BarLoadType : std::uint8_t
{
    FX,
    FY,
    FZ,
    FXE,
    FYE,
    FZE,
    MX,
    MY,
    MZ,
    MXE,
    MYE,
    MZE
};

// SCALE: how X1 and X2 are measured along the element axis: LE actual lengths, FR fractions of the length; PR
// (projected) measures the length of a distributed load projected on the axis it acts along.
enum class BarLoadScale : std::uint8_t
{
    LE,
    FR,
    LEPR,
    FRPR
};

// One PLOAD1 card: a concentrated or distributed load on a CBAR, CBEAM or CBEND element.
//
// Card layout: PLOAD1  SID  EID  TYPE  SCALE  X1  P1  X2  P2
// A blank X2 is X1 (blank X2 or X2 = X1 is a concentrated load P1 at X1): X2 is stored resolved and
// always written. A blank P2 stays blank on the card (written back blank): the solver reads it as 0 when X2 differs
// from X1, a load that falls linearly from P1 to 0; effectiveP2() is the value the solver uses.
struct BeamDistributedLoadCard
{
    base::EntityId id = 0;
    base::EntityId elemId = 0;
    BarLoadType kind = BarLoadType::FX;
    BarLoadScale scaleFactor = BarLoadScale::LE;
    double xCoord1 = 0.0;
    double pressure1 = 0.0;
    double xCoord2 = 0.0;
    std::optional<double> pressure2; // none: the field is blank

    [[nodiscard]] bool isConcentrated() const noexcept { return xCoord2 == xCoord1; }
    // The P2 the solver loads with: the value given, else P1 for a concentrated load (where it plays no part) and 0 for
    // a distributed one.
    [[nodiscard]] double effectiveP2() const noexcept
    {
        if (pressure2)
            return *pressure2;
        return isConcentrated() ? pressure1 : 0.0;
    }
    [[nodiscard]] bool isFractional() const noexcept
    {
        return scaleFactor == BarLoadScale::FR || scaleFactor == BarLoadScale::FRPR;
    }
};

} // namespace oor::model
