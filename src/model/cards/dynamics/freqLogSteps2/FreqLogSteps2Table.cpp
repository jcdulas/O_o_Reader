#include "oor/model/cards/dynamics/freqLogSteps2/FreqLogSteps2Table.hpp"

namespace oor::model
{

std::optional<FreqLogSteps2Card> FreqLogSteps2Format::read(FieldReader& reader)
{
    FreqLogSteps2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.frequency1 = reader.requiredReal(2, "frequency1");
    card.frequency2 = reader.requiredReal(3, "frequency2");
    card.intervalCount = reader.integer(4, "intervalCount", 1);
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("intervalCount"), "intervalCount");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreqLogSteps2Format::check(const FreqLogSteps2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the frequency set ID must be a positive integer", "id"};
    if (!(card.frequency1 > 0.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be greater than zero (the steps are logarithmic)",
                           "frequency1"};
    if (!(card.frequency2 > card.frequency1))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be greater than " + CardProblem::fieldNamed("frequency1"),
            "frequency2"};
    if (card.intervalCount < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "intervalCount"};
    return std::nullopt;
}

void FreqLogSteps2Format::write(const FreqLogSteps2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.frequency1);
    writer.real(3, card.frequency2);
    writer.integerUnlessDefault(4, card.intervalCount, 1);
}

std::uint32_t FreqLogSteps2Table::append(const FreqLogSteps2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frequency1.push(card.frequency1);
    m_frequency2.push(card.frequency2);
    m_intervalCount.push(card.intervalCount);
    return row;
}

FreqLogSteps2Card FreqLogSteps2Table::card(std::uint32_t row) const
{
    return FreqLogSteps2Card{m_id.get(row), m_frequency1.get(row), m_frequency2.get(row), m_intervalCount.get(row)};
}

void FreqLogSteps2Table::store(std::uint32_t row, const FreqLogSteps2Card& card)
{
    m_id.set(row, card.id);
    m_frequency1.set(row, card.frequency1);
    m_frequency2.set(row, card.frequency2);
    m_intervalCount.set(row, card.intervalCount);
}

void FreqLogSteps2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frequency1.keepRows(rows);
    m_frequency2.keepRows(rows);
    m_intervalCount.keepRows(rows);
}

} // namespace oor::model
