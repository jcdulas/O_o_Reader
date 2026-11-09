#pragma once

#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/providers/quantities/CutConnections.hpp"
#include "oor/query/providers/quantities/NodePointForceSet.hpp"
#include "oor/query/providers/quantities/Resultants.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace oor::query
{

// The resultant of a cut at every step of one grid point force table.
struct CutTableSum
{
    bool hasRows = false;          // the side has a row at a cut grid point, or connections cross the cut
    std::int64_t nodeCount = 0;    // the cut grid points where an element of the side has a row
    std::int64_t elementCount = 0; // the elements of the side with a row at a cut grid point
    bool complete = true;
    std::vector<std::string> uncountedCards; // what links the two sides and is not counted, with its ID
    std::vector<std::int32_t> uncountedIds;
    std::vector<std::array<double, 6>> real; // a step each: fx fy fz mx my mz in the basic system
    std::vector<std::array<double, 6>> imaginary;
    std::vector<std::vector<CutNodeForce>> nodes; // a step each, when asked
};

// Sums the grid point forces across a cut (equilibrium and cuts): the rows of the side elements at the
// cut grid points, then, for each counted cut connection, minus the multiPointConstrForce rows of its grid points on
// the side (the force the side gives the connection, which carries it across). At equilibrium the sum is the applied
// load and the reactions of the interior of the side (appliedLoad and singlePointConstrForce there): the element rows
// of the side add up to zero over their grid points, and at an interior grid point they balance its applied load,
// constraint forces and multiPointConstrForce.
//
// What links the two sides and is not counted is named, and the sum is then not complete: a connection whose
// multiPointConstrForce rows cannot be split, an element of the side with no grid point force (a type GPFORCE does not
// cover), a grid point of the cut with no row (GPFORCE asked for a set that leaves it out).
class CutForceSum
{
public:
    // The rows a cut sums in the tables of one layout (and one kind of subcase, static or not): the same for every
    // subcase of a result set, found once. Each term is a grid point of the cut with the rows of the side elements
    // there (added), or a grid point of a cut connection with a multiPointConstrForce row (taken with the opposite
    // sign), in the order of the sum.
    struct Term
    {
        std::int32_t node = 0;
        base::Vec3 at;
        std::uint32_t first = 0; // the rows of the term in `rows`
        std::uint32_t count = 0;
        const char* contribution = "element"; // "element", "mpc" or "rigid"
        bool negated = false;                 // a multiPointConstrForce row
    };
    struct Plan
    {
        std::int64_t nodeCount = 0;
        std::int64_t elementCount = 0;
        bool complete = true;
        bool hasRows = false;
        std::vector<std::string> uncountedCards;
        std::vector<std::int32_t> uncountedIds;
        std::vector<Term> terms;
        std::vector<std::uint32_t> rows;
    };

    // The sum about `point` (basic system). `connections` are the cut connections of the MPC selection of the table.
    [[nodiscard]] static CutTableSum of(const NodePointForceTable& table, const CutDefinition& cut,
                                        const std::vector<CutConnection>& connections,
                                        const std::array<double, 3>& point, const NodePositions& positions,
                                        bool withNodes);
    // The plan of a cut for the tables with the layout and the kind of subcase of `table`.
    [[nodiscard]] static Plan plan(const NodePointForceTable& table, const CutDefinition& cut,
                                   const std::vector<CutConnection>& connections, const NodePositions& positions);
    // The sum of a table by a plan made for its layout: the values `of` gives, to the last bit. `sum` is overwritten
    // (its storage is kept: one sum serves the tables of a cut).
    static void apply(const Plan& plan, const NodePointForceTable& table, const std::array<double, 3>& point,
                      bool withNodes, CutTableSum& sum);
};

} // namespace oor::query
