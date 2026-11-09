#include "oor/model/cards/elements/pointMassMatrix/PointMassMatrixTable.hpp"

#include <string>

namespace oor::model
{

namespace
{

// The name of the value in `massMatrix` at `index`: massTerm11, massTerm21, massTerm22, massTerm31, ... (M11, M21, ...)
std::string valueName(std::size_t index)
{
    std::size_t row = 1;
    while ((row + 1) * row / 2 <= index)
        ++row;
    const std::size_t column = index - row * (row - 1) / 2 + 1;
    return "massTerm" + std::to_string(row) + std::to_string(column);
}

} // namespace

std::optional<PointMassMatrixCard> PointMassMatrixFormat::read(FieldReader& reader)
{
    PointMassMatrixCard card;
    card.id = reader.requiredInteger(1, "id");
    card.node = reader.requiredInteger(2, "node");
    card.frameId = reader.integer(3, "frameId", 0);
    // The 21 values are the fields 4 to 24, one after the other (the continuation lines carry on the numbering).
    for (std::size_t index = 0; index < PointMassMatrixCard::VALUE_COUNT; ++index)
    {
        const std::string name = valueName(index);
        card.massMatrix[index] = reader.real(4 + static_cast<int>(index), name.c_str(), 0.0);
    }
    if (reader.lastUsedField() > 24)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 21 mass values (fields 4 to 24)",
                     "massTerm66");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PointMassMatrixFormat::check(const PointMassMatrixCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.node <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "node"};
    if (card.frameId < 0)
        return CardProblem{"card.bad-value", "{field} must be 0 or a coordinate system ID", "frameId"};
    return std::nullopt;
}

void PointMassMatrixFormat::write(const PointMassMatrixCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.node);
    writer.integerUnlessDefault(3, card.frameId, 0);
    for (std::size_t index = 0; index < PointMassMatrixCard::VALUE_COUNT; ++index)
        writer.realUnlessDefault(4 + static_cast<int>(index), card.massMatrix[index], 0.0);
}

std::uint32_t PointMassMatrixTable::append(const PointMassMatrixCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_node.push(card.node);
    m_frameId.push(card.frameId);
    m_massMatrix.push(card.massMatrix);
    return row;
}

PointMassMatrixCard PointMassMatrixTable::card(std::uint32_t row) const
{
    return PointMassMatrixCard{m_id.get(row), m_node.get(row), m_frameId.get(row), m_massMatrix.get(row)};
}

void PointMassMatrixTable::store(std::uint32_t row, const PointMassMatrixCard& card)
{
    m_id.set(row, card.id);
    m_node.set(row, card.node);
    m_frameId.set(row, card.frameId);
    m_massMatrix.set(row, card.massMatrix);
}

void PointMassMatrixTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_frameId.keepRows(rows);
    m_massMatrix.keepRows(rows);
}

void PointMassMatrixTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_node.set(row, visitor.visit("node", ReferenceTarget::Node, m_node.get(row)));
    // CID 0 is the basic system: it does not refer to a card.
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
