#include "ShellElements.hpp"

#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingTable.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"

namespace oor::analysis::detail
{

namespace
{

// Copies the PID and the node slots of a shell card.
template <typename TableT>
void readCard(const model::Model& model, std::uint32_t row, ShellElement& element)
{
    const auto* table = model.findTable<TableT>();
    const auto card = table->card(row);
    element.eid = card.id;
    element.pid = card.propId;
    for (std::size_t slot = 0; slot < card.nodes.size() && slot < MAX_SHELL_NODES; ++slot)
        element.nodes[slot] = card.nodes[slot];
}

} // namespace

base::Vec3 ShellElement::pointAt(const ShapeValues& values) const noexcept
{
    base::Vec3 point;
    for (std::size_t slot = 0; slot < slotCount(); ++slot)
        point += values.n[slot] * points[slot];
    return point;
}

ShellElements::ShellElements(const model::Model& model, const NodePositions& positions)
    : m_model(model), m_positions(positions)
{
}

bool ShellElements::isShellType(std::uint16_t type) noexcept
{
    return type == model::typeIndexOf<model::ShellQuad4Table>() ||
           type == model::typeIndexOf<model::ShellQuad4DrillingTable>() ||
           type == model::typeIndexOf<model::ShellTria3Table>() ||
           type == model::typeIndexOf<model::ShellTria3DrillingTable>() ||
           type == model::typeIndexOf<model::ShellQuad8Table>() || type == model::typeIndexOf<model::ShellTria6Table>();
}

bool ShellElements::place(ShellElement& element) const
{
    element.presentNodes = 0;
    const std::size_t corners = ShellShape::cornerCount(element.kind);
    for (std::size_t slot = 0; slot < element.slotCount(); ++slot)
    {
        if (element.nodes[slot] == 0)
        {
            if (slot < corners)
                return false; // a corner is required
            continue;
        }
        const auto position = m_positions.of(element.nodes[slot]);
        if (!position)
            return false;
        element.points[slot] = *position;
        element.presentNodes = static_cast<std::uint8_t>(element.presentNodes | (1U << slot));
    }
    return true;
}

std::optional<ShellElement> ShellElements::read(model::CardLocation location) const
{
    const model::CardTable* table = m_model.findTable(location.table);
    if (table == nullptr || location.row >= table->rows().rowCount() || !table->rows().isAlive(location.row))
        return std::nullopt;
    ShellElement element;
    element.location = location;
    const std::uint16_t type = location.table;
    if (type == model::typeIndexOf<model::ShellQuad4Table>())
    {
        element.kind = ShellShapeKind::Quad4;
        readCard<model::ShellQuad4Table>(m_model, location.row, element);
    }
    else if (type == model::typeIndexOf<model::ShellQuad4DrillingTable>())
    {
        element.kind = ShellShapeKind::Quad4;
        readCard<model::ShellQuad4DrillingTable>(m_model, location.row, element);
    }
    else if (type == model::typeIndexOf<model::ShellTria3Table>())
    {
        element.kind = ShellShapeKind::Tria3;
        readCard<model::ShellTria3Table>(m_model, location.row, element);
    }
    else if (type == model::typeIndexOf<model::ShellTria3DrillingTable>())
    {
        element.kind = ShellShapeKind::Tria3;
        readCard<model::ShellTria3DrillingTable>(m_model, location.row, element);
    }
    else if (type == model::typeIndexOf<model::ShellQuad8Table>())
    {
        element.kind = ShellShapeKind::Quad8;
        readCard<model::ShellQuad8Table>(m_model, location.row, element);
    }
    else if (type == model::typeIndexOf<model::ShellTria6Table>())
    {
        element.kind = ShellShapeKind::Tria6;
        readCard<model::ShellTria6Table>(m_model, location.row, element);
    }
    else
        return std::nullopt;
    if (!place(element))
        return std::nullopt;
    return element;
}

std::optional<ShellElement> ShellElements::readById(base::EntityId eid) const
{
    if (eid == 0)
        return std::nullopt;
    const auto location = m_model.locate(base::IdSpace::Element, eid);
    if (!location)
        return std::nullopt;
    return read(*location);
}

std::optional<ShellElement> ShellElements::fromGrids(std::span<const base::EntityId> grids, char shape) const
{
    ShellElement element;
    element.isVirtual = true;
    const bool quadrilateral = shape == 'Q';
    const std::size_t corners = quadrilateral ? 4 : 3;
    const std::size_t slots = quadrilateral ? 8 : 6;
    bool anyMidSide = false;
    for (std::size_t slot = 0; slot < slots && slot < grids.size(); ++slot)
    {
        element.nodes[slot] = grids[slot];
        anyMidSide = anyMidSide || (slot >= corners && grids[slot] != 0);
    }
    // The GRIDID of a CWELD: Q4 to Q8 and T3 to T6, the mid-side points optional.
    if (quadrilateral)
        element.kind = anyMidSide ? ShellShapeKind::Quad8 : ShellShapeKind::Quad4;
    else
        element.kind = anyMidSide ? ShellShapeKind::Tria6 : ShellShapeKind::Tria3;
    if (!place(element))
        return std::nullopt;
    return element;
}

} // namespace oor::analysis::detail
