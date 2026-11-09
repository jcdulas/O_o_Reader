#include "oor/query/named/common/Topology.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace oor::query
{

std::optional<base::Vec3> warpedMeanPlane(const std::array<base::Vec3, 4>& corners)
{
    // the corners of a bilinear face lie at +-h from its mean plane, h = |(c1 - c2 + c3 - c4) . n| / 4
    constexpr double FLAT = 1.0e-10;
    const base::Vec3 diagonals = base::cross(corners[2] - corners[0], corners[3] - corners[1]);
    const double length = base::norm(diagonals); // twice the area of the projection
    if (!(length > 0.0))
        return std::nullopt;
    const base::Vec3 normal = diagonals / length;
    const double height = std::fabs(base::dot(corners[0] - corners[1] + corners[2] - corners[3], normal)) / 4.0;
    if (!(height > FLAT * std::sqrt(0.5 * length)))
        return std::nullopt;
    return normal;
}

std::vector<std::uint16_t> topologicalTypes()
{
    std::vector<std::uint16_t> types;
    for (const std::uint16_t type : typesOfCategory(model::CardCategory::Element))
    {
        if (analysis::ElementTopology::supports(type))
            types.push_back(type);
    }
    return types;
}

bool isCornerPosition(std::uint16_t type, std::size_t position) noexcept
{
    using analysis::ElementShape;
    const ElementShape shape = analysis::ElementTopology::shapeOf(type);
    if (analysis::ElementTopology::alternatingCorners(type))
        return position % 2 == 0;
    switch (shape)
    {
    case ElementShape::Line:
        return position < 2;
    case ElementShape::Triangle:
        return position < 3;
    case ElementShape::Quadrilateral:
        return position < 4;
    case ElementShape::Tetrahedron:
        return position < 4;
    case ElementShape::Pentahedron:
        return position < 6;
    case ElementShape::Hexahedron:
        return position < 8;
    case ElementShape::Pyramid:
        return position < 5;
    default:
        return true;
    }
}

namespace
{

// A mid-side node: the corners it is between (positions from 0) and its position in the node list.
struct MidSide
{
    std::uint8_t first;
    std::uint8_t second;
    std::uint8_t position;
};

constexpr std::array<MidSide, 3> TRIANGLE6 = {{{0, 1, 3}, {1, 2, 4}, {2, 0, 5}}};
// CTRIAX6: the corners are the nodes 0, 2 and 4, so the mid-side nodes are between them (positions in the node list).
constexpr std::array<MidSide, 3> TRIANGLE_ALTERNATING = {{{0, 2, 1}, {2, 4, 3}, {4, 0, 5}}};
constexpr std::array<MidSide, 4> QUAD8 = {{{0, 1, 4}, {1, 2, 5}, {2, 3, 6}, {3, 0, 7}}};
constexpr std::array<MidSide, 6> TETRA10 = {{{0, 1, 4}, {1, 2, 5}, {2, 0, 6}, {0, 3, 7}, {1, 3, 8}, {2, 3, 9}}};
constexpr std::array<MidSide, 9> PENTA15 = {
    {{0, 1, 6}, {1, 2, 7}, {2, 0, 8}, {0, 3, 9}, {1, 4, 10}, {2, 5, 11}, {3, 4, 12}, {4, 5, 13}, {5, 3, 14}}};
constexpr std::array<MidSide, 12> HEXA20 = {{{0, 1, 8},
                                             {1, 2, 9},
                                             {2, 3, 10},
                                             {3, 0, 11},
                                             {0, 4, 12},
                                             {1, 5, 13},
                                             {2, 6, 14},
                                             {3, 7, 15},
                                             {4, 5, 16},
                                             {5, 6, 17},
                                             {6, 7, 18},
                                             {7, 4, 19}}};
constexpr std::array<MidSide, 8> PYRAMID13 = {
    {{0, 1, 5}, {1, 2, 6}, {2, 3, 7}, {3, 0, 8}, {0, 4, 9}, {1, 4, 10}, {2, 4, 11}, {3, 4, 12}}};

template <std::size_t N>
std::int32_t lookup(const std::array<MidSide, N>& table, std::span<const std::int32_t> nodes, std::size_t first,
                    std::size_t second) noexcept
{
    for (const MidSide& entry : table)
    {
        if ((entry.first == first && entry.second == second) || (entry.first == second && entry.second == first))
            return entry.position < nodes.size() ? nodes[entry.position] : 0;
    }
    return 0;
}

} // namespace

std::int32_t midSideNodeOf(std::uint16_t type, std::span<const std::int32_t> nodes, std::size_t first,
                           std::size_t second) noexcept
{
    using analysis::ElementShape;
    const ElementShape shape = analysis::ElementTopology::shapeOf(type);
    const std::size_t count = nodes.size();
    if (analysis::ElementTopology::alternatingCorners(type))
        return count == 6 ? lookup(TRIANGLE_ALTERNATING, nodes, first, second) : 0;
    switch (shape)
    {
    case ElementShape::Triangle:
        return count == 6 ? lookup(TRIANGLE6, nodes, first, second) : 0;
    case ElementShape::Quadrilateral:
        return count >= 8 ? lookup(QUAD8, nodes, first, second) : 0;
    case ElementShape::Tetrahedron:
        return count == 10 ? lookup(TETRA10, nodes, first, second) : 0;
    case ElementShape::Pentahedron:
        return count == 15 ? lookup(PENTA15, nodes, first, second) : 0;
    case ElementShape::Hexahedron:
        return count == 20 ? lookup(HEXA20, nodes, first, second) : 0;
    case ElementShape::Pyramid:
        return count == 13 ? lookup(PYRAMID13, nodes, first, second) : 0;
    default:
        return 0;
    }
}

// The face of a solid element in the order that gives the outward normal, found by its sorted corner nodes.
std::int32_t orientedFaceIndex(const model::Model& model, const analysis::ElementRef& owner,
                               const analysis::FaceKey& key, std::array<base::EntityId, 4>& corners)
{
    std::vector<std::array<base::EntityId, 4>> faces;
    analysis::ElementTopology::orientedFacesOf(*model.findTable(owner.type), owner.type, owner.row, faces);
    for (std::size_t position = 0; position < faces.size(); ++position)
    {
        const auto& face = faces[position];
        std::array<base::EntityId, 4> sorted = face;
        const std::size_t count = face[3] == 0 ? 3 : 4;
        for (std::size_t item = 1; item < count; ++item)
        {
            for (std::size_t place = item; place > 0 && sorted[place - 1] > sorted[place]; --place)
                std::swap(sorted[place - 1], sorted[place]);
        }
        if (count == 3)
            sorted[3] = 0; // FaceKey: the corners in increasing order, a triangle with 0 last
        if (sorted == key.nodes)
        {
            corners = face;
            return static_cast<std::int32_t>(position + 1);
        }
    }
    return 0;
}

bool orientedFaceOf(const model::Model& model, const analysis::ElementRef& owner, const analysis::FaceKey& key,
                    std::array<base::EntityId, 4>& corners)
{
    return orientedFaceIndex(model, owner, key, corners) != 0;
}

std::optional<std::array<base::EntityId, 4>> pressureFace(const model::Model& model, model::CardLocation element,
                                                          base::EntityId node1, base::EntityId node3Or4)
{
    const model::CardTable* table = model.findTable(element.table);
    if (table == nullptr || node1 == 0 || analysis::dimensionOf(analysis::ElementTopology::shapeOf(element.table)) != 3)
        return std::nullopt;
    std::vector<std::array<base::EntityId, 4>> faces;
    analysis::ElementTopology::orientedFacesOf(*table, element.table, element.row, faces);
    const bool tetrahedron = analysis::ElementTopology::shapeOf(element.table) == analysis::ElementShape::Tetrahedron;
    const auto has = [](const std::array<base::EntityId, 4>& face, base::EntityId node)
    { return std::find(face.begin(), face.end(), node) != face.end() && node != 0; };
    for (const auto& face : faces)
    {
        if (!has(face, node1))
            continue;
        if (node3Or4 == 0)
        {
            // CTETRA: G1 is on the face and G4 is not given: any face with G1 would do, the format reference gives
            // both. A CPENTA triangle is found by G1 alone.
            return face;
        }
        const bool containsNode3Or4 = has(face, node3Or4);
        if (tetrahedron ? !containsNode3Or4 : containsNode3Or4)
            return face;
    }
    return std::nullopt;
}

std::int64_t TopologyList::indexOf(std::int32_t id) const noexcept
{
    const auto found = std::lower_bound(list.ids.begin(), list.ids.end(), id);
    if (found == list.ids.end() || *found != id)
        return -1;
    return found - list.ids.begin();
}

TopologyList TopologyList::build(const model::Model& model, ExecutionContext& context,
                                 const std::optional<std::vector<std::int64_t>>& only)
{
    TopologyList result;
    result.list = listCards(model, topologicalTypes(), only);
    result.nodes = nodesOfCards(model, result.list, context);
    return result;
}

} // namespace oor::query
