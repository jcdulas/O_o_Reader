#pragma once

#include "oor/analysis/geometry/ElementGeometry.hpp"

#include <cstddef>
#include <vector>

namespace oor::analysis
{

// clang-format off
// The quality of one element. Every metric that does not apply to the element, or cannot be computed (a degenerate
// element), is NaN, the same in a value and in an array. Angles are in degrees.
//
//   aspectRatio   longest edge over shortest edge (the corner-to-corner distances); 1 for a square, a cube, an
//                 equilateral triangle, a regular tetrahedron
//   skew          shells: 90 degrees less the angle between the lines that join the middles of opposite sides
//                 (quadrilateral) or between a median and the opposite side (triangle); 0 for a rectangle and an
//                 equilateral triangle
//   warp          quadrilateral: the largest distance of a corner from the mean plane over the mean length of the
//                 diagonals (format reference GEOMCHECK); 0 for a planar element
//   taper         quadrilateral: the largest |area of the triangle at a corner over half the area of the element
//                 - 1| (format reference GEOMCHECK); 0 for a rectangle and a parallelogram
//   minAngle      the smallest interior angle: of a shell at its corners, of a solid over the corners of all its
//                 faces
//   maxAngle      the largest; a concave corner of a quadrilateral counts more than 180
//   jacobianRatio the smallest over the largest absolute Jacobian determinant at the corners and the centre of the
//                 element (the four corners of the base for a pyramid), negative where the mapping is inverted at
//                 one of those points; 1 for an element with a uniform mapping
//   jacobianMin   the smallest Jacobian determinant there, in the reference coordinates of the volume rule (so
//                 that it is the area of a straight shell over the area of its reference element, 1/2 or 4, and
//                 the volume of a straight solid over the volume of its reference element); not positive for an
//                 inverted or collapsed element
//   minEdge       the shortest and the longest edge (corner to corner)
//   maxEdge
//   collapse      tetrahedron: the smallest height of a vertex over the face opposite to it, relative to the
//                 height of the regular tetrahedron that has a face of the same area; 1 for a regular
//                 tetrahedron, 0 for a flat one
//   edgePointLengthRatio  elements with mid-side nodes: the largest |2 r - 1|, r the position of the node along
//                 the line between the corners of its edge (0 at the middle, 0.5 at a quarter point; format reference
//                 GEOMCHECK EPLR)
//   edgePointAngle  the smallest angle at a mid-side node between the lines to the corners of its edge (180 on
//                 the straight line; format reference GEOMCHECK EPIA)
//   faceWarp      solids with quadrilateral faces: the smallest cosine between the normals at diagonally opposite
//                 corners of a face (1 for planar faces; format reference GEOMCHECK HEX_WARP)
// clang-format on
struct QualityValues
{
    double aspectRatio = NOT_DEFINED;
    double skew = NOT_DEFINED;
    double warp = NOT_DEFINED;
    double taper = NOT_DEFINED;
    double minAngle = NOT_DEFINED;
    double maxAngle = NOT_DEFINED;
    double jacobianRatio = NOT_DEFINED;
    double jacobianMin = NOT_DEFINED;
    double minEdge = NOT_DEFINED;
    double maxEdge = NOT_DEFINED;
    double collapse = NOT_DEFINED;
    double edgePointLengthRatio = NOT_DEFINED;
    double edgePointAngle = NOT_DEFINED;
    double faceWarp = NOT_DEFINED;
};

// The quality of many elements of one card type: one array per metric, one entry per element in the order of the rows
// asked for.
struct QualityArrays
{
    std::vector<double> aspectRatio;
    std::vector<double> skew;
    std::vector<double> warp;
    std::vector<double> taper;
    std::vector<double> minAngle;
    std::vector<double> maxAngle;
    std::vector<double> jacobianRatio;
    std::vector<double> jacobianMin;
    std::vector<double> minEdge;
    std::vector<double> maxEdge;
    std::vector<double> collapse;
    std::vector<double> edgePointLengthRatio;
    std::vector<double> edgePointAngle;
    std::vector<double> faceWarp;

    // All arrays `count` long, NaN.
    void assign(std::size_t count);
    [[nodiscard]] std::size_t size() const noexcept { return aspectRatio.size(); }
    void store(std::size_t index, const QualityValues& values) noexcept;
    [[nodiscard]] QualityValues at(std::size_t index) const noexcept;
};

} // namespace oor::analysis
