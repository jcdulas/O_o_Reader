#include "oor/model/cards/dynamics/freqSpreadAroundModes4/FreqSpreadAroundModes4Table.hpp"

namespace oor::model
{

std::optional<FreqSpreadAroundModes4Card> FreqSpreadAroundModes4Format::read(FieldReader& reader)
{
    FreqSpreadAroundModes4Card card;
    card.id = reader.requiredInteger(1, "id");
    card.frequency1 = reader.real(2, "frequency1", 0.0);
    card.frequency2 = reader.real(3, "frequency2", 1.0E20);
    card.frequencySpread = reader.real(4, "frequencySpread", 0.10);
    card.frequencyCount = reader.integer(5, "frequencyCount", 3);
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("frequencyCount"),
                     "frequencyCount");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreqSpreadAroundModes4Format::check(const FreqSpreadAroundModes4Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the frequency set ID must be a positive integer", "id"};
    if (!(card.frequency1 >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "frequency1"};
    if (!(card.frequency2 > card.frequency1))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be greater than " + CardProblem::fieldNamed("frequency1"),
            "frequency2"};
    if (!(card.frequencySpread > 0.0 && card.frequencySpread < 1.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 0.0 and 1.0",
                           "frequencySpread"};
    if (card.frequencyCount < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "frequencyCount"};
    return std::nullopt;
}

void FreqSpreadAroundModes4Format::write(const FreqSpreadAroundModes4Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.frequency1, 0.0);
    writer.realUnlessDefault(3, card.frequency2, 1.0E20);
    writer.realUnlessDefault(4, card.frequencySpread, 0.10);
    writer.integerUnlessDefault(5, card.frequencyCount, 3);
}

std::uint32_t FreqSpreadAroundModes4Table::append(const FreqSpreadAroundModes4Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frequency1.push(card.frequency1);
    m_frequency2.push(card.frequency2);
    m_frequencySpread.push(card.frequencySpread);
    m_frequencyCount.push(card.frequencyCount);
    return row;
}

FreqSpreadAroundModes4Card FreqSpreadAroundModes4Table::card(std::uint32_t row) const
{
    return FreqSpreadAroundModes4Card{m_id.get(row), m_frequency1.get(row), m_frequency2.get(row),
                                      m_frequencySpread.get(row), m_frequencyCount.get(row)};
}

void FreqSpreadAroundModes4Table::store(std::uint32_t row, const FreqSpreadAroundModes4Card& card)
{
    m_id.set(row, card.id);
    m_frequency1.set(row, card.frequency1);
    m_frequency2.set(row, card.frequency2);
    m_frequencySpread.set(row, card.frequencySpread);
    m_frequencyCount.set(row, card.frequencyCount);
}

void FreqSpreadAroundModes4Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frequency1.keepRows(rows);
    m_frequency2.keepRows(rows);
    m_frequencySpread.keepRows(rows);
    m_frequencyCount.keepRows(rows);
}

} // namespace oor::model
