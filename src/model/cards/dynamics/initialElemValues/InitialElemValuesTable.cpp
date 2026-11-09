#include "oor/model/cards/dynamics/initialElemValues/InitialElemValuesTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_VARIABLE_FIELD = 3;

} // namespace

std::optional<InitialElemValuesCard> InitialElemValuesFormat::read(FieldReader& reader)
{
    InitialElemValuesCard card;
    card.id = reader.requiredInteger(1, "id");
    card.elementSetId = reader.requiredInteger(2, "elementSetId");
    card.variables = readInitialVariables(reader, FIRST_VARIABLE_FIELD, InitialValueMode::Numbers);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> InitialElemValuesFormat::check(const InitialElemValuesCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.elementSetId, "elementSetId"))
        return problem;
    return checkInitialVariables(card.variables, InitialValueMode::Numbers);
}

void InitialElemValuesFormat::write(const InitialElemValuesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.elementSetId);
    writeInitialVariables(writer, FIRST_VARIABLE_FIELD, InitialValueMode::Numbers, card.variables);
}

std::uint32_t InitialElemValuesTable::append(const InitialElemValuesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void InitialElemValuesTable::store(std::uint32_t row, const InitialElemValuesCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void InitialElemValuesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void InitialElemValuesTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void InitialElemValuesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "elementSetId", ReferenceTarget::Set, m_cards.edit(row).elementSetId);
}

} // namespace oor::model
