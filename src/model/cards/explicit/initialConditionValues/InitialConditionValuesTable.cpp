#include "oor/model/cards/explicit/initialConditionValues/InitialConditionValuesTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_VARIABLE_FIELD = 3;

} // namespace

std::optional<InitialConditionValuesCard> InitialConditionValuesFormat::read(FieldReader& reader)
{
    InitialConditionValuesCard card;
    card.id = reader.requiredInteger(1, "id");
    card.method =
        readKeyword(reader, 2, "method", INITIAL_CONDITION_VALUES_METHOD_WORDS, InitialConditionValuesMethod::Normal);
    // The method decides whether a value is a number or the ID of a table, so it is read before the list.
    card.variables = readInitialVariables(reader, FIRST_VARIABLE_FIELD, card.valueMode());
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> InitialConditionValuesFormat::check(const InitialConditionValuesCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkInitialVariables(card.variables, card.valueMode());
}

void InitialConditionValuesFormat::write(const InitialConditionValuesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, INITIAL_CONDITION_VALUES_METHOD_WORDS, card.method,
                              InitialConditionValuesMethod::Normal);
    writeInitialVariables(writer, FIRST_VARIABLE_FIELD, card.valueMode(), card.variables);
}

std::uint32_t InitialConditionValuesTable::append(const InitialConditionValuesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void InitialConditionValuesTable::store(std::uint32_t row, const InitialConditionValuesCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void InitialConditionValuesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void InitialConditionValuesTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void InitialConditionValuesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    InitialConditionValuesCard& card = m_cards.edit(row);
    visitInitialTables(card.variables, card.valueMode(), visitor);
}

} // namespace oor::model
