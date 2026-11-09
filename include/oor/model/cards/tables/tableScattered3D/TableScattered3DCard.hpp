#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One point of a TABLE3D: three coordinates and the value of the function there.
struct TableScattered3DPoint
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double value = 0.0; // the value of the function at the point

    [[nodiscard]] bool operator==(const TableScattered3DPoint&) const noexcept = default;
};

// One TABLE3D card: a function of three variables given by scattered points, which only the CORD3G (a general
// coordinate system whose angles are tables, FORM = TABLE) uses. It is not one of the TABL3Di forms of SOL 400, but the
// SOL 400 tables and this card share the category MultiDimensionalTable, because a CORD3G names the one and the
// others name the rest by the same kind of number.
//
// Card layout:
//   TABLE3D  TID  X0  Y0  Z0  F0                                   fields 1 to 5 (6 to 8 are not used)
//            X1   Y1  Z1  F1  X2  Y2  Z2  F2                       from field 9, two points to a line
//            X3   Y3  Z3  F3  X4  Y4  Z4  F4
//            -etc.-  ENDT
// A point takes four fields, so the points follow each other without a gap and ENDT is the field after the last F (the
// first field of a line when the last point ends a line). X0, Y0, Z0 and F0 are an offset of the three variables and of
// the function (0.0 when blank), and so is every coordinate and value of a point, so that a blank field inside a point
// is 0.0 too and a point of blank fields is a point at the origin. The format asks for two continuation entries at
// least; as the points are packed (two to a line, ENDT right after the last), that means at least two points.
//
// The value of the function at a point of space is a mean of the values of the four points nearest to it, weighted by
// the inverse of their distance, with the offsets applied. The card keeps the points and evaluates
// nothing. The TID is unique among the cards of the four multi-dimensional tables (IdSpace::PerCardType).
struct TableScattered3DCard
{
    base::EntityId id = 0;
    double xOffset = 0.0;
    double yOffset = 0.0;
    double zOffset = 0.0;
    double valueOffset = 0.0;
    std::vector<TableScattered3DPoint> points;
};

} // namespace oor::model
