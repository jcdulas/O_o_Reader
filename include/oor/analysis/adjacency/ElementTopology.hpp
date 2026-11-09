#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace oor::analysis
{

// The shape of an element as far as connectivity is concerned (the PLOTEL, which is for plotting and whose IDs are not
// in the element ID space, is not an element here): which of its nodes are corners, which are edges and faces. Mid-side
// nodes never define an edge or a face (a CQUAD8 and a CQUAD4 that share two corners share an edge); they are nodes the
// element uses.
enum class ElementShape : std::uint8_t
{
    None,          // not an element, or an element that has no nodes in the model
    Point,         // an element attached at its nodes and nothing else: masses, springs between scalar points, rigid
                   // elements, connectors; it has no edge and no face
    Line,          // two end nodes joined by an edge: rods, bars, beams, bushes, springs and dampers
    Triangle,      // 3 corners (and up to 3 mid-side nodes): one face, three edges
    Quadrilateral, // 4 corners (and up to 5 other nodes): one face, four edges
    Tetrahedron,   // 4 corners (and up to 6 mid-side nodes): four triangular faces, six edges
    Pentahedron,   // 6 corners: two triangular and three quadrilateral faces, nine edges
    Hexahedron,    // 8 corners: six quadrilateral faces, twelve edges
    Pyramid        // 5 corners: one quadrilateral and four triangular faces, eight edges
};

// 0 for a point, 1 for a line, 2 for a shell, 3 for a solid; -1 for None.
[[nodiscard]] constexpr int dimensionOf(ElementShape shape) noexcept
{
    switch (shape)
    {
    case ElementShape::Point:
        return 0;
    case ElementShape::Line:
        return 1;
    case ElementShape::Triangle:
    case ElementShape::Quadrilateral:
        return 2;
    case ElementShape::Tetrahedron:
    case ElementShape::Pentahedron:
    case ElementShape::Hexahedron:
    case ElementShape::Pyramid:
        return 3;
    case ElementShape::None:
        break;
    }
    return -1;
}

// An element of the model by its place in its table: the card type and the row. It is what the indexes hold; it is
// valid until the connectivity of the model changes (Model::structureVersion), because a compaction moves rows.
struct ElementRef
{
    std::uint32_t row = 0;
    std::uint16_t type = 0;

    [[nodiscard]] bool operator==(const ElementRef&) const noexcept = default;
};

// An edge or a face by the IDs of its corner nodes in increasing order (a face of three corners has 0 last).
struct EdgeKey
{
    base::EntityId first = 0;
    base::EntityId second = 0;
    [[nodiscard]] bool operator==(const EdgeKey&) const noexcept = default;
    [[nodiscard]] bool operator<(const EdgeKey& other) const noexcept
    {
        return first != other.first ? first < other.first : second < other.second;
    }
};
struct FaceKey
{
    std::array<base::EntityId, 4> nodes{};
    [[nodiscard]] bool operator==(const FaceKey&) const noexcept = default;
    [[nodiscard]] bool operator<(const FaceKey& other) const noexcept { return nodes < other.nodes; }
};

// The corners of a face of an element in the order that gives the outward normal of an element whose Jacobian is
// positive (the orientation of the format reference: G1 G2 G3 counter-clockwise seen from G4 for a tetrahedron, G1..G4
// counter-clockwise seen from G5..G8 for a hexahedron). `count` is 3 or 4.
struct FaceCorners
{
    std::uint8_t count = 0;
    std::array<std::uint8_t, 4> corner{};
};

// What the connectivity layer knows about an element card type, and how to read the nodes of an element. One function
// per use; the shape of each card type is decided in ElementTopology.cpp.
class ElementTopology
{
public:
    // The shape of the elements of a card type (ElementShape::None for a card type that is not an element).
    [[nodiscard]] static ElementShape shapeOf(std::uint16_t typeIndex) noexcept;

    // True when the card type is an element that the connectivity layer treats (shape other than None).
    [[nodiscard]] static bool supports(std::uint16_t typeIndex) noexcept
    {
        return shapeOf(typeIndex) != ElementShape::None;
    }

    // True for the card type whose corners are every other node of the card (CTRIAX6: G1, G3 and G5 are the corners,
    // G2, G4 and G6 the mid-side nodes between them).
    [[nodiscard]] static bool alternatingCorners(std::uint16_t typeIndex) noexcept;

    // The nodes the element in `row` is attached to: every node ID the card gives that is not blank, the corners and
    // the mid-side nodes; the orientation node of a bar or a bush is not one of them. Appended to `nodes`.
    static void nodesOf(const model::Model& model, model::CardLocation location, std::vector<base::EntityId>& nodes);
    // The same for a table and a row.
    static void nodesOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                        std::vector<base::EntityId>& nodes);

    // The node IDs of the element in `row` at their place on the card: corners, then mid-side points, 0 for a blank
    // field
    // (`nodesOf` leaves the blanks out). Appended to `nodes`.
    static void slotsOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                        std::vector<base::EntityId>& nodes);

    // The edges of the element in `row`, each by the keys of its end nodes. A line gives its one edge; a shell the
    // sides of its corner polygon; a solid its edges. An edge with a blank corner or the same node twice is left out.
    static void edgesOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                        std::vector<EdgeKey>& edges);
    // The faces of the element in `row`: a shell is its own face, a solid has its faces. A face that has fewer than
    // three distinct corners is left out; one with a repeated corner (a collapsed hexahedron) counts as a triangle.
    static void facesOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                        std::vector<FaceKey>& faces);

    // The faces of an element with their corners in the order of the card and the outward orientation: `faces` gets one
    // entry per face of the template of the shape, with the node IDs (0 for a corner that is blank). For a shell the
    // one face is the element itself. Nothing for a point or a line.
    static void orientedFacesOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                                std::vector<std::array<base::EntityId, 4>>& faces);
    // The mid-side nodes of the same faces, parallel to orientedFacesOf: for each face, entry k is the node on the edge
    // from its corner k to its corner k + 1 (the last corner to the first), 0 when the card leaves it blank or the
    // element has no mid-side node. The mid-side slots follow the format reference: the edges of the shape in the order of the card
    // (CHEXA: G9 to G12 on the edges of G1..G4, G13 to G16 from G1..G4 to G5..G8, G17 to G20 on the edges of G5..G8;
    // CPENTA: G7 to G9 on the first triangle, G10 to G12 between the triangles, G13 to G15 on the second one; CTRIAX6:
    // G2, G4, G6 between its corners).
    static void orientedFaceMidSidesOf(const model::CardTable& table, std::uint16_t typeIndex, std::uint32_t row,
                                       std::vector<std::array<base::EntityId, 4>>& midSides);

    // The edge and face templates of a shape, by corner index (the corners of the element in the order of the format reference: the
    // first slots of the card, except for CTRIAX6 whose corners are G1, G3 and G5). The quality metrics use them too.
    [[nodiscard]] static std::span<const std::array<std::uint8_t, 2>> edgeTemplate(ElementShape shape) noexcept;
    [[nodiscard]] static std::span<const FaceCorners> faceTemplate(ElementShape shape) noexcept;
};

} // namespace oor::analysis
