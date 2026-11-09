#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oor::analysis
{

// What a viewer draws of a model, in the basic system, every primitive with its element and every
// point with the grid point it moves with. The rows of the grid points are those of `nodes.xyz_global()`: the live GRID
// cards by increasing ID (-1 for a point that is no grid point).
struct DisplayMeshOptions
{
    std::optional<std::vector<base::EntityId>> elements; // only these elements (any order); nothing: all
    bool zoffs = false;    // the vertices of the shells on their reference surface (ZOFFS along the element normal)
    bool sections = false; // the outlines of the PBARL and PBEAML sections of the 1D elements drawn
};

struct DisplayMesh
{
    // The triangles of the shells (every 2D element: a quadrilateral split along G1-G3 into (1, 2, 3) and (1, 3, 4),
    // the mid-side points of a CTRIA6 and CQUAD8 covered) and of the free faces of the solids, in the order of the
    // element IDs (the shells, then the faces of the solids).
    std::vector<double> triangleVertices;   // 9 per triangle
    std::vector<std::int32_t> triangleEid;  //
    std::vector<std::int32_t> triangleFace; // -1 for a shell, the face of a solid (from 1, format reference face order)
    std::vector<std::int64_t> vertexGridRow;    // 3 per triangle
    std::vector<std::int32_t> vertexLocalIndex; // 3 per triangle: the place of the grid point on the card, from 1

    // The segments of the 1D elements between their offset ends (CBAR, CBEAM: A' and B'; CROD, CONROD, CTUBE, CBEND:
    // GA and GB; CBEAM3: GA-GC and GC-GB, or GA-GB without GC).
    std::vector<double> segmentEnds;           // 6 per segment
    std::vector<std::int32_t> segmentEid;      //
    std::vector<std::int32_t> segmentPid;      //
    std::vector<std::int64_t> segmentGridRows; // 2 per segment

    // The outlines of the PBARL and PBEAML sections of the segments (`sections`): rings of (y, z) points in the
    // element y-z plane (y, z of the element axes), the outer outline counter-clockwise (z to the right, y up) and the
    // holes clockwise; one set of rings per property and station.
    std::vector<double> outlinePoints;           // 2 per point
    std::vector<std::int64_t> outlineOffsets{0}; // the first point of each ring, and the end
    std::vector<std::int32_t> outlinePid;        // per ring
    std::vector<std::int32_t> outlineStation;    // per ring: the station of the property (0 at end A)
    std::vector<double> outlineXxb;              // per ring
    std::vector<std::uint8_t> outlineHole;       // per ring
    std::vector<std::string> outlineType;        // per ring: the section type ("I", ...)

    // The segments of the rigid elements and connections: a star from the first grid point of the card (GN of an
    // RBE2, REFGRID of an RBE3), the chain of an RSPLINE, the sides of an RTRPLT, GA-GB of the others.
    std::vector<double> connectionEnds;           // 6 per segment
    std::vector<std::int32_t> connectionEid;      //
    std::vector<std::uint8_t> connectionKind;     // an index in connectionKinds()
    std::vector<std::int64_t> connectionGridRows; // 2 per segment

    // The masses: CONM2 at its centre of gravity, CONM1 at its grid point, CMASS1 and CMASS2 between their grid points.
    std::vector<double> massPosition;      // 3 per mass
    std::vector<std::int32_t> massEid;     //
    std::vector<double> mass;              // M of CONM2, M11 of CONM1, M of CMASS2 or of the PMASS of a CMASS1
    std::vector<std::int64_t> massGridRow; //

    // The grid points: their positions in the order of the rows (3 per grid point).
    std::vector<double> gridPositions;
};

// The names of the connection kinds, in the order of their indices: rigid, rigidInterpolation, spring, damper, bush,
// fastener, gap.
[[nodiscard]] const std::vector<std::string>& connectionKinds();

[[nodiscard]] DisplayMesh buildDisplayMesh(const model::Model& model, const DisplayMeshOptions& options,
                                           base::ThreadPool* pool = nullptr);

// The displacement of points attached to grid points, in the basic system: u + theta x (p - x), x the position of the
// grid point and (u, theta) its motion. `motion` is (cases, grid points, 6), the rows of `positions`. A point of row -1
// gets NaN. `out` is (cases, points, 3).
void displacePoints(std::span<const double> points, std::span<const std::int64_t> rows,
                    std::span<const double> positions, std::span<const double> motion, std::size_t cases,
                    std::span<double> out);

} // namespace oor::analysis
