// The sets a LOAD combination reaches and the loads of a set's own cards: with the factors of the
// LOAD entries on the way, the own loads of the sets reached are the expansion of the combination, to the bit (the
// cards of its own of a set that also holds a LOAD entry with the scale 1).

#include "oor/analysis/components/LoadSets.hpp"
#include "oor/model/cards/loads/gravityLoad/GravityLoadTable.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstring>
#include <string>
#include <tuple>
#include <vector>

using namespace oor;
using oor::testing::readModelText;

namespace
{

// Sets 1 (two FORCE), 2 (a PLOAD2 range) and 3 (a GRAV); LOAD 10 of sets 1 and 2, LOAD 20 of LOAD 10 and set 3
// (nested), LOAD 30 that reaches set 1 twice, LOAD 40 whose set holds a FORCE of its own, LOAD 50 and 51 a cycle.
const char* const DECK =
    "[model]\n"
    "IsoMat;1;2.E+5;;0.3\n"
    "ShellProp;1;1;1.\n"
    "Node;1;;0.;0.;0.\n"
    "Node;2;;1.;0.;0.\n"
    "Node;3;;1.;1.;0.\n"
    "Node;4;;0.;1.;0.\n"
    "ShellQuad4;1;1;1;2;3;4\n"
    "ShellQuad4;2;1;1;2;3;4\n"
    "PointForceLoad;1;1;;10.;1.;0.;0.\n"
    "PointForceLoad;1;2;;5.;0.;1.;0.\n"
    "ShellPressureLoad;2;100.;1;Through;2\n"
    "GravityLoad;3;;9.81;0.;0.;-1.\n"
    "LoadCombination;10;0.7;0.1;1;0.3333333333333333;2\n"
    "LoadCombination;20;1.1;2.2;10;-1.3;3\n"
    "LoadCombination;30;0.9;1.;10;0.6;1\n"
    "LoadCombination;40;2.;0.5;3\n"
    "PointForceLoad;40;3;;1.;0.;0.;1.\n"
    "LoadCombination;50;1.;1.;51;1.;1\n"
    "LoadCombination;51;1.;1.;50;2.;2\n";

bool sameBits(double left, double right)
{
    return std::memcmp(&left, &right, sizeof(double)) == 0;
}

} // namespace

TEST_CASE("The sets a nested LOAD reaches, with the factors of the expansion", "[analysis][loads]")
{
    const model::Model model = readModelText(DECK);
    const analysis::LoadSetIndex index(model);
    const std::vector<analysis::ReachedSet> sets = analysis::LoadSets::reachedSets(model, index, 20);
    REQUIRE(sets.size() == 3);
    CHECK(sets[0].sid == 1);
    CHECK(sets[1].sid == 2);
    CHECK(sets[2].sid == 3);
    // factor: the factors on the way without the S of LOAD 20, as loadCombinationsTable multiplies them; scale: every
    // factor, in the order of the expansion
    CHECK(sameBits(sets[0].factor, ((1.0 * 2.2) * 0.7) * 0.1));
    CHECK(sameBits(sets[0].scale, ((1.0 * 1.1) * 2.2) * 0.7 * 0.1));
    CHECK(sameBits(sets[2].factor, -1.3));
    CHECK(sameBits(sets[2].scale, (1.0 * 1.1) * -1.3));
}

TEST_CASE("The own loads of the sets reached, scaled, are the expansion", "[analysis][loads]")
{
    const model::Model model = readModelText(DECK);
    const analysis::LoadSetIndex index(model);
    for (const base::EntityId sid : {10, 20, 30, 40})
    {
        const std::vector<analysis::ElementaryLoad> expanded = analysis::LoadSets::expand(model, index, sid);
        // a set with cards of its own besides its LOAD entry (40) brings them with the scale 1
        std::vector<analysis::ElementaryLoad> made = analysis::LoadSets::ownLoads(model, index, sid);
        for (const analysis::ReachedSet& set : analysis::LoadSets::reachedSets(model, index, sid))
        {
            for (analysis::ElementaryLoad load : analysis::LoadSets::ownLoads(model, index, set.sid))
            {
                CHECK(load.scale == 1.0);
                CHECK(load.sourceSid == set.sid);
                load.scale = set.scale;
                made.push_back(load);
            }
        }
        REQUIRE(made.size() == expanded.size());
        // the same cards with the same scales; the order differs from the expansion's only where a set holds cards of
        // its own besides a LOAD entry (LOAD 40)
        const auto key = [](const analysis::ElementaryLoad& load)
        { return std::tuple(load.card.table, load.card.row, load.sourceSid, load.scale); };
        std::vector<std::tuple<std::uint16_t, std::uint32_t, base::EntityId, double>> left;
        std::vector<std::tuple<std::uint16_t, std::uint32_t, base::EntityId, double>> right;
        for (const analysis::ElementaryLoad& load : made)
            left.push_back(key(load));
        for (const analysis::ElementaryLoad& load : expanded)
            right.push_back(key(load));
        std::sort(left.begin(), left.end());
        std::sort(right.begin(), right.end());
        CHECK(left == right);
    }
}

TEST_CASE("A set reached twice is listed twice; the own cards of a combination's set are not reached",
          "[analysis][loads]")
{
    const model::Model model = readModelText(DECK);
    const analysis::LoadSetIndex index(model);
    const std::vector<analysis::ReachedSet> twice = analysis::LoadSets::reachedSets(model, index, 30);
    REQUIRE(twice.size() == 3);
    CHECK(twice[0].sid == 1);
    CHECK(twice[1].sid == 2);
    CHECK(twice[2].sid == 1);
    const std::vector<analysis::ReachedSet> own = analysis::LoadSets::reachedSets(model, index, 40);
    REQUIRE(own.size() == 1);
    CHECK(own[0].sid == 3);
    CHECK(sameBits(own[0].factor, 1.0 * 0.5));
    CHECK(sameBits(own[0].scale, (1.0 * 2.0) * 0.5));
    CHECK(analysis::LoadSets::ownLoads(model, index, 40).size() == 1); // its FORCE
}

TEST_CASE("A cycle of LOAD entries stops where it closes; a set of its own reaches itself", "[analysis][loads]")
{
    const model::Model model = readModelText(DECK);
    const analysis::LoadSetIndex index(model);
    const std::vector<analysis::ReachedSet> cycle = analysis::LoadSets::reachedSets(model, index, 50);
    REQUIRE(cycle.size() == 2);
    CHECK(cycle[0].sid == 2); // through LOAD 51
    CHECK(cycle[1].sid == 1);
    const std::vector<analysis::ReachedSet> alone = analysis::LoadSets::reachedSets(model, index, 1);
    REQUIRE(alone.size() == 1);
    CHECK(alone[0].sid == 1);
    CHECK(analysis::LoadSets::reachedSets(model, index, 99).empty()); // no card
    // the own loads of a set, filtered by card type
    const std::vector<std::uint16_t> types{model::typeIndexOf<model::GravityLoadTable>()};
    CHECK(analysis::LoadSets::ownLoads(model, index, 1, types).empty());
    CHECK(analysis::LoadSets::ownLoads(model, index, 3, types).size() == 1);
    CHECK(analysis::LoadSets::ownLoads(model, index, 10).empty()); // a combination has no card of its own
}
