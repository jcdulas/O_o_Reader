#pragma once

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/query/named/common/CardList.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace oor::query
{

// The card types that have nodes the connectivity code understands (shells, solids, lines, points), in registry order.
[[nodiscard]] std::vector<std::uint16_t> topologicalTypes();

// True when the position (from 0) in the node list of an element of this card type is a corner node (the others are
// mid-side nodes and the like): the first nodes of a shell or solid up to its number of corners, the two ends of a
// line, every other node of a CTRIAX6, every node of a point element.
[[nodiscard]] bool isCornerPosition(std::uint16_t type, std::size_t position) noexcept;

// The mid-side node of the edge between the corners `first` and `second` (positions from 0 in the node list of the
// card) of an element whose nodes are `nodes`, for the cards that number their mid-side nodes the standard way (CQUAD8,
// CTRIA6, CQUAD, CTETRA with ten nodes, CPENTA with fifteen, CHEXA with twenty, CPYRAM with thirteen); 0 when the
// element has no mid-side node there.
[[nodiscard]] std::int32_t midSideNodeOf(std::uint16_t type, std::span<const std::int32_t> nodes, std::size_t first,
                                         std::size_t second) noexcept;

// The face of a solid element, with its corners in the order that gives the outward normal (as the element lists them),
// found by the sorted corner nodes of the face (a triangle has 0 last). False when the element has no such face.
[[nodiscard]] bool orientedFaceOf(const model::Model& model, const analysis::ElementRef& owner,
                                  const analysis::FaceKey& key, std::array<base::EntityId, 4>& corners);

// The same, and the position (from 1) of the face in the face list of the element type (the template of
// ElementTopology::orientedFacesOf); 0 when the element has no such face.
[[nodiscard]] std::int32_t orientedFaceIndex(const model::Model& model, const analysis::ElementRef& owner,
                                             const analysis::FaceKey& key, std::array<base::EntityId, 4>& corners);

// The face a PLOAD4 loads on a solid element, with its corners in the order of the outward normal: the face of the
// element that has the corner G1 and, when given, G3 or G4 (format reference: the corner diagonally opposite to G1 on a
// rectangular face, an adjacent one on a CPYRAM for a triangular face; for a CTETRA the vertex that is not on the
// face). Nothing when no face matches. `count` is 3 or 4.
[[nodiscard]] std::optional<std::array<base::EntityId, 4>> pressureFace(const model::Model& model,
                                                                        model::CardLocation element,
                                                                        base::EntityId node1, base::EntityId node3Or4);

// The unit normal of the mean plane of a warped quadrilateral face (the cross product of its diagonals, in the sense of
// the order of its corners): a PLOAD4 on a four-noded quadrilateral shell (CQUAD4, CQUADR) acts on that plane, its
// pressure integrated over the projection of the face on it and along its normal, this library's rule, the solver's
// unverified (a companion package's D-039 alike: the plane a CQUAD4 is formulated on), the loads at the grid points p n
// int N_i (xXi x xEta) . n. Nothing for a face whose corners are within 1e-10 of its size of a plane (the rounding of
// the corners of a flat face), whose surface is that plane: its loads are those of the surface, to the bit.
[[nodiscard]] std::optional<base::Vec3> warpedMeanPlane(const std::array<base::Vec3, 4>& corners);

// The elements with nodes by ID, for the queries that look an element up by its ID: the list, the nodes of each and the
// shape.
struct TopologyList
{
    CardList list;
    NodeLists nodes;

    // The position in the list of the element with this ID, -1 when there is none.
    [[nodiscard]] std::int64_t indexOf(std::int32_t id) const noexcept;
    [[nodiscard]] static TopologyList build(const model::Model& model, ExecutionContext& context,
                                            const std::optional<std::vector<std::int64_t>>& only = std::nullopt);
};

} // namespace oor::query
