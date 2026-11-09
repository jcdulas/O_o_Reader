// The geometry of the user dummy elements CDUM1 to CDUM9: their grid points are known, nothing
// else is, because the user's own routines define the element. Each number has a provider of its own, so each test runs
// over the nine tables.

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/adjacency/MeshAdjacency.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/elements/userDefined1/UserDefined1Table.hpp"
#include "oor/model/cards/elements/userDefined2/UserDefined2Table.hpp"
#include "oor/model/cards/elements/userDefined3/UserDefined3Table.hpp"
#include "oor/model/cards/elements/userDefined4/UserDefined4Table.hpp"
#include "oor/model/cards/elements/userDefined5/UserDefined5Table.hpp"
#include "oor/model/cards/elements/userDefined6/UserDefined6Table.hpp"
#include "oor/model/cards/elements/userDefined7/UserDefined7Table.hpp"
#include "oor/model/cards/elements/userDefined8/UserDefined8Table.hpp"
#include "oor/model/cards/elements/userDefined9/UserDefined9Table.hpp"
#include "support/ModelTestSupport.hpp"
#include "support/SlotRecord.hpp"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace oor;
using namespace oor::analysis;
using namespace oor::model;
using oor::testing::readModelText;

namespace
{

using CdumTables = std::tuple<UserDefined1Table, UserDefined2Table, UserDefined3Table, UserDefined4Table, UserDefined5Table,
                              UserDefined6Table, UserDefined7Table, UserDefined8Table, UserDefined9Table>;
using Ids = std::vector<base::EntityId>;

// Five nodes, the ones the connection of a common model names.
const std::string NODES =
    "Node;2;;0.;0.;0.\nNode;5;;1.;0.;0.\nNode;6;;1.;1.;0.\nNode;8;;0.;1.;0.\nNode;11;;0.5;0.5;1.\n";

// The user element of a common model: five nodes in the six places of the first line of the old layout (slots 3 to
// 8), the additional fields after them (slots 9 to 14).
std::string connection(std::string_view name, int eid)
{
    return oor::testing::slotRecord(
        name, {std::to_string(eid), "108", "2", "5", "6", "8", "11", "", "2.4", "", "3.E4", "2", "", "50"});
}

} // namespace

TEMPLATE_LIST_TEST_CASE("a CDUMi is attached at its grid points and has no geometry of its own", "[analysis][geometry]",
                        CdumTables)
{
    const std::string_view name = TestType::NAME;
    const Model model = readModelText(NODES + connection(name, 114));
    const std::uint16_t type = typeIndexOf<TestType>();
    REQUIRE(ElementGeometry::supports(type));
    // The element is a point of the connectivity (it is not a line, a shell or a solid).
    REQUIRE(ElementTopology::shapeOf(type) == ElementShape::Point);
    const auto location = model.locate(base::IdSpace::Element, 114);
    REQUIRE(location.has_value());

    // The nodes are the leading run of integers of the card, in the order of the card; the additional fields after the
    // blank sixth place are not nodes.
    Ids nodes;
    ElementTopology::nodesOf(model, *location, nodes);
    REQUIRE(nodes == Ids{2, 5, 6, 8, 11});

    // Every quantity is undefined: the shape of a user element is not known.
    const ElementValues values = ElementGeometry::of(model, *location);
    REQUIRE_FALSE(isDefined(values.center));
    REQUIRE_FALSE(isDefined(values.normal));
    REQUIRE(std::isnan(values.area));
    REQUIRE(std::isnan(values.volume));
    REQUIRE(std::isnan(values.length));
    REQUIRE(std::isnan(values.thickness));
    REQUIRE(std::isnan(values.frame.values[0]));
    REQUIRE(std::isnan(values.materialFrame.values[0]));
}

TEMPLATE_LIST_TEST_CASE("a CDUMi is among the elements of each of its grid points", "[analysis][geometry]", CdumTables)
{
    const std::string_view name = TestType::NAME;
    const Model model =
        readModelText(NODES + connection(name, 114) + oor::testing::slotRecord(name, {"115", "108", "5", "6", "8"}));
    Ids atFive = MeshAdjacency::elementsOfNode(model, 5);
    std::sort(atFive.begin(), atFive.end());
    REQUIRE(atFive == Ids{114, 115});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 2) == Ids{114});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 11) == Ids{114});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 99).empty());
}

TEMPLATE_LIST_TEST_CASE("an integer additional field that follows the last grid point is taken for a grid point",
                        "[analysis][geometry]", CdumTables)
{
    // The card cannot tell the flag 7 from a seventh grid point: the NG of the ADUMi card does (see UserDefined1Card).
    const std::string_view name = TestType::NAME;
    const Model model = readModelText(NODES + oor::testing::slotRecord(name, {"114", "108", "2", "5", "7"}));
    const auto location = model.locate(base::IdSpace::Element, 114);
    REQUIRE(location.has_value());
    Ids nodes;
    ElementTopology::nodesOf(model, *location, nodes);
    REQUIRE(nodes == Ids{2, 5, 7});
}
