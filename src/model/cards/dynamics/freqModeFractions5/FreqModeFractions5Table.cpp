#include "oor/model/cards/dynamics/freqModeFractions5/FreqModeFractions5Table.hpp"

namespace oor::model
{

std::optional<FreqModeFractions5Card> FreqModeFractions5Format::read(FieldReader& reader)
{
    FreqModeFractions5Card card;
    card.id = reader.requiredInteger(1, "id");
    card.frequency1 = reader.real(2, "frequency1", 0.0);
    card.frequency2 = reader.real(3, "frequency2", 1.0E20);
    // The fractions follow from field 4 over the continuation lines. A blank field is padding.
    const int last = reader.lastUsedField();
    for (int field = 4; field <= last && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field))
            card.fractions.push_back(reader.requiredReal(field, "fractions"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreqModeFractions5Format::check(const FreqModeFractions5Card& card)
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
    if (card.fractions.empty())
        return CardProblem{"card.missing-field", "the card needs at least one fraction", "fraction1"};
    for (const double fraction : card.fractions)
    {
        if (!(fraction > 0.0))
            return CardProblem{"card.bad-value", "a fraction of a natural frequency must be greater than zero",
                               "fractions"};
    }
    return std::nullopt;
}

void FreqModeFractions5Format::write(const FreqModeFractions5Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.frequency1, 0.0);
    writer.realUnlessDefault(3, card.frequency2, 1.0E20);
    int field = 4;
    for (const double fraction : card.fractions)
        writer.real(field++, fraction);
}

std::uint32_t FreqModeFractions5Table::append(const FreqModeFractions5Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frequency1.push(card.frequency1);
    m_frequency2.push(card.frequency2);
    m_fractions.push(card.fractions);
    return row;
}

FreqModeFractions5Card FreqModeFractions5Table::card(std::uint32_t row) const
{
    FreqModeFractions5Card card;
    card.id = m_id.get(row);
    card.frequency1 = m_frequency1.get(row);
    card.frequency2 = m_frequency2.get(row);
    const auto fractions = m_fractions.get(row);
    card.fractions.assign(fractions.begin(), fractions.end());
    return card;
}

void FreqModeFractions5Table::store(std::uint32_t row, const FreqModeFractions5Card& card)
{
    m_id.set(row, card.id);
    m_frequency1.set(row, card.frequency1);
    m_frequency2.set(row, card.frequency2);
    m_fractions.set(row, card.fractions);
}

void FreqModeFractions5Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frequency1.keepRows(rows);
    m_frequency2.keepRows(rows);
    m_fractions.keepRows(rows);
}

} // namespace oor::model
