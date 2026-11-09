#include "oor/model/cards/loads/cyclicAzimuthTableLoad/CyclicAzimuthTableLoadTable.hpp"

namespace oor::model
{

std::optional<CyclicAzimuthTableLoadCard> CyclicAzimuthTableLoadFormat::read(FieldReader& reader)
{
    CyclicAzimuthTableLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.tableId1 = reader.requiredInteger(2, "tableId1");
    card.loadSetId1 = reader.requiredInteger(3, "loadSetId1");
    card.methodName1 = reader.integer(4, "methodName1", 0);
    // The second group is there as soon as its table or its load set is; both are then required.
    if (!reader.isBlank(5) || !reader.isBlank(6))
    {
        card.tableId2 = reader.requiredInteger(5, "tableId2");
        card.loadSetId2 = reader.requiredInteger(6, "loadSetId2");
    }
    card.methodName2 = reader.integer(7, "methodName2", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CyclicAzimuthTableLoadFormat::check(const CyclicAzimuthTableLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.tableId1 <= 0)
        return CardProblem{"card.bad-id", "the table ID must be a positive integer", "tableId1"};
    if (card.loadSetId1 <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "loadSetId1"};
    if (card.methodName1 != 0 && card.methodName1 != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "methodName1"};
    if (card.tableId2 < 0)
        return CardProblem{"card.bad-id", "the table ID cannot be negative", "tableId2"};
    if (card.loadSetId2 < 0)
        return CardProblem{"card.bad-id", "the load set ID cannot be negative", "loadSetId2"};
    if ((card.tableId2 == 0) != (card.loadSetId2 == 0))
        return CardProblem{
            "card.missing-field",
            CardProblem::fieldNamed("tableId2") + " and " + std::string(CardProblem::FIELD) + " go together",
            "loadSetId2"};
    if (card.methodName2 != 0 && card.methodName2 != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "methodName2"};
    if (!card.hasSecond() && card.methodName2 != 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " needs a second table and load set",
                           "methodName2"};
    return std::nullopt;
}

void CyclicAzimuthTableLoadFormat::write(const CyclicAzimuthTableLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.tableId1);
    writer.integer(3, card.loadSetId1);
    writer.integerUnlessDefault(4, card.methodName1, 0);
    if (card.hasSecond())
    {
        writer.integer(5, card.tableId2);
        writer.integer(6, card.loadSetId2);
        writer.integerUnlessDefault(7, card.methodName2, 0);
    }
}

std::uint32_t CyclicAzimuthTableLoadTable::append(const CyclicAzimuthTableLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_tableId1.push(card.tableId1);
    m_loadSetId1.push(card.loadSetId1);
    m_methodName1.push(card.methodName1);
    m_tableId2.push(card.tableId2);
    m_loadSetId2.push(card.loadSetId2);
    m_methodName2.push(card.methodName2);
    return row;
}

CyclicAzimuthTableLoadCard CyclicAzimuthTableLoadTable::card(std::uint32_t row) const
{
    CyclicAzimuthTableLoadCard card;
    card.id = m_id.get(row);
    card.tableId1 = m_tableId1.get(row);
    card.loadSetId1 = m_loadSetId1.get(row);
    card.methodName1 = m_methodName1.get(row);
    card.tableId2 = m_tableId2.get(row);
    card.loadSetId2 = m_loadSetId2.get(row);
    card.methodName2 = m_methodName2.get(row);
    return card;
}

void CyclicAzimuthTableLoadTable::store(std::uint32_t row, const CyclicAzimuthTableLoadCard& card)
{
    m_id.set(row, card.id);
    m_tableId1.set(row, card.tableId1);
    m_loadSetId1.set(row, card.loadSetId1);
    m_methodName1.set(row, card.methodName1);
    m_tableId2.set(row, card.tableId2);
    m_loadSetId2.set(row, card.loadSetId2);
    m_methodName2.set(row, card.methodName2);
}

void CyclicAzimuthTableLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_tableId1.keepRows(rows);
    m_loadSetId1.keepRows(rows);
    m_methodName1.keepRows(rows);
    m_tableId2.keepRows(rows);
    m_loadSetId2.keepRows(rows);
    m_methodName2.keepRows(rows);
}

void CyclicAzimuthTableLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // the tables are TABLED1 or TABLED2. The load sets are FORCE, MOMENT, PLOAD, ... cards.
    m_tableId1.set(row, visitor.visit("tableId1", ReferenceTarget::DynamicTable, m_tableId1.get(row)));
    m_loadSetId1.set(row, visitor.visit("loadSetId1", ReferenceTarget::LoadCombinationSet, m_loadSetId1.get(row)));
    if (const base::EntityId table = m_tableId2.get(row); table > 0)
        m_tableId2.set(row, visitor.visit("tableId2", ReferenceTarget::DynamicTable, table));
    if (const base::EntityId set = m_loadSetId2.get(row); set > 0)
        m_loadSetId2.set(row, visitor.visit("loadSetId2", ReferenceTarget::LoadCombinationSet, set));
}

} // namespace oor::model
