#include "oor/model/cards/optimisation/designEquation/DesignEquationTable.hpp"

#include "oor/model/expressions/EquationSet.hpp"

namespace oor::model
{

std::optional<CardProblem> DesignEquationFormat::check(const DesignEquationCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    const EquationParse parsed = parseEquations(card.text);
    if (!parsed.ok())
        return CardProblem{"card.bad-equation",
                           "the equation is not valid: " + parsed.problem->message + " (character " +
                               std::to_string(parsed.problem->position) + " of the equation)",
                           "equation"};
    return std::nullopt;
}

std::optional<DesignEquationCard> DesignEquationFormat::read(FieldReader& reader)
{
    DesignEquationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.text = std::string(reader.text(2));
    if (card.text.empty())
        reader.error("card.missing-field", "DesignEquation needs its equations", "equation");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

void DesignEquationFormat::write(const DesignEquationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.text);
}

std::uint32_t DesignEquationTable::append(const DesignEquationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void DesignEquationTable::store(std::uint32_t row, const DesignEquationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void DesignEquationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignEquationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
