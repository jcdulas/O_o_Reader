// The geometry of the RBE2GS: the element has no connectivity, only the grid or point GS where the solver
// starts its search, so its only node is GS and every quantity is NaN.

#include "oor/analysis/adjacency/MeshAdjacency.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/rigid_elements/rigidBodySearched/RigidBodySearchedTable.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::analysis;
using namespace oor::model;
using oor::testing::readModelText;

namespace
{

// The record of a node: id, x, y, z (in the basic system).
std::string grid(int id, double x, double y, double z)
{
    return oor::testing::modelRecord("Node", id, "", x, y, z);
}

ElementValues geometryOf(const Model& model, base::EntityId eid)
{
    const auto location = model.locate(base::IdSpace::Element, eid);
    REQUIRE(location.has_value());
    return ElementGeometry::of(model, *location);
}

} // namespace

TEST_CASE("an RBE2GS is an element with no shape: every quantity is NaN", "[analysis][geometry]")
{
    const Model model = readModelText(grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + "GeomPoint;7;;1.;2.;3.\n" +
                                      "RigidBodySearched;10;1;;;0.5;;;;;;;2\n"
                                      "RigidBodySearched;11;7;;;0.5\n"
                                      "RigidBodySearched;12;;;;0.5;;;3.;0.;0.\n"); // a search location given as numbers
    REQUIRE(ElementGeometry::supports(typeIndexOf<RigidBodySearchedTable>()));
    for (const base::EntityId eid : {10, 11, 12})
    {
        const ElementValues values = geometryOf(model, eid);
        REQUIRE(std::isnan(values.center.x));
        REQUIRE(std::isnan(values.normal.x));
        REQUIRE(std::isnan(values.area));
        REQUIRE(std::isnan(values.volume));
        REQUIRE(std::isnan(values.length));
        REQUIRE(std::isnan(values.thickness));
        REQUIRE(std::isnan(values.frame.values[0]));
    }
}

TEST_CASE("an RBE2GS is attached to its search grid and to nothing else", "[analysis][geometry]")
{
    const Model model = readModelText(grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 4, 0, 0) +
                                      "RigidBodySearched;10;1;;;0.5;;;;;;;2\n"
                                      "RigidBodySearched;11;3;;;0.5\n"
                                      "RigidBodySearched;12;;;;0.5;;;3.;0.;0.\n"); // no node at all
    REQUIRE(MeshAdjacency::elementsOfNode(model, 1) == std::vector<base::EntityId>{10});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 3) == std::vector<base::EntityId>{11});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 2).empty());
}

TEST_CASE("an RBE2GS whose search grid does not exist is reported with the element", "[analysis][geometry]")
{
    const Model model = readModelText(grid(1, 0, 0, 0) + "RigidBodySearched;10;77;;;0.5\n");
    const auto location = model.locate(base::IdSpace::Element, 10);
    REQUIRE(location.has_value());
    REQUIRE_THROWS_AS(ElementGeometry::of(model, *location), GeometryError);
}
