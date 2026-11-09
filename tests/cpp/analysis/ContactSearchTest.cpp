// Contact between bodies in the components: the bodies, the pairs, the node pairs found by the search and what the
// three contact modes make of them, on two hexahedra worked out by hand. The Python tests
// (tests/python/test_contact_components.py) cover the queries and the tables.

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/RevisionStamp.hpp"
#include "oor/analysis/connections/ContactSearch.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace oor;
using oor::testing::readModelText;

namespace
{

// Two unit cubes, one CHEXA each: element 1 on nodes 1 to 8 (z from 0 to 1), element 2 on nodes 11 to 18 (z from
// `bottom` to `bottom` + 1), bodies 1 and 2 over BSURF 1 and 2, and a BCONECT 5 pairing body 2 (secondary) with body 1.
// `pair` and `body2` are the records of the BCONECT and of the second BCBODY1, for the tests that change them.
std::string stackedCubes(double bottom, const std::string& extra = "",
                         const std::string& pair = "ContactPair;5;;;2;1\n",
                         const std::string& body2 = "ContactBody1;2;;ThreeDimensional;Deformable;2\n")
{
    std::string text = "IsoMat;1;2.E+5;7.5E+4;0.3\nSolidProp;1;1\n";
    const double corners[4][2] = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}};
    for (const auto& [first, z0] : {std::pair{1, 0.0}, std::pair{11, bottom}})
    {
        for (int layer = 0; layer < 2; ++layer)
        {
            for (int corner = 0; corner < 4; ++corner)
                text += testing::modelRecord("Node", first + 4 * layer + corner, "", corners[corner][0],
                                             corners[corner][1], z0 + layer);
        }
        text += testing::modelRecord("SolidHexa", first == 1 ? 1 : 2, 1, first, first + 1, first + 2, first + 3,
                                     first + 4, first + 5, first + 6, first + 7);
    }
    return text + "ContactElemSurface;1;1\nContactElemSurface;2;2\nContactBody1;1;;ThreeDimensional;Deformable;1\n" +
           body2 + pair + extra;
}

} // namespace

TEST_CASE("the bottom grid points of a cube resting on another meet its top face", "[analysis][contact]")
{
    const model::Model model = readModelText(stackedCubes(1.0));
    const auto search = analysis::ContactSearch::compute(model, std::nullopt);
    REQUIRE(search->pairs().size() == 1);
    const analysis::ContactPair& pair = search->pairs().front();
    CHECK(pair.tableLabel() == "ContactPair 5");
    CHECK(pair.secondary->id == 2);
    CHECK(pair.primary->id == 1);
    CHECK(pair.kind == analysis::ContactKind::Touching);
    const analysis::ContactPairResult& result = search->results().front();
    // 5 % of the median edge of the secondary surface, the six unit faces of a cube
    CHECK(result.tolerance == Catch::Approx(0.05));
    REQUIRE(result.meets);
    std::vector<base::EntityId> nids;
    for (const analysis::ContactMeeting& meeting : result.meetings)
    {
        nids.push_back(meeting.nid);
        CHECK(meeting.distance == Catch::Approx(0.0).margin(1e-12));
        CHECK(meeting.face->eid == 1);
        CHECK(meeting.point.z == Catch::Approx(1.0));
    }
    CHECK(nids == std::vector<base::EntityId>{11, 12, 13, 14}); // the bottom of cube 2; its top is 1 away
}

TEST_CASE("the contact modes merge, connect or ignore the cubes", "[analysis][contact]")
{
    const model::Model model = readModelText(stackedCubes(1.0));
    const auto merged = analysis::ComponentModel::of(model);
    CHECK(merged->componentCount() == 1);
    CHECK(merged->contactJoins().size() == 1);
    CHECK(merged->contactJoinsOf(1) == std::vector<std::uint32_t>{0});

    const auto connected =
        analysis::ComponentModel::of(model, analysis::ContactOptions{analysis::ContactMode::Connection, std::nullopt});
    REQUIRE(connected->componentCount() == 2);
    REQUIRE(connected->connections().size() == 1);
    CHECK(connected->connections().front().kind == analysis::ConnectionKind::Contact);
    CHECK(connected->connections().front().id == 5);
    // the four grid points that meet (on component 2) and the corners of the top face of cube 1 (component 1)
    std::vector<base::EntityId> lower;
    std::vector<base::EntityId> upper;
    for (const analysis::InterfaceNode& node : connected->interfaceNodes(connected->componentOfNode(1)))
        lower.push_back(node.node);
    for (const analysis::InterfaceNode& node : connected->interfaceNodes(connected->componentOfNode(11)))
        upper.push_back(node.node);
    CHECK(lower == std::vector<base::EntityId>{5, 6, 7, 8});
    CHECK(upper == std::vector<base::EntityId>{11, 12, 13, 14});

    const auto ignored =
        analysis::ComponentModel::of(model, analysis::ContactOptions{analysis::ContactMode::Ignore, std::nullopt});
    CHECK(ignored->componentCount() == 2);
    CHECK(ignored->connections().empty());
}

TEST_CASE("a pair whose surfaces do not meet is kept apart, and a larger ERROR joins it", "[analysis][contact]")
{
    const model::Model apart = readModelText(stackedCubes(1.2));
    const auto search = analysis::ContactSearch::compute(apart, std::nullopt);
    REQUIRE(search->pairs().size() == 1);
    CHECK_FALSE(search->results().front().meets);
    CHECK(analysis::ComponentModel::of(apart)->componentCount() == 2);

    // format reference BCONPRG: ERROR is the distance below which a node touches a body
    const model::Model reached = readModelText(stackedCubes(
        1.2, "ContactPairGeomParams;7;ContactDistanceTolerance;0.25;GlueOption;1\n", "ContactPair;5;7;;2;1\n"));
    const auto joined = analysis::ContactSearch::compute(reached, std::nullopt);
    CHECK(joined->results().front().tolerance == Catch::Approx(0.25));
    CHECK(joined->results().front().meets);
    CHECK(joined->pairs().front().kind == analysis::ContactKind::Glued);
    CHECK(analysis::ComponentModel::of(reached)->componentCount() == 1);
}

TEST_CASE("the the solver rule ties each grid point once, from the lower body first", "[analysis][contact]")
{
    // the cubes touch; ERROR blank is the smallest edge over 20 (1 / 20), the BIAS of a pair without glue
    // 0.9 (format reference BCPARA): the zone is -1.9 x 0.05 to 0.1 x 0.05
    const model::Model model = readModelText(stackedCubes(1.0));
    const auto search = analysis::ContactSearch::compute(model, std::nullopt, analysis::ContactRule::Solver);
    REQUIRE(search->results().size() == 1);
    const analysis::ContactPairResult& result = search->results().front();
    CHECK(result.tolerance == Catch::Approx(0.05));
    // ISEARCH 0: body 1 (the primary body, the lower ID) first: the top corners of cube 1 on the bottom face of cube 2;
    // then body 2, whose bottom corners would be tied to the face whose corners are already tied: left out
    REQUIRE(result.meetings.size() == 4);
    for (const analysis::ContactMeeting& meeting : result.meetings)
    {
        CHECK(meeting.searchedFrom == analysis::SearchSide::Primary);
        CHECK(meeting.secondary->id == 1);
        CHECK(meeting.face->eid == 2);
        CHECK(meeting.gap == Catch::Approx(0.0).margin(1e-12));
    }
    CHECK(result.meetings.front().nid == 5);
    // the local coordinates of a corner of the face: the corners of a quadrilateral are at (+-1, +-1)
    CHECK(std::abs(result.meetings.front().coordinates.xi) == Catch::Approx(1.0));
    CHECK(std::abs(result.meetings.front().coordinates.eta) == Catch::Approx(1.0));
}

TEST_CASE("the digit B of COPTS and COPTM says which skins of a shell take part", "[analysis][contact]")
{
    using Side = analysis::ShellSkins::Side;
    const auto skins = [](std::int32_t code)
    {
        const analysis::ShellSkins found = analysis::ShellSkins::fromContactOption(code);
        return std::pair{found.withThickness, found.side};
    };
    // COPxxx = A + 10 B + 1000 C
    CHECK(skins(0) == std::pair{true, Side::Both});
    CHECK(skins(1011) == std::pair{true, Side::Both});
    CHECK(skins(1021) == std::pair{true, Side::Bottom});
    CHECK(skins(31) == std::pair{false, Side::Bottom});
    CHECK(skins(41) == std::pair{true, Side::Top});
    CHECK(skins(10051) == std::pair{false, Side::Top});
    CHECK(skins(61) == std::pair{false, Side::Both});
}

TEST_CASE("the contact cards are in the stamp of the component they merge", "[analysis][contact]")
{
    const model::Model first =
        readModelText(stackedCubes(1.0, "ContactPairGeomParams;7;ContactDistanceTolerance;0.01\n"));
    const model::Model second =
        readModelText(stackedCubes(1.0, "ContactPairGeomParams;7;ContactDistanceTolerance;0.02\n"));
    // the BCONPRG is not named by the pair: it is no contact card of the component
    CHECK(*analysis::RevisionStamp::of(first) == *analysis::RevisionStamp::of(second));
    const model::Model named = readModelText(stackedCubes(1.0, "ContactBodyParams;3\n"));
    const model::Model renamed = readModelText(stackedCubes(1.0, "ContactBodyParams;3\n", "ContactPair;5;;;2;1\n",
                                                            "ContactBody1;2;3;ThreeDimensional;Deformable;2\n"));
    CHECK(*analysis::RevisionStamp::of(named) != *analysis::RevisionStamp::of(renamed));
    const analysis::ContactOptions ignore{analysis::ContactMode::Ignore, std::nullopt};
    CHECK(*analysis::RevisionStamp::of(named, ignore) == *analysis::RevisionStamp::of(renamed, ignore));
}
