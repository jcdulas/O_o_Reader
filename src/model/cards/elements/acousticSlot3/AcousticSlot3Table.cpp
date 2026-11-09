#include "oor/model/cards/elements/acousticSlot3/AcousticSlot3Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int UNUSED_FIELD = 5; // the blank cell between IDS3 and RHO
constexpr int LAST_FIELD = 8;
constexpr base::EntityId ID_LIMIT = 100000000; // an element ID is below it (format reference, EID)

constexpr const char* NODE_NAMES[AcousticSlot3Card::NODE_COUNT] = {"slotPoint1", "slotPoint2", "slotPoint3"};

} // namespace

std::optional<AcousticSlot3Card> AcousticSlot3Format::read(FieldReader& reader)
{
    AcousticSlot3Card card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < AcousticSlot3Card::NODE_COUNT; ++index)
        card.nodes[index] = reader.requiredInteger(2 + static_cast<int>(index), NODE_NAMES[index]);
    requireBlankFields(reader, {UNUSED_FIELD}, LAST_FIELD);
    card.density = reader.optionalReal(6, "density");
    card.bulkModulus = reader.optionalReal(7, "bulkModulus");
    card.slotCount = reader.optionalInteger(8, "slotCount");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticSlot3Format::check(const AcousticSlot3Card& card)
{
    if (card.id <= 0 || card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
    for (std::size_t index = 0; index < AcousticSlot3Card::NODE_COUNT; ++index)
    {
        if (card.nodes[index] <= 0)
            return CardProblem{"card.bad-id", "the slot point IDs must be positive integers", NODE_NAMES[index]};
    }
    if (const auto problem = checkOptionalReal(card.density, "density", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.bulkModulus, "bulkModulus", RealRule::NonNegative))
        return problem;
    if (card.slotCount && *card.slotCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "slotCount"};
    return std::nullopt;
}

void AcousticSlot3Format::write(const AcousticSlot3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < AcousticSlot3Card::NODE_COUNT; ++index)
        writer.integer(2 + static_cast<int>(index), card.nodes[index]);
    if (card.density)
        writer.real(6, *card.density);
    if (card.bulkModulus)
        writer.real(7, *card.bulkModulus);
    if (card.slotCount)
        writer.integer(8, *card.slotCount);
}

std::uint32_t AcousticSlot3Table::append(const AcousticSlot3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodes.push(card.nodes);
    m_density.push(realToColumn(card.density));
    m_bulkModulus.push(realToColumn(card.bulkModulus));
    m_slotCount.push(card.slotCount.value_or(NO_SLOT_COUNT));
    return row;
}

AcousticSlot3Card AcousticSlot3Table::card(std::uint32_t row) const
{
    AcousticSlot3Card card;
    card.id = m_id.get(row);
    card.nodes = m_nodes.get(row);
    card.density = realFromColumn(m_density.get(row));
    card.bulkModulus = realFromColumn(m_bulkModulus.get(row));
    if (const std::int32_t m = m_slotCount.get(row); m != NO_SLOT_COUNT)
        card.slotCount = m;
    return card;
}

void AcousticSlot3Table::store(std::uint32_t row, const AcousticSlot3Card& card)
{
    m_id.set(row, card.id);
    m_nodes.set(row, card.nodes);
    m_density.set(row, realToColumn(card.density));
    m_bulkModulus.set(row, realToColumn(card.bulkModulus));
    m_slotCount.set(row, card.slotCount.value_or(NO_SLOT_COUNT));
}

void AcousticSlot3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_nodes.keepRows(rows);
    m_density.keepRows(rows);
    m_bulkModulus.keepRows(rows);
    m_slotCount.keepRows(rows);
}

void AcousticSlot3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The slot points are GRIDS cards, which a later step types: they share the node ID space, so they are nodes.
    std::array<base::EntityId, AcousticSlot3Card::NODE_COUNT> nodes = m_nodes.get(row);
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        if (node <= 0)
            continue;
        const base::EntityId updated = visitor.visit("nodes", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
}

} // namespace oor::model
