#include "oor/model/cards/nodes/associativeNode/AssociativeNodeTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

// The name runs over the eight fields of the first continuation line.
constexpr int NAME_FIRST_FIELD = 9;
constexpr int NAME_LAST_FIELD = 16;
constexpr int NAME_FIELD_COUNT = NAME_LAST_FIELD - NAME_FIRST_FIELD + 1;

std::optional<CardProblem> partNameProblem(const std::string& name)
{
    return checkNameWord(name, "partitionName");
}

} // namespace

std::optional<AssociativeNodeCard> AssociativeNodeFormat::read(FieldReader& reader)
{
    AssociativeNodeCard card;
    card.id = reader.requiredInteger(1, "id");
    card.origNodeId = reader.requiredInteger(2, "origNodeId");
    // Fields 3 to 8 are the blank cells of the format; the name is on the continuation line.
    requireBlankFields(reader, {3, 4, 5, 6, 7, 8}, NAME_LAST_FIELD);
    card.partitionName = reader.spanText(NAME_FIRST_FIELD, NAME_LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AssociativeNodeFormat::check(const AssociativeNodeCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.origNodeId, "origNodeId"))
        return problem;
    return partNameProblem(card.partitionName);
}

void AssociativeNodeFormat::write(const AssociativeNodeCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.origNodeId);
    writer.spanText(NAME_FIRST_FIELD, NAME_FIELD_COUNT, card.partitionName);
}

std::uint32_t AssociativeNodeTable::append(const AssociativeNodeCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_origNodeId.push(card.origNodeId);
    m_partitionName.push(card.partitionName);
    return row;
}

AssociativeNodeCard AssociativeNodeTable::card(std::uint32_t row) const
{
    AssociativeNodeCard card;
    card.id = m_id.get(row);
    card.origNodeId = m_origNodeId.get(row);
    card.partitionName = m_partitionName.get(row);
    return card;
}

void AssociativeNodeTable::store(std::uint32_t row, const AssociativeNodeCard& card)
{
    m_id.set(row, card.id);
    m_origNodeId.set(row, card.origNodeId);
    m_partitionName.set(row, card.partitionName);
}

void AssociativeNodeTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_origNodeId.keepRows(rows);
    m_partitionName.keepRows(rows);
}

} // namespace oor::model
