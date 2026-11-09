#pragma once

#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/query/named/common/NodePositions.hpp"

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace oor::query
{

struct CutDefinition;

// A rigid element or an MPC equation that links the side of a cut to the rest (load paths through rigid
// elements and MPC equations): its grid points on the side, and whether the multiPointConstrForce rows of those grid
// points belong to it alone.
struct CutConnection
{
    std::string card;                    // RBE2, RBE3, RBAR, ..., or MPC
    std::int32_t id = 0;                 // the element ID; the set ID of an MPC equation
    bool isEquation = false;             // an MPC equation (contribution "mpc"; "rigid" otherwise)
    std::vector<std::int32_t> sideNodes; // its grid points on the side, sorted, once each
    bool countable = false;              // no other rigid element or MPC equation acts at any of them
    bool withinCut = false;              // every grid point of it is on the cut: never counted
    // At each grid point of `sideNodes`: no other rigid element or MPC equation acts there (the lumped
    // multiPointConstrForce row of the grid point is its own; `countable` is all of them).
    std::vector<std::uint8_t> aloneAt;
    // At each grid point of `sideNodes`: its multiPointConstrForce row split by relation is its own: always for a
    // rigid element; for an MPC equation, when every equation of its set that acts there crosses the cut with it there
    // (the split row of a set lumps its equations at a grid point, and is then counted once for all of them).
    std::vector<std::uint8_t> ownRowAt;

    // The ID its split multiPointConstrForce rows carry in elementIds: the element ID, minus the set ID of an MPC
    // equation.
    [[nodiscard]] std::int32_t relation() const noexcept { return isEquation ? -id : id; }
};

// The rigid elements and the MPC equations of one MPC selection, and the cut connections of each cut. The grid point
// forces of the solver give the load of a rigid element or an MPC equation only as the multiPointConstrForce row of
// each of its grid points, the rows of every connection at the point lumped. So a connection whose grid points are on
// the side and off it carries load across the cut that the elements of the side do not show; its force on the side is
// the multiPointConstrForce rows of its grid points on the side when no other connection acts there (then the row is
// its own), else the rows cannot be split and the connection is named as not counted. A table whose
// multiPointConstrForce rows are split by relation (a row per relation and grid point, elementIds the relation: a
// companion package) gives each connection its own rows wherever others act (CutForceSum).
//
// The side is the interior of the cut (CutDefinition::interiorNodes: the grid points of the side elements that are not
// on the cut); for a plane cut, a grid point that no element uses (the centre of a spider) is on the side when it is
// behind the plane.
class CutConnections
{
public:
    // The rigid elements of the model and the MPC equations of `mpcSets` (the sets the subcase selects, MPCADD
    // expanded); every set of the model when it is nothing (the subcase is not in the Case Control of the model).
    CutConnections(const model::Model& model, const std::optional<std::set<base::EntityId>>& mpcSets,
                   base::ThreadPool* pool);

    // The connections that link the side of `cut` to the rest.
    [[nodiscard]] std::vector<CutConnection> crossing(const CutDefinition& cut) const;

    // The MPC sets a subcase selects (the MPC command of the subcase, MPCADD expanded; empty when it selects none);
    // nothing when the Case Control of the model has no such subcase.
    [[nodiscard]] static std::optional<std::set<base::EntityId>> mpcSetsOf(const model::Model& model,
                                                                           std::int32_t subcase);

private:
    struct Active
    {
        std::string card;
        std::int32_t id = 0;
        bool isEquation = false;
        std::vector<std::int32_t> nodes; // sorted, once each
    };

    NodePositions m_positions;
    std::vector<Active> m_active;
    std::unordered_map<std::int32_t, std::int32_t> m_actingAt; // how many active connections act at a grid point
    // how many active MPC equations of a set act at a grid point, by (grid point, set) (`key`)
    std::unordered_map<std::uint64_t, std::int32_t> m_equationsAt;

    [[nodiscard]] static std::uint64_t key(std::int32_t node, std::int32_t set) noexcept
    {
        return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(node)) << 32U) | static_cast<std::uint32_t>(set);
    }
};

} // namespace oor::query
