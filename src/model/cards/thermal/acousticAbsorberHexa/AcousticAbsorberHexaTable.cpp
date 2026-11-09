#include "oor/model/cards/thermal/acousticAbsorberHexa/AcousticAbsorberHexaTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr std::size_t CORNER_COUNT = 8;

} // namespace

int AcousticAbsorberHexaFormat::fieldOfNode(std::size_t index) noexcept
{
    // G1 to G12 are fields 3 to 14; G13 to G16 do not exist, and G17 to G20 are fields 19 to 22.
    return index < 12 ? 3 + static_cast<int>(index) : 19 + static_cast<int>(index - 12);
}

std::optional<AcousticAbsorberHexaCard> AcousticAbsorberHexaFormat::read(FieldReader& reader)
{
    AcousticAbsorberHexaCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    for (std::size_t index = 0; index < AcousticAbsorberHexaCard::NODE_COUNT; ++index)
    {
        const std::string name = "node" + std::to_string(index < 12 ? index + 1 : index + 5);
        card.nodes[index] = index < CORNER_COUNT ? reader.requiredInteger(fieldOfNode(index), name.c_str())
                                                 : reader.integer(fieldOfNode(index), name.c_str(), 0);
    }
    requireUnusedBlank(reader, {15, 16, 17, 18}, 22);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticAbsorberHexaFormat::check(const AcousticAbsorberHexaCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    for (std::size_t index = 0; index < AcousticAbsorberHexaCard::NODE_COUNT; ++index)
    {
        if (card.nodes[index] < 0)
            return CardProblem{"card.bad-id", "a grid point ID cannot be negative", "node"};
        // the corner grid points cannot be deleted.
        if (index < CORNER_COUNT && card.nodes[index] == 0)
            return CardProblem{"card.missing-field",
                               "the corner grid points " + CardProblem::fieldNamed("node1") + " to " +
                                   CardProblem::fieldNamed("node8") + " are required",
                               "node"};
    }
    for (std::size_t first = 0; first < AcousticAbsorberHexaCard::NODE_COUNT; ++first)
    {
        for (std::size_t second = first + 1; second < AcousticAbsorberHexaCard::NODE_COUNT; ++second)
        {
            if (card.nodes[first] != 0 && card.nodes[first] == card.nodes[second])
                return CardProblem{"card.bad-value", "a grid point is repeated in the element", "node"};
        }
    }
    return std::nullopt;
}

void AcousticAbsorberHexaFormat::write(const AcousticAbsorberHexaCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (std::size_t index = 0; index < AcousticAbsorberHexaCard::NODE_COUNT; ++index)
    {
        if (index < CORNER_COUNT)
            writer.integer(fieldOfNode(index), card.nodes[index]);
        else
            writer.integerUnlessDefault(fieldOfNode(index), card.nodes[index], 0);
    }
}

std::uint32_t AcousticAbsorberHexaTable::append(const AcousticAbsorberHexaCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

AcousticAbsorberHexaCard AcousticAbsorberHexaTable::card(std::uint32_t row) const
{
    AcousticAbsorberHexaCard card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.nodes = m_nodes.get(row);
    return card;
}

void AcousticAbsorberHexaTable::store(std::uint32_t row, const AcousticAbsorberHexaCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void AcousticAbsorberHexaTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void AcousticAbsorberHexaTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, m_propId.get(row)));
    std::array<base::EntityId, AcousticAbsorberHexaCard::NODE_COUNT> nodes = m_nodes.get(row);
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        if (node <= 0)
            continue;
        const base::EntityId updated = visitor.visit("node", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
}

} // namespace oor::model
