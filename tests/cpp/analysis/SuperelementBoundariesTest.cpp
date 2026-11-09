// The boundaries of the superelements, on hand-built decks whose answer is worked out by hand: the automatic
// search after a SELOC and an SEMPLN, a point that only two parts share (the solver creates its residual point), the
// solver's numbering of the boundary points, the tolerance rule, and the cell index the search uses.

#include "oor/analysis/geometry/PointCellIndex.hpp"
#include "oor/analysis/superelements/PartBoundaries.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::analysis;
using oor::testing::readModelText;

namespace
{

// Part 10 meets the residual structure at 1 and 2, and part 20 at its point 103 (no residual point there). Part 30 is
// moved by its SELOC onto 11, 12, 13. Part 40 is the mirror of part 30 through the plane z = 0 of 1, 2 and 5: its
// points 301 and 302 fall on 1 and 2 (and on part 10), its 303 on 5.
const char* const PARTS =
    "[executive]\n"
    "SolutionSequence;101\n"
    "[model]\n"
    "Node;1;;0.;0.;0.\n"
    "Node;2;;1.;0.;0.\n"
    "Node;5;;0.;1.;0.\n"
    "Node;11;;10.;0.;0.\n"
    "Node;12;;10.;1.;0.\n"
    "Node;13;;9.;0.;0.\n"
    "SuperelementLocation;30;301;302;303;11;12;13\n"
    "SuperelementBulkOptions;40;Mirror;30\n"
    "SuperelementMirrorPlane;40;Plane;1;2;5\n"
    "[model;SuperelementPart;10]\n"
    "Node;101;;0.;0.;0.\n"
    "Node;102;;1.;0.;0.\n"
    "Node;103;;2.;0.;0.\n"
    "Node;104;;5.;5.;5.\n"
    "[model;SuperelementPart;20]\n"
    "Node;201;;2.;0.;0.\n"
    "Node;202;;3.;0.;0.\n"
    "Node;203;;1.;0.;2.E-5\n"
    "[model;SuperelementPart;30]\n"
    "Node;301;;0.;0.;0.\n"
    "Node;302;;1.;0.;0.\n"
    "Node;303;;0.;1.;0.\n"
    "Node;304;;0.;0.;1.\n";

struct Expected
{
    base::EntityId seid;
    base::EntityId nid;
    base::EntityId downstreamNid; // 0: created by the solver
    std::int32_t boundaryPoint;
};

void requireRows(const PartBoundaries& boundaries, const std::vector<Expected>& expected)
{
    const std::vector<PartBoundaryRow>& rows = boundaries.rows();
    REQUIRE(rows.size() == expected.size());
    for (std::size_t index = 0; index < rows.size(); ++index)
    {
        INFO("row " << index);
        REQUIRE(rows[index].seid == expected[index].seid);
        REQUIRE(rows[index].nid == expected[index].nid);
        REQUIRE(rows[index].downstreamNid.value_or(0) == expected[index].downstreamNid);
        REQUIRE(rows[index].boundaryPoint == expected[index].boundaryPoint);
    }
}

} // namespace

TEST_CASE("the automatic search joins the placed points of the parts and numbers them as the solver does",
          "[analysis][superelements]")
{
    const model::Model model = readModelText(PARTS, true);
    const PartBoundaries boundaries(model);
    REQUIRE(boundaries.problems().empty());
    // Primaries 10, 20, 30 by number, then the image 40; inside each, its points by ID.
    requireRows(boundaries, {{10, 101, 1, 1},
                             {10, 102, 2, 2},
                             {10, 103, 0, 3},
                             {20, 201, 0, 3},
                             {30, 301, 11, 4},
                             {30, 302, 12, 5},
                             {30, 303, 13, 6},
                             {40, 301, 1, 1},
                             {40, 302, 2, 2},
                             {40, 303, 5, 7}});
    for (const PartBoundaryRow& row : boundaries.rows())
    {
        REQUIRE(row.source == BoundarySource::Automatic);
        REQUIRE(row.dof == "123456");
        REQUIRE(row.distance < 1.0e-12);
    }
    const SuperelementEntry* image = boundaries.catalogue().find(40);
    REQUIRE((image != nullptr && image->type == SuperelementType::Mirror && image->imageOf == 30));
}

TEST_CASE("the tolerance of the search is the SEBULK TOL, else CONFAC", "[analysis][superelements]")
{
    // Point 203 of part 20 is 2.0E-5 from residual 2: outside the default CONFAC of 1.0E-5, inside a TOL of 1.0E-4.
    std::string deck = PARTS;
    const auto joined = [](const PartBoundaries& boundaries)
    {
        for (const PartBoundaryRow& row : boundaries.rows())
        {
            if (row.seid == 20 && row.nid == 203)
                return std::pair{true, row.distance};
        }
        return std::pair{false, 0.0};
    };
    REQUIRE_FALSE(joined(PartBoundaries(readModelText(deck, true))).first);
    deck.insert(deck.find("[model;SuperelementPart;10]"), "SuperelementBulkOptions;20;Primary;;Automatic;1.E-4\n");
    const auto [isJoined, distance] = joined(PartBoundaries(readModelText(deck, true)));
    REQUIRE(isJoined);
    REQUIRE(std::abs(distance - 2.0e-5) < 1.0e-12);
}

TEST_CASE("the cell index finds every pair within the distance and no other", "[analysis][superelements]")
{
    const std::vector<base::Vec3> points{
        {0.0, 0.0, 0.0}, {0.9, 0.0, 0.0}, {1.1, 0.0, 0.0}, {5.0, 5.0, 5.0}, {-0.5, 0.0, 0.0}};
    const PointCellIndex index(points, 1.0);
    std::vector<std::pair<std::uint32_t, std::uint32_t>> pairs;
    index.forEachPairWithin(1.0, [&](std::uint32_t first, std::uint32_t second) { pairs.emplace_back(first, second); });
    REQUIRE(pairs == std::vector<std::pair<std::uint32_t, std::uint32_t>>{{0, 1}, {0, 4}, {1, 2}});
}
