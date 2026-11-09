#include "oor/model/cards/dynamics/freqLinearSteps1/FreqLinearSteps1Table.hpp"

namespace oor::model
{

std::optional<FreqLinearSteps1Card> FreqLinearSteps1Format::read(FieldReader& reader)
{
    FreqLinearSteps1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.frequency1 = reader.real(2, "frequency1", 0.0);
    card.frequencyIncrement = reader.requiredReal(3, "frequencyIncrement");
    card.incrementCount = reader.integer(4, "incrementCount", 1);
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("incrementCount"),
                     "incrementCount");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreqLinearSteps1Format::check(const FreqLinearSteps1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the frequency set ID must be a positive integer", "id"};
    if (!(card.frequency1 >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "frequency1"};
    if (!(card.frequencyIncrement > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "frequencyIncrement"};
    if (card.incrementCount < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "incrementCount"};
    return std::nullopt;
}

void FreqLinearSteps1Format::write(const FreqLinearSteps1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.frequency1, 0.0);
    writer.real(3, card.frequencyIncrement);
    writer.integerUnlessDefault(4, card.incrementCount, 1);
}

std::uint32_t FreqLinearSteps1Table::append(const FreqLinearSteps1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frequency1.push(card.frequency1);
    m_frequencyIncrement.push(card.frequencyIncrement);
    m_incrementCount.push(card.incrementCount);
    return row;
}

FreqLinearSteps1Card FreqLinearSteps1Table::card(std::uint32_t row) const
{
    return FreqLinearSteps1Card{m_id.get(row), m_frequency1.get(row), m_frequencyIncrement.get(row),
                                m_incrementCount.get(row)};
}

void FreqLinearSteps1Table::store(std::uint32_t row, const FreqLinearSteps1Card& card)
{
    m_id.set(row, card.id);
    m_frequency1.set(row, card.frequency1);
    m_frequencyIncrement.set(row, card.frequencyIncrement);
    m_incrementCount.set(row, card.incrementCount);
}

void FreqLinearSteps1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frequency1.keepRows(rows);
    m_frequencyIncrement.keepRows(rows);
    m_incrementCount.keepRows(rows);
}

} // namespace oor::model
