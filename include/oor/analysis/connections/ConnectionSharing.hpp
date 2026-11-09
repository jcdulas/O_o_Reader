#pragma once

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace oor::analysis
{

// What a connection is, as the sharing of grid points tells them apart (the `connection_sharing` query).
enum class ConnectionClass : std::uint8_t
{
    Rigid,              // a rigid element other than a RigidInterpolation
    RigidInterpolation, // a RigidInterpolation (its class name is "rigidInterpolation")
    Mpc,                // an MPC equation
    Fastener,           // a CFAST or a CWELD, through the grid points of the shells it pierces
    Connector,          // any other connection element: springs, dampers, bushes, gaps, a 1D element or a mass between
                        // components
    Contact             // a pair of contact bodies, in the "connection" mode of contact
};

[[nodiscard]] std::string_view connectionClassName(ConnectionClass kind) noexcept;

// The connections that touch each grid point: the connections of the
// components (ComponentModel::connections), each on the grid points it acts on (a CFAST or a CWELD on the grid points
// of the shells it pierces and its own GA and GB). Built over the connections in one pass; immutable.
class ConnectionSharing
{
public:
    [[nodiscard]] static ConnectionSharing build(const ComponentModel& components);

    // The grid points some connection touches, sorted.
    [[nodiscard]] const std::vector<base::EntityId>& nodes() const noexcept { return m_nodes; }
    // The connections that touch a node: indexes into ComponentModel::connections(), increasing, each once.
    [[nodiscard]] std::span<const std::uint32_t> connectionsAt(base::EntityId node) const noexcept;
    [[nodiscard]] ConnectionClass classOf(std::size_t connection) const noexcept { return m_classes[connection]; }
    // The number of an MPC equation among the MPC cards of its set (from 1, in the order of the table, as `mpc_table`
    // numbers them for that set); 0 for any other connection.
    [[nodiscard]] std::int32_t equationOf(std::size_t connection) const noexcept { return m_equations[connection]; }

    // The connections other than `self` that touch one of `nodes`, increasing; `blockingNodes` (optional) gets the
    // nodes they touch, sorted. What decides whether a spider or a fastener can be absorbed into a unit.
    [[nodiscard]] std::vector<std::uint32_t> othersAt(std::span<const base::EntityId> nodes, std::uint32_t self,
                                                      std::vector<base::EntityId>* blockingNodes = nullptr) const;

private:
    std::vector<base::EntityId> m_nodes;
    std::vector<std::uint32_t> m_start;
    std::vector<std::uint32_t> m_connections;
    std::vector<ConnectionClass> m_classes;
    std::vector<std::int32_t> m_equations;
};

} // namespace oor::analysis
