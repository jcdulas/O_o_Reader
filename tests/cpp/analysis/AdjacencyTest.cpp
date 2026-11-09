#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/adjacency/MeshAdjacency.hpp"
#include "oor/model/cards/elements/solidHexa/SolidHexaTable.hpp"
#include "oor/model/cards/elements/solidPenta/SolidPentaTable.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

using oor::testing::modelRecord;

using namespace oor;
using namespace oor::analysis;
using namespace oor::model;
using oor::testing::readModelText;

namespace
{

using Ids = std::vector<base::EntityId>;

const char* const PROPERTIES = "IsoMat;1;2.1E+5;;0.3\nShellProp;1;1;0.1\nSolidProp;2;1\nRodProp;3;1;1.\n";

std::string grid(int id, double x, double y, double z)
{
    return modelRecord("Node", id, "", x, y, z);
}

// A rectangular grid of nx by ny elements in the x-y plane: the nodes are numbered along x from 1, the elements
// likewise.
std::string plate(int nx, int ny)
{
    std::string deck;
    for (int j = 0; j <= ny; ++j)
    {
        for (int i = 0; i <= nx; ++i)
            deck += grid(1 + i + j * (nx + 1), i, j, 0.0);
    }
    int eid = 1;
    for (int j = 0; j < ny; ++j)
    {
        for (int i = 0; i < nx; ++i)
        {
            const int n = 1 + i + j * (nx + 1);
            deck += modelRecord("ShellQuad4", eid++, 1, n, n + 1, n + nx + 2, n + nx + 1);
        }
    }
    return deck;
}

// A block of nx by ny by nz hexahedra: the nodes are numbered along x, then y, then z, from 1; the elements likewise,
// from 1.
std::string block(int nx, int ny, int nz)
{
    std::string deck;
    const int row = nx + 1;
    const int layer = row * (ny + 1);
    for (int k = 0; k <= nz; ++k)
    {
        for (int j = 0; j <= ny; ++j)
        {
            for (int i = 0; i <= nx; ++i)
                deck += grid(1 + i + j * row + k * layer, i, j, k);
        }
    }
    int eid = 1;
    for (int k = 0; k < nz; ++k)
    {
        for (int j = 0; j < ny; ++j)
        {
            for (int i = 0; i < nx; ++i)
            {
                const int n = 1 + i + j * row + k * layer;
                deck += modelRecord("SolidHexa", eid++, 2, n, n + 1, n + 1 + row, n + row, n + layer, n + 1 + layer,
                                    n + 1 + row + layer, n + row + layer);
            }
        }
    }
    return deck;
}

CardLocation locationOf(const Model& model, base::EntityId eid)
{
    const auto location = model.locate(base::IdSpace::Element, eid);
    REQUIRE(location.has_value());
    return *location;
}

Ids neighbours(const Model& model, base::EntityId eid, AdjacencyKind kind, const std::vector<std::uint16_t>& types = {})
{
    return MeshAdjacency::adjacentElements(model, locationOf(model, eid), kind, types);
}

} // namespace

TEST_CASE("the neighbours of the elements of a small plate are those known by hand", "[analysis][adjacency]")
{
    // Four quadrilaterals on a 3 by 3 grid of nodes:
    //   7---8---9
    //   | 3 | 4 |
    //   4---5---6
    //   | 1 | 2 |
    //   1---2---3
    const Model model = readModelText(std::string(PROPERTIES) + plate(2, 2));
    REQUIRE(neighbours(model, 1, AdjacencyKind::Edge) == Ids{2, 3});
    REQUIRE(neighbours(model, 2, AdjacencyKind::Edge) == Ids{1, 4});
    REQUIRE(neighbours(model, 3, AdjacencyKind::Edge) == Ids{1, 4});
    REQUIRE(neighbours(model, 4, AdjacencyKind::Edge) == Ids{2, 3});
    // By node every element touches the three others, at the centre node 5.
    REQUIRE(neighbours(model, 1, AdjacencyKind::Node) == Ids{2, 3, 4});
    REQUIRE(neighbours(model, 4, AdjacencyKind::Node) == Ids{1, 2, 3});
    // No two have the same four corners.
    REQUIRE(neighbours(model, 1, AdjacencyKind::Face).empty());
    // The elements that use a node.
    REQUIRE(MeshAdjacency::elementsOfNode(model, 5) == Ids{1, 2, 3, 4});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 1) == Ids{1});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 2) == Ids{1, 2});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 9) == Ids{4});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 99).empty());
}

TEST_CASE("the free edges of a plate are its boundary, with the one shell that owns each", "[analysis][adjacency]")
{
    const Model model = readModelText(std::string(PROPERTIES) + plate(2, 2));
    const FreeEdges edges = MeshAdjacency::freeEdges(model);
    using Edge = std::array<base::EntityId, 2>;
    const std::vector<Edge> expected = {{1, 2}, {1, 4}, {2, 3}, {3, 6}, {4, 7}, {6, 9}, {7, 8}, {8, 9}};
    REQUIRE(edges.nodes == expected);
    REQUIRE(edges.elements == Ids{1, 1, 2, 2, 3, 4, 3, 4});
    // The free faces of a model without solids: none.
    REQUIRE(MeshAdjacency::freeFaces(model).nodes.empty());
}

TEST_CASE("a block of hexahedra shares faces, edges and nodes as the cube they make", "[analysis][adjacency]")
{
    // 2 by 2 by 1: elements 1 and 2 in the first row, 3 and 4 in the second.
    const Model model = readModelText(std::string(PROPERTIES) + block(2, 2, 1));
    REQUIRE(neighbours(model, 1, AdjacencyKind::Face) == Ids{2, 3});
    REQUIRE(neighbours(model, 4, AdjacencyKind::Face) == Ids{2, 3});
    // The element diagonal to it shares an edge (the vertical one at the centre), not a face.
    REQUIRE(neighbours(model, 1, AdjacencyKind::Edge) == Ids{2, 3, 4});
    REQUIRE(neighbours(model, 1, AdjacencyKind::Node) == Ids{2, 3, 4});
    // Twelve interior-and-boundary nodes of the middle column belong to all four elements.
    REQUIRE(MeshAdjacency::elementsOfNode(model, 5) == Ids{1, 2, 3, 4});
    // The surface of the block: 4 + 4 on the top and the bottom, 8 on the sides.
    const FreeFaces faces = MeshAdjacency::freeFaces(model);
    REQUIRE(faces.nodes.size() == 16);
    REQUIRE(faces.elements.size() == 16);
    // The faces of element 1 that are free: bottom (1 4 5 2: seen from below the corners run G1 G4 G3 G2), top, the
    // sides x = 0 and y = 0; each has its four corners among the element's, and none has 0.
    std::size_t ofFirst = 0;
    for (std::size_t index = 0; index < faces.nodes.size(); ++index)
    {
        if (faces.elements[index] == 1)
            ++ofFirst;
        for (const base::EntityId corner : faces.nodes[index])
            REQUIRE(corner != 0);
    }
    REQUIRE(ofFirst == 4);
    // The bottom face of element 1 is listed in the order of the outward normal: G1 G4 G3 G2.
    const std::array<base::EntityId, 4> bottom = {1, 4, 5, 2};
    REQUIRE(std::find(faces.nodes.begin(), faces.nodes.end(), bottom) != faces.nodes.end());
}

TEST_CASE("a shell on the face of a solid is its neighbour by face, a rod on its edge by edge", "[analysis][adjacency]")
{
    // Element 1: a hexahedron on nodes 1-8; element 20: a quadrilateral on its top face (nodes 5 6 7 8); element 30: a
    // rod on its edge 1-2; element 40: a bar between 1 and 9 with its orientation vector given by the node 8 (not a
    // connection).
    const Model model =
        readModelText(std::string(PROPERTIES) + block(1, 1, 1) + grid(9, 5, 5, 5) +
                      "ShellQuad4;20;1;5;6;7;8\nRod2;30;3;1;2\nBar2;40;3;1;9;;8\nBar2;41;3;8;9;;1\n");
    REQUIRE(neighbours(model, 1, AdjacencyKind::Face) == Ids{20});
    REQUIRE(neighbours(model, 20, AdjacencyKind::Face) == Ids{1});
    REQUIRE(neighbours(model, 1, AdjacencyKind::Edge) == Ids{20, 30});
    REQUIRE(neighbours(model, 30, AdjacencyKind::Edge) == Ids{1});
    // The bar between 1 and 9 touches the solid at node 1 only: the orientation node 8 does not connect it.
    REQUIRE(neighbours(model, 40, AdjacencyKind::Node) == Ids{1, 30, 41});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 8) == Ids{1, 20, 41});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 9) == Ids{40, 41});
    // The type filter keeps one card type.
    const std::uint16_t quadrilateral = typeIndexOf<ShellQuad4Table>();
    REQUIRE(neighbours(model, 1, AdjacencyKind::Node, {quadrilateral}) == Ids{20});
    REQUIRE(neighbours(model, 1, AdjacencyKind::Node) == Ids{20, 30, 40, 41});
    // The shell that closes the face does not make the face of the solid closed for the free faces; the rod does not
    // close edges.
    const FreeFaces faces = MeshAdjacency::freeFaces(model);
    REQUIRE(faces.nodes.size() == 6);
    // The free edges: the shell has four, none of them used by another shell.
    const FreeEdges edges = MeshAdjacency::freeEdges(model);
    REQUIRE(edges.nodes.size() == 4);
}

TEST_CASE("mid-side nodes count as nodes of an element but not as corners of its edges", "[analysis][adjacency]")
{
    // A CQUAD8 and a CQUAD4 side by side; the CQUAD8 has a mid-side node on its right side (G6 between G2 and G3), a
    // node nobody else uses; an unrelated CQUAD4 uses the mid-side node of the CQUAD8 as a corner.
    const Model model = readModelText(std::string(PROPERTIES) +
                                      "Node;1;;0.;0.;0.\n"
                                      "Node;2;;1.;0.;0.\n"
                                      "Node;3;;1.;1.;0.\n"
                                      "Node;4;;0.;1.;0.\n"
                                      "Node;5;;2.;0.;0.\n"
                                      "Node;6;;2.;1.;0.\n"
                                      "Node;7;;1.;0.5;0.\n"
                                      "Node;8;;3.;0.;0.\n"
                                      "Node;9;;3.;1.;0.\n"
                                      "ShellQuad8;10;1;1;2;3;4;;7\n"
                                      "ShellQuad4;11;1;2;5;6;3\n"
                                      "ShellQuad4;12;1;7;8;9;6\n");
    REQUIRE(neighbours(model, 10, AdjacencyKind::Edge) == Ids{11});
    // Element 12 shares the node 7 with element 10 (a mid-side node), and the node 6 with element 11.
    REQUIRE(neighbours(model, 10, AdjacencyKind::Node) == Ids{11, 12});
    REQUIRE(neighbours(model, 12, AdjacencyKind::Edge).empty());
    REQUIRE(MeshAdjacency::elementsOfNode(model, 7) == Ids{10, 12});
}

TEST_CASE("two tetrahedra on one face are face neighbours, and the free faces are the six others",
          "[analysis][adjacency]")
{
    const Model model =
        readModelText(std::string(PROPERTIES) + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 0, 1, 0) +
                      grid(4, 0, 0, 1) + grid(5, 1, 1, 1) + "SolidTetra;10;2;1;2;3;4\nSolidTetra;11;2;2;5;3;4\n");
    REQUIRE(neighbours(model, 10, AdjacencyKind::Face) == Ids{11});
    REQUIRE(neighbours(model, 10, AdjacencyKind::Edge) == Ids{11});
    const FreeFaces faces = MeshAdjacency::freeFaces(model);
    REQUIRE(faces.nodes.size() == 6);
    // A face of the first tetrahedron, with the outward orientation of the positive element: the one opposite to G4 is
    // G1 G3 G2.
    const std::array<base::EntityId, 4> bottom = {1, 3, 2, 0};
    REQUIRE(std::find(faces.nodes.begin(), faces.nodes.end(), bottom) != faces.nodes.end());
}

TEST_CASE("scenario S2: the neighbours of a strip of elements before and after the deletion of one",
          "[analysis][adjacency]")
{
    // 1---2---3---4
    // | 1 | 2 | 3 |
    // 5---6---7---8
    Model model = readModelText(std::string(PROPERTIES) + plate(3, 1));
    REQUIRE(neighbours(model, 2, AdjacencyKind::Edge) == Ids{1, 3});
    REQUIRE(neighbours(model, 1, AdjacencyKind::Edge) == Ids{2});
    REQUIRE(neighbours(model, 1, AdjacencyKind::Node) == Ids{2});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 3) == Ids{2, 3});
    REQUIRE(MeshAdjacency::statistics(model).edgeTableBuilds == 1);
    REQUIRE(MeshAdjacency::freeEdges(model).nodes.size() == 8);

    model.remove(locationOf(model, 2));
    REQUIRE(neighbours(model, 1, AdjacencyKind::Edge).empty());
    REQUIRE(neighbours(model, 3, AdjacencyKind::Edge).empty());
    REQUIRE(neighbours(model, 3, AdjacencyKind::Node).empty());
    REQUIRE(MeshAdjacency::elementsOfNode(model, 3) == Ids{3});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 2) == Ids{1});
    // The tables were rebuilt once for the deletion; both elements now have all their four edges free.
    const AdjacencyCache::Statistics stats = MeshAdjacency::statistics(model);
    REQUIRE(stats.edgeTableBuilds == 2);
    REQUIRE(stats.invalidations >= 1);
    REQUIRE(MeshAdjacency::freeEdges(model).nodes.size() == 8);
    // Putting an element back restores the neighbours.
    ShellQuad4Card card;
    card.id = 2;
    card.propId = 1;
    card.nodes = {2, 3, 7, 6};
    model.add<ShellQuad4Table>(card);
    REQUIRE(neighbours(model, 2, AdjacencyKind::Edge) == Ids{1, 3});
    REQUIRE(neighbours(model, 1, AdjacencyKind::Edge) == Ids{2});
}

TEST_CASE("a coordinate edit does not rebuild the tables, a structural edit does", "[analysis][adjacency][cache]")
{
    Model model = readModelText(std::string(PROPERTIES) + plate(3, 2));
    (void)neighbours(model, 1, AdjacencyKind::Edge);
    (void)neighbours(model, 1, AdjacencyKind::Node);
    const AdjacencyCache::Statistics before = MeshAdjacency::statistics(model);
    REQUIRE(before.nodeIndexBuilds == 1);
    REQUIRE(before.edgeTableBuilds == 1);
    const std::uint64_t version = model.structureVersion();

    // Moving a node, changing its coordinate system, the property or the thickness of an element: nothing structural.
    NodeTable& grids = model.table<NodeTable>();
    grids.setXyz(model.locate(base::IdSpace::Node, 3)->row, base::Vec3{9.0, 9.0, 9.0});
    grids.setPositionFrameId(model.locate(base::IdSpace::Node, 4)->row, 0);
    (void)neighbours(model, 2, AdjacencyKind::Edge);
    (void)neighbours(model, 2, AdjacencyKind::Node);
    (void)MeshAdjacency::freeEdges(model);
    const AdjacencyCache::Statistics after = MeshAdjacency::statistics(model);
    REQUIRE(model.structureVersion() == version);
    REQUIRE(after.nodeIndexBuilds == before.nodeIndexBuilds);
    REQUIRE(after.edgeTableBuilds == before.edgeTableBuilds);
    REQUIRE(after.invalidations == 0);

    // Adding an element is structural: both tables are built again, when they are asked for.
    ShellQuad4Card card;
    card.id = 50;
    card.propId = 1;
    card.nodes = {1, 2, 6, 5};
    model.add<ShellQuad4Table>(card);
    REQUIRE(model.structureVersion() != version);
    (void)neighbours(model, 1, AdjacencyKind::Edge);
    REQUIRE(MeshAdjacency::statistics(model).edgeTableBuilds == before.edgeTableBuilds + 1);
    REQUIRE(MeshAdjacency::statistics(model).nodeIndexBuilds == before.nodeIndexBuilds); // not asked for yet
    (void)neighbours(model, 1, AdjacencyKind::Node);
    REQUIRE(MeshAdjacency::statistics(model).nodeIndexBuilds == before.nodeIndexBuilds + 1);

    // A connectivity edit that the editing layer reports.
    const std::uint64_t next = model.structureVersion();
    model.table<ShellQuad4Table>().setNodes(locationOf(model, 50).row, {2, 3, 7, 6});
    model.noteStructureChange();
    REQUIRE(model.structureVersion() != next);
    // The same corners as element 2: it shares all the edges of that one, and the edges of 1, 3 and 5 on the other
    // sides.
    REQUIRE(neighbours(model, 50, AdjacencyKind::Edge) == Ids{1, 2, 3, 5});
    REQUIRE(MeshAdjacency::statistics(model).edgeTableBuilds == before.edgeTableBuilds + 2);

    // A compaction moves the rows: the tables are built again and still say the same.
    model.remove(locationOf(model, 50));
    model.compact();
    REQUIRE(neighbours(model, 2, AdjacencyKind::Edge) == Ids{1, 3, 5});
    REQUIRE(MeshAdjacency::statistics(model).edgeTableBuilds == before.edgeTableBuilds + 3);
}

TEST_CASE("a renumbering invalidates the tables and the neighbours are those of the new IDs",
          "[analysis][adjacency][cache]")
{
    Model model = readModelText(std::string(PROPERTIES) + plate(2, 2));
    REQUIRE(neighbours(model, 1, AdjacencyKind::Edge) == Ids{2, 3});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 5) == Ids{1, 2, 3, 4});
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(10));
    REQUIRE(neighbours(model, 11, AdjacencyKind::Edge) == Ids{12, 13});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 105) == Ids{11, 12, 13, 14});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 5).empty());
    const FreeEdges edges = MeshAdjacency::freeEdges(model);
    REQUIRE(edges.nodes.front() == std::array<base::EntityId, 2>{101, 102});
    REQUIRE(edges.elements.front() == 11);
}

TEST_CASE("a copy has its own tables, in the state of the original, and edits stay on their side",
          "[analysis][adjacency][cache]")
{
    Model model = readModelText(std::string(PROPERTIES) + plate(3, 1));
    (void)neighbours(model, 1, AdjacencyKind::Edge);
    Model copy = model.copy();
    // The copy starts with the tables of the original: no build.
    REQUIRE(neighbours(copy, 2, AdjacencyKind::Edge) == Ids{1, 3});
    REQUIRE(MeshAdjacency::statistics(copy).edgeTableBuilds == 1);

    copy.remove(locationOf(copy, 2));
    REQUIRE(neighbours(copy, 1, AdjacencyKind::Edge).empty());
    REQUIRE(MeshAdjacency::statistics(copy).edgeTableBuilds == 2);
    // The original is untouched: same neighbours, no rebuild.
    REQUIRE(neighbours(model, 2, AdjacencyKind::Edge) == Ids{1, 3});
    REQUIRE(MeshAdjacency::statistics(model).edgeTableBuilds == 1);
    REQUIRE(MeshAdjacency::statistics(model).invalidations == 0);
}

TEST_CASE(
    "the tables built over threads equal the ones built on one, and the vectorised answers equal the one-by-one ones",
    "[analysis][adjacency]")
{
    const Model model = readModelText(std::string(PROPERTIES) + plate(130, 130));
    const std::uint16_t type = typeIndexOf<ShellQuad4Table>();
    const CardTable& table = *model.findTable(type);
    std::vector<CardLocation> locations;
    for (std::uint32_t row = 0; row < table.rows().rowCount(); ++row)
        locations.push_back(CardLocation{type, row});

    const NodeToElementIndex serialNodes = NodeToElementIndex::build(model, nullptr);
    const EdgeTable serialEdges = EdgeTable::build(model, nullptr);
    const FaceTable serialFaces = FaceTable::build(model, nullptr);
    base::ThreadPool pool(3);
    const NodeToElementIndex threadedNodes = NodeToElementIndex::build(model, &pool);
    const EdgeTable threadedEdges = EdgeTable::build(model, &pool);
    const FaceTable threadedFaces = FaceTable::build(model, &pool);
    REQUIRE(serialNodes.nodeCount() == 131U * 131U);
    REQUIRE(serialNodes.entryCount() == 4U * 16900U);
    REQUIRE(serialEdges.edgeCount() == 2U * 130U * 131U);
    REQUIRE(serialEdges.entryCount() == 4U * 16900U);
    REQUIRE(serialFaces.faceCount() == 16900U);
    REQUIRE(threadedNodes.nodeCount() == serialNodes.nodeCount());
    REQUIRE(std::equal(serialEdges.edges().begin(), serialEdges.edges().end(), threadedEdges.edges().begin(),
                       threadedEdges.edges().end()));
    for (std::size_t edge = 0; edge < serialEdges.edgeCount(); edge += 17)
    {
        const auto a = serialEdges.elements(edge);
        const auto b = threadedEdges.elements(edge);
        REQUIRE(std::equal(a.begin(), a.end(), b.begin(), b.end()));
    }
    for (std::size_t node = 0; node < serialNodes.nodeCount(); node += 13)
    {
        const auto a = serialNodes.elements(node);
        const auto b = threadedNodes.elements(node);
        REQUIRE(std::equal(a.begin(), a.end(), b.begin(), b.end()));
    }
    // An interior node has four elements, a corner one.
    REQUIRE(serialNodes.elementsOf(66 + 65 * 131).size() == 4);
    REQUIRE(serialNodes.elementsOf(1).size() == 1);

    for (const AdjacencyKind kind : {AdjacencyKind::Node, AdjacencyKind::Edge, AdjacencyKind::Face})
    {
        const AdjacencyCsr serial = MeshAdjacency::adjacency(model, locations, kind, {}, nullptr);
        const AdjacencyCsr threaded = MeshAdjacency::adjacency(model, locations, kind, {}, &pool);
        REQUIRE(serial.offsets == threaded.offsets);
        REQUIRE(serial.values == threaded.values);
        REQUIRE(serial.offsets.size() == locations.size() + 1);
        for (std::size_t index = 0; index < locations.size(); index += 97)
        {
            const Ids one = MeshAdjacency::adjacentElements(model, locations[index], kind);
            const Ids row(serial.values.begin() + serial.offsets[index],
                          serial.values.begin() + serial.offsets[index + 1]);
            REQUIRE(row == one);
        }
    }
    // A bulk answer for a few nodes.
    const std::vector<base::EntityId> nodes = {1, 66 + 65 * 131, 12345678};
    const AdjacencyCsr users = MeshAdjacency::nodeElements(model, nodes);
    REQUIRE(users.offsets == std::vector<std::int64_t>{0, 1, 5, 5});
    // 4 x 130 free edges on the boundary of the 130 by 130 plate.
    REQUIRE(MeshAdjacency::freeEdges(model, &pool).nodes.size() == 520);
}

TEST_CASE("the mid-side nodes of each face follow its edges in the order of the format reference",
          "[analysis][adjacency]")
{
    // A SolidHexa with its twenty nodes 1 to 20 and a SolidPenta with its fifteen 101 to 115, in card order: the format
    // reference numbers the mid-side slots by edges in another order than the edge templates (CHEXA G13 to G16 between
    // the two faces, then G17 to G20 on the second one; CPENTA G10 to G12 between the triangles), so each face finds
    // its own.
    std::string text = PROPERTIES;
    for (int id = 1; id <= 20; ++id)
        text += grid(id, id, 0.0, 0.0);
    for (int id = 101; id <= 115; ++id)
        text += grid(id, id, 1.0, 0.0);
    text += "SolidHexa;1;2;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;19;20\n";
    text += "SolidPenta;2;2;101;102;103;104;105;106;107;108;109;110;111;112;113;114;115\n";
    const Model model = readModelText(text);

    const std::uint16_t hexa = typeIndexOf<SolidHexaTable>();
    std::vector<std::array<base::EntityId, 4>> corners;
    std::vector<std::array<base::EntityId, 4>> midSides;
    ElementTopology::orientedFacesOf(*model.findTable(hexa), hexa, 0, corners);
    ElementTopology::orientedFaceMidSidesOf(*model.findTable(hexa), hexa, 0, midSides);
    REQUIRE(midSides.size() == corners.size());
    // the face G1 G4 G3 G2: edges 1-4 (G12), 4-3 (G11), 3-2 (G10), 2-1 (G9); the face G5..G8: G17 to G20; the side G1
    // G2 G6 G5: G9, G14, G17, G13
    CHECK(corners[0] == std::array<base::EntityId, 4>{1, 4, 3, 2});
    CHECK(midSides[0] == std::array<base::EntityId, 4>{12, 11, 10, 9});
    CHECK(midSides[1] == std::array<base::EntityId, 4>{17, 18, 19, 20});
    CHECK(corners[2] == std::array<base::EntityId, 4>{1, 2, 6, 5});
    CHECK(midSides[2] == std::array<base::EntityId, 4>{9, 14, 17, 13});

    const std::uint16_t penta = typeIndexOf<SolidPentaTable>();
    corners.clear();
    midSides.clear();
    ElementTopology::orientedFacesOf(*model.findTable(penta), penta, 0, corners);
    ElementTopology::orientedFaceMidSidesOf(*model.findTable(penta), penta, 0, midSides);
    // the triangle G1 G3 G2: G9, G8, G7; the side G1 G2 G5 G4: G7, G11, G13, G10
    CHECK(midSides[0] == std::array<base::EntityId, 4>{109, 108, 107, 0});
    CHECK(corners[2] == std::array<base::EntityId, 4>{101, 102, 105, 104});
    CHECK(midSides[2] == std::array<base::EntityId, 4>{107, 111, 113, 110});
}
