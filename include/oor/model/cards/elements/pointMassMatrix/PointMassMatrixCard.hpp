#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CONM1 card: a concentrated mass at a grid point, given as a full 6 x 6 symmetric mass matrix.
//
// Card layout (the values run on over the continuation lines, eight fields each):
//   CONM1  EID  G    CID  M11  M21  M22  M31  M32
//          M33  M41  M42  M43  M44  M51  M52  M53
//          M54  M55  M61  M62  M63  M64  M65  M66
//
// The matrix is kept the way the card gives it: the 21 values of the lower triangle, row by row (M11, M21, M22, M31,
// M32, M33, ..., M61 to M66). A blank value is 0. CID is the coordinate system of the matrix: 0 (or blank) is the basic
// one.
struct PointMassMatrixCard
{
    static constexpr std::size_t VALUE_COUNT = 21;

    base::EntityId id = 0;
    base::EntityId node = 0;
    base::EntityId frameId = 0;
    std::array<double, VALUE_COUNT> massMatrix = {};

    // The position in `m` of the term of row `row` and column `column` (1 to 6, column <= row): M(row)(column).
    [[nodiscard]] static constexpr std::size_t indexOf(std::size_t row, std::size_t column) noexcept
    {
        return row * (row - 1) / 2 + (column - 1);
    }
};

} // namespace oor::model
