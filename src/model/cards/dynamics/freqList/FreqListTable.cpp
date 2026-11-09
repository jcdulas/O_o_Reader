#include "oor/model/cards/dynamics/freqList/FreqListTable.hpp"

namespace oor::model
{

std::optional<FreqListCard> FreqListFormat::read(FieldReader& reader)
{
    FreqListCard card;
    card.id = reader.requiredInteger(1, "id");
    // The frequencies follow each other from field 2, over as many continuation lines as needed. A blank field is
    // padding.
    const int last = reader.lastUsedField();
    for (int field = 2; field <= last && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field))
            card.frequencies.push_back(reader.requiredReal(field, "frequencies"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreqListFormat::check(const FreqListCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the frequency set ID must be a positive integer", "id"};
    if (card.frequencies.empty())
        return CardProblem{"card.missing-field", "the card needs at least one frequency", "frequency1"};
    for (const double frequency : card.frequencies)
    {
        // The format reference says greater than zero; zero (the static solution) is used in practice and allowed.
        if (!(frequency >= 0.0))
            return CardProblem{"card.bad-value", "a frequency must not be negative", "frequencies"};
    }
    return std::nullopt;
}

void FreqListFormat::write(const FreqListCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = 2;
    for (const double frequency : card.frequencies)
        writer.real(field++, frequency);
}

std::uint32_t FreqListTable::append(const FreqListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frequencies.push(card.frequencies);
    return row;
}

FreqListCard FreqListTable::card(std::uint32_t row) const
{
    FreqListCard card;
    card.id = m_id.get(row);
    const auto frequencies = m_frequencies.get(row);
    card.frequencies.assign(frequencies.begin(), frequencies.end());
    return card;
}

void FreqListTable::store(std::uint32_t row, const FreqListCard& card)
{
    m_id.set(row, card.id);
    m_frequencies.set(row, card.frequencies);
}

void FreqListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frequencies.keepRows(rows);
}

} // namespace oor::model
