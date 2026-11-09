#pragma once

#include "oor/analysis/components/ElementRoles.hpp"
#include "oor/analysis/connections/FastenerPiercings.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oor::analysis
{

// One thing of the model that acts on grid points: an element with a role in the structure (ElementRoles), or an MPC
// equation (one MPC card; `id` is its set).
struct ModelEntity
{
    model::CardLocation card;
    base::EntityId id = 0;
    ElementRole role = ElementRole::None;
    bool isEquation = false;
};

// Every element of the model that has a role in the structure, with the grid points it acts on, and the MPC equations
// with the grid points of their terms: what the components, the groups and the connection system are made of.
//
// The grid points of an element are those ElementTopology gives, except for a CFAST and a CWELD, which act on the grid
// points of the elements they pierce (FastenerPiercings: the element of side A and the element of side B, the face of
// grid points of a GRIDID side) and on their own GA and GB; GS only locates them, except in a point-to-patch
// connection. A fastener with no own grid point and no side found keeps the grid points of its card.
//
// The elements come in the order of the card types, then of the rows; the equations after them, in the order of the
// MPC table. Immutable once built; cached with the model (ComponentCache).
class ModelEntities
{
public:
    [[nodiscard]] static ModelEntities build(const model::Model& model, const FastenerPiercings* piercings,
                                             base::ThreadPool* pool = nullptr);

    [[nodiscard]] std::size_t size() const noexcept { return m_entities.size(); }
    [[nodiscard]] const ModelEntity& entity(std::size_t index) const noexcept { return m_entities[index]; }
    [[nodiscard]] const std::vector<ModelEntity>& entities() const noexcept { return m_entities; }
    // The grid points of an entity, in the order of its card (a grid point may come twice).
    [[nodiscard]] std::span<const base::EntityId> nodesOf(std::size_t index) const noexcept
    {
        return std::span<const base::EntityId>(m_nodes).subspan(m_start[index], m_start[index + 1] - m_start[index]);
    }
    // The position of the element at `card`, or nothing for a card that is not one of the entities.
    [[nodiscard]] std::optional<std::size_t> indexOf(model::CardLocation card) const noexcept;

private:
    std::vector<ModelEntity> m_entities;
    std::vector<std::uint32_t> m_start;
    std::vector<base::EntityId> m_nodes;
    std::vector<std::vector<std::uint32_t>> m_indexOfRow; // by card type, then row: position + 1 (0: none)
};

} // namespace oor::analysis
