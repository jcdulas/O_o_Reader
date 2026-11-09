#include "oor/analysis/adjacency/ElementTopology.hpp"

#include "ElementProvider.hpp"

#include <algorithm>
#include <string_view>

// Which card types are lines, shells and solids, and what their edges and faces are. The nodes of an element come from
// the same readers as its geometry (ElementProvider::nodes); this file only says which of those nodes count and how
// they join.

namespace oor::analysis
{

namespace
{

struct TypeShape
{
    ElementShape shape = ElementShape::None;
    bool alternating = false; // AxisymTria6: the corners are G1, G3, G5
    bool threeNode = false;   // Beam3: the nodes are GA, GB and GC (GO is the orientation node)
};

bool isAny(std::string_view name, std::initializer_list<std::string_view> names)
{
    return std::find(names.begin(), names.end(), name) != names.end();
}

TypeShape shapeOfCard(std::string_view name)
{
    if (isAny(name, {"ShellTria3", "ShellTria3Drilling", "ShellTria6", "PlaneStrainTria3", "PlaneStrainTria6",
                     "PlaneStressTria3", "PlaneStressTria6", "AxisymTria"}))
        return {ElementShape::Triangle, false, false};
    if (name == "AxisymTria6")
        return {ElementShape::Triangle, true, false};
    if (isAny(name, {"ShellQuad4", "ShellQuad4Drilling", "ShellQuad8", "PlaneStrainQuad", "ShearPanel4", "AxisymQuad",
                     "AxisymQuad4", "AxisymQuad8", "PlaneStrainQuad4", "PlaneStrainQuad8", "PlaneStressQuad4",
                     "PlaneStressQuad8"}))
        return {ElementShape::Quadrilateral, false, false};
    if (name == "SolidTetra")
        return {ElementShape::Tetrahedron, false, false};
    if (name == "SolidPenta")
        return {ElementShape::Pentahedron, false, false};
    if (name == "SolidHexa")
        return {ElementShape::Hexahedron, false, false};
    if (name == "SolidPyramid")
        return {ElementShape::Pyramid, false, false};
    if (name == "Beam3")
        return {ElementShape::Line, false, true};
    if (isAny(name, {"Rod2",
                     "Rod2InlineSection",
                     "Tube2",
                     "Bar2",
                     "Beam2",
                     "Bushing",
                     "Gap2",
                     "Bushing1D",
                     "SpringDofProperty",
                     "SpringDofInline",
                     "DamperDofProperty",
                     "DamperDofInline",
                     "ViscousDamper2",
                     "MassDofProperty",
                     "MassDofInline",
                     "CurvedBeam2",
                     "MarkerBody",
                     "ShellAxisym",
                     "Bushing2D",
                     "SpringDofPropertyExplicit",
                     "SpringDofInlineExplicit",
                     "DamperDofPropertyExplicit",
                     "DamperDofInlineExplicit",
                     "SpringOffsetExplicit"}))
        return {ElementShape::Line, false, false};
    return {ElementShape::Point, false, false};
}

// One entry per card type; a card type with no element reader (no geometry provider) has none.
const std::vector<TypeShape>& shapes()
{
    static const std::vector<TypeShape> TABLE = []
    {
        const model::CardRegistry& registry = model::CardRegistry::instance();
        std::vector<TypeShape> table(registry.size());
        for (std::size_t type = 1; type < registry.size(); ++type)
        {
            const model::CardTypeInfo& info = registry.entry(static_cast<std::uint16_t>(type)).info;
            // The surface elements of the heat transfer cards (CHBDYE, CHBDYG, CHBDYP) are elements of an ID space of
            // their own; they are attached at their nodes like a point element.
            if ((info.idSpace != base::IdSpace::Element && info.idSpace != base::IdSpace::SurfaceElement) ||
                detail::ProviderRegistry::instance().find(static_cast<std::uint16_t>(type)) == nullptr)
                continue;
            table[type] = shapeOfCard(info.name);
        }
        return table;
    }();
    return TABLE;
}

constexpr std::array<std::array<std::uint8_t, 2>, 1> LINE_EDGES = {{{0, 1}}};
constexpr std::array<std::array<std::uint8_t, 2>, 3> TRIANGLE_EDGES = {{{0, 1}, {1, 2}, {2, 0}}};
constexpr std::array<std::array<std::uint8_t, 2>, 4> QUAD_EDGES = {{{0, 1}, {1, 2}, {2, 3}, {3, 0}}};
constexpr std::array<std::array<std::uint8_t, 2>, 6> TETRA_EDGES = {{{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}}};
constexpr std::array<std::array<std::uint8_t, 2>, 9> PENTA_EDGES = {
    {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5}, {5, 3}, {0, 3}, {1, 4}, {2, 5}}};
constexpr std::array<std::array<std::uint8_t, 2>, 12> HEXA_EDGES = {
    {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}}};
constexpr std::array<std::array<std::uint8_t, 2>, 8> PYRAMID_EDGES = {
    {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 4}, {2, 4}, {3, 4}}};

constexpr std::array<FaceCorners, 1> TRIANGLE_FACES = {{{3, {0, 1, 2, 0}}}};
constexpr std::array<FaceCorners, 1> QUAD_FACES = {{{4, {0, 1, 2, 3}}}};
// The faces of the solids with the corners ordered for the outward normal of an element with a positive Jacobian.
constexpr std::array<FaceCorners, 4> TETRA_FACES = {
    {{3, {1, 2, 3, 0}}, {3, {0, 3, 2, 0}}, {3, {0, 1, 3, 0}}, {3, {0, 2, 1, 0}}}};
constexpr std::array<FaceCorners, 5> PENTA_FACES = {
    {{3, {0, 2, 1, 0}}, {3, {3, 4, 5, 0}}, {4, {0, 1, 4, 3}}, {4, {1, 2, 5, 4}}, {4, {2, 0, 3, 5}}}};
constexpr std::array<FaceCorners, 6> HEXA_FACES = {
    {{4, {0, 3, 2, 1}}, {4, {4, 5, 6, 7}}, {4, {0, 1, 5, 4}}, {4, {1, 2, 6, 5}}, {4, {2, 3, 7, 6}}, {4, {3, 0, 4, 7}}}};
constexpr std::array<FaceCorners, 5> PYRAMID_FACES = {
    {{4, {0, 3, 2, 1}}, {3, {0, 1, 4, 0}}, {3, {1, 2, 4, 0}}, {3, {2, 3, 4, 0}}, {3, {3, 0, 4, 0}}}};

// The edges that hold the mid-side nodes of a card, in the order of its mid-side slots (format reference: the slots
// after the corners). They differ from the edge templates above where the card numbers its edges in another order.
constexpr std::array<std::array<std::uint8_t, 2>, 3> TRIANGLE_MID_SIDES = TRIANGLE_EDGES;
constexpr std::array<std::array<std::uint8_t, 2>, 4> QUAD_MID_SIDES = QUAD_EDGES;
constexpr std::array<std::array<std::uint8_t, 2>, 6> TETRA_MID_SIDES = TETRA_EDGES;
constexpr std::array<std::array<std::uint8_t, 2>, 9> PENTA_MID_SIDES = {
    {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 4}, {2, 5}, {3, 4}, {4, 5}, {5, 3}}};
constexpr std::array<std::array<std::uint8_t, 2>, 12> HEXA_MID_SIDES = {
    {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 5}, {2, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}, {7, 4}}};
constexpr std::array<std::array<std::uint8_t, 2>, 8> PYRAMID_MID_SIDES = PYRAMID_EDGES;

std::span<const std::array<std::uint8_t, 2>> midSideTemplate(ElementShape shape) noexcept
{
    switch (shape)
    {
    case ElementShape::Triangle:
        return TRIANGLE_MID_SIDES;
    case ElementShape::Quadrilateral:
        return QUAD_MID_SIDES;
    case ElementShape::Tetrahedron:
        return TETRA_MID_SIDES;
    case ElementShape::Pentahedron:
        return PENTA_MID_SIDES;
    case ElementShape::Hexahedron:
        return HEXA_MID_SIDES;
    case ElementShape::Pyramid:
        return PYRAMID_MID_SIDES;
    default:
        return {};
    }
}

// The node IDs of an element in the order its reader gives them, kept between calls of a thread.
std::vector<base::EntityId>& scratchIds()
{
    thread_local std::vector<base::EntityId> ids;
    return ids;
}

// The IDs of the corners of an element from the list its reader gave: the first slots, or every other one for CTRIAX6.
base::EntityId cornerId(const TypeShape& type, std::span<const base::EntityId> ids, std::size_t corner)
{
    const std::size_t slot = type.alternating ? 2 * corner : corner;
    return slot < ids.size() ? ids[slot] : 0;
}

// Sorts the first `count` (at most 4) entries and removes the repeated ones; returns how many distinct values are left.
std::size_t sortDistinct(std::array<base::EntityId, 4>& values, std::size_t count) noexcept
{
    count = std::min<std::size_t>(count, 4);
    for (std::size_t index = 1; index < count; ++index)
    {
        const base::EntityId value = values[index];
        std::size_t place = index;
        while (place > 0 && values[place - 1] > value)
        {
            values[place] = values[place - 1];
            --place;
        }
        values[place] = value;
    }
    std::size_t distinct = count == 0 ? 0 : 1;
    for (std::size_t index = 1; index < count; ++index)
    {
        if (values[index] != values[distinct - 1])
            values[distinct++] = values[index];
    }
    return distinct;
}

std::size_t cornerCount(ElementShape shape) noexcept
{
    switch (shape)
    {
    case ElementShape::Line:
        return 2;
    case ElementShape::Triangle:
        return 3;
    case ElementShape::Quadrilateral:
    case ElementShape::Tetrahedron:
        return 4;
    case ElementShape::Pyramid:
        return 5;
    case ElementShape::Pentahedron:
        return 6;
    case ElementShape::Hexahedron:
        return 8;
    default:
        return 0;
    }
}

} // namespace

ElementShape ElementTopology::shapeOf(std::uint16_t typeIndex) noexcept
{
    const std::vector<TypeShape>& table = shapes();
    return typeIndex < table.size() ? table[typeIndex].shape : ElementShape::None;
}

bool ElementTopology::alternatingCorners(std::uint16_t typeIndex) noexcept
{
    const std::vector<TypeShape>& table = shapes();
    return typeIndex < table.size() && table[typeIndex].alternating;
}

std::span<const std::array<std::uint8_t, 2>> ElementTopology::edgeTemplate(ElementShape shape) noexcept
{
    switch (shape)
    {
    case ElementShape::Line:
        return LINE_EDGES;
    case ElementShape::Triangle:
        return TRIANGLE_EDGES;
    case ElementShape::Quadrilateral:
        return QUAD_EDGES;
    case ElementShape::Tetrahedron:
        return TETRA_EDGES;
    case ElementShape::Pentahedron:
        return PENTA_EDGES;
    case ElementShape::Hexahedron:
        return HEXA_EDGES;
    case ElementShape::Pyramid:
        return PYRAMID_EDGES;
    default:
        return {};
    }
}

std::span<const FaceCorners> ElementTopology::faceTemplate(ElementShape shape) noexcept
{
    switch (shape)
    {
    case ElementShape::Triangle:
        return TRIANGLE_FACES;
    case ElementShape::Quadrilateral:
        return QUAD_FACES;
    case ElementShape::Tetrahedron:
        return TETRA_FACES;
    case ElementShape::Pentahedron:
        return PENTA_FACES;
    case ElementShape::Hexahedron:
        return HEXA_FACES;
    case ElementShape::Pyramid:
        return PYRAMID_FACES;
    default:
        return {};
    }
}

void ElementTopology::nodesOf(const model::Model& model, model::CardLocation location,
                              std::vector<base::EntityId>& nodes)
{
    const model::CardTable* table = model.findTable(location.table);
    if (table != nullptr)
        nodesOf(*table, location.table, location.row, nodes);
}

void ElementTopology::nodesOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                              std::vector<base::EntityId>& nodes)
{
    const TypeShape& type = shapes()[typeIndex];
    if (type.shape == ElementShape::None)
        return;
    const detail::ElementProvider* provider = detail::ProviderRegistry::instance().find(typeIndex);
    std::vector<base::EntityId>& ids = scratchIds();
    ids.clear();
    provider->nodes(table, row, ids);
    if (type.shape == ElementShape::Line)
    {
        // The end nodes only: the orientation node of a bar, a bush or a bend does not connect. A CBEAM3 has a third
        // node, the middle one, in the fourth slot.
        const std::size_t slots[3] = {0, 1, 3};
        const std::size_t count = type.threeNode ? 3 : 2;
        for (std::size_t index = 0; index < count && slots[index] < ids.size(); ++index)
        {
            if (ids[slots[index]] != 0)
                nodes.push_back(ids[slots[index]]);
        }
        return;
    }
    for (const base::EntityId id : ids)
    {
        if (id != 0)
            nodes.push_back(id);
    }
}

void ElementTopology::edgesOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                              std::vector<EdgeKey>& edges)
{
    const TypeShape& type = shapes()[typeIndex];
    if (type.shape == ElementShape::None || type.shape == ElementShape::Point)
        return;
    std::vector<base::EntityId>& ids = scratchIds();
    ids.clear();
    detail::ProviderRegistry::instance().find(typeIndex)->nodes(table, row, ids);
    std::array<base::EntityId, 8> corners{};
    const std::size_t count = cornerCount(type.shape);
    for (std::size_t corner = 0; corner < count; ++corner)
        corners[corner] = cornerId(type, ids, corner);
    for (const auto& edge : edgeTemplate(type.shape))
    {
        const base::EntityId first = corners[edge[0]];
        const base::EntityId second = corners[edge[1]];
        if (first == 0 || second == 0 || first == second)
            continue;
        edges.push_back(first < second ? EdgeKey{first, second} : EdgeKey{second, first});
    }
}

void ElementTopology::facesOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                              std::vector<FaceKey>& faces)
{
    const TypeShape& type = shapes()[typeIndex];
    const int dimension = dimensionOf(type.shape);
    if (dimension < 2)
        return;
    std::vector<base::EntityId>& ids = scratchIds();
    ids.clear();
    detail::ProviderRegistry::instance().find(typeIndex)->nodes(table, row, ids);
    std::array<base::EntityId, 8> corners{};
    const std::size_t count = cornerCount(type.shape);
    for (std::size_t corner = 0; corner < count; ++corner)
        corners[corner] = cornerId(type, ids, corner);
    for (const FaceCorners& face : faceTemplate(type.shape))
    {
        std::array<base::EntityId, 4> sorted{};
        bool blank = false;
        for (std::size_t index = 0; index < face.count; ++index)
        {
            sorted[index] = corners[face.corner[index]];
            blank = blank || sorted[index] == 0;
        }
        if (blank)
            continue;
        const std::size_t distinct = sortDistinct(sorted, face.count);
        if (distinct < 3)
            continue;
        for (std::size_t index = distinct; index < 4; ++index)
            sorted[index] = 0;
        faces.push_back(FaceKey{sorted});
    }
}

void ElementTopology::slotsOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                              std::vector<base::EntityId>& nodes)
{
    if (const auto* provider = detail::ProviderRegistry::instance().find(typeIndex))
        provider->nodes(table, row, nodes);
}

void ElementTopology::orientedFacesOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                                      std::vector<std::array<base::EntityId, 4>>& faces)
{
    const TypeShape& type = shapes()[typeIndex];
    if (dimensionOf(type.shape) < 2)
        return;
    std::vector<base::EntityId>& ids = scratchIds();
    ids.clear();
    detail::ProviderRegistry::instance().find(typeIndex)->nodes(table, row, ids);
    for (const FaceCorners& face : faceTemplate(type.shape))
    {
        std::array<base::EntityId, 4> nodes{};
        for (std::size_t index = 0; index < face.count; ++index)
            nodes[index] = cornerId(type, ids, face.corner[index]);
        faces.push_back(nodes);
    }
}

void ElementTopology::orientedFaceMidSidesOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                                             std::vector<std::array<base::EntityId, 4>>& midSides)
{
    const TypeShape& type = shapes()[typeIndex];
    if (dimensionOf(type.shape) < 2)
        return;
    std::vector<base::EntityId>& ids = scratchIds();
    ids.clear();
    detail::ProviderRegistry::instance().find(typeIndex)->nodes(table, row, ids);
    const std::span<const std::array<std::uint8_t, 2>> edges = midSideTemplate(type.shape);
    const std::size_t corners = cornerCount(type.shape);
    // The card slot of the mid-side node of an edge given by two corner indices, or nothing.
    const auto midSideOf = [&](std::uint8_t first, std::uint8_t second) -> base::EntityId
    {
        for (std::size_t edge = 0; edge < edges.size(); ++edge)
        {
            const bool same = edges[edge][0] == first && edges[edge][1] == second;
            const bool reversed = edges[edge][0] == second && edges[edge][1] == first;
            if (!same && !reversed)
                continue;
            // CTRIAX6 alternates corners and mid-side nodes: G2 between G1 and G3, and so on.
            const std::size_t slot = type.alternating ? 2 * edge + 1 : corners + edge;
            return slot < ids.size() ? ids[slot] : 0;
        }
        return 0;
    };
    for (const FaceCorners& face : faceTemplate(type.shape))
    {
        std::array<base::EntityId, 4> nodes{};
        for (std::size_t index = 0; index < face.count; ++index)
            nodes[index] = midSideOf(face.corner[index], face.corner[(index + 1) % face.count]);
        midSides.push_back(nodes);
    }
}

} // namespace oor::analysis
