#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqList/NonlinearHarmonicFreqListTable.hpp"

#include <cmath>

namespace oor::model
{

std::optional<NonlinearHarmonicFreqListCard> NonlinearHarmonicFreqListFormat::read(FieldReader& reader)
{
    NonlinearHarmonicFreqListCard card;
    card.id = reader.requiredInteger(1, "id");
    // The frequencies follow from field 2 over the continuation lines. A blank field is padding.
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

std::optional<CardProblem> NonlinearHarmonicFreqListFormat::check(const NonlinearHarmonicFreqListCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.frequencies.empty())
        return CardProblem{"card.missing-field", "the card needs at least one frequency", "frequencies"};
    for (const double frequency : card.frequencies)
    {
        if (!(frequency > 0.0) || !std::isfinite(frequency))
            return CardProblem{"card.bad-value", "a forcing frequency must be greater than zero", "frequencies"};
    }
    return std::nullopt;
}

void NonlinearHarmonicFreqListFormat::write(const NonlinearHarmonicFreqListCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = 2;
    for (const double frequency : card.frequencies)
        writer.real(field++, frequency);
}

std::uint32_t NonlinearHarmonicFreqListTable::append(const NonlinearHarmonicFreqListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frequencies.push(card.frequencies);
    return row;
}

NonlinearHarmonicFreqListCard NonlinearHarmonicFreqListTable::card(std::uint32_t row) const
{
    NonlinearHarmonicFreqListCard card;
    card.id = m_id.get(row);
    const auto frequencies = m_frequencies.get(row);
    card.frequencies.assign(frequencies.begin(), frequencies.end());
    return card;
}

void NonlinearHarmonicFreqListTable::store(std::uint32_t row, const NonlinearHarmonicFreqListCard& card)
{
    m_id.set(row, card.id);
    m_frequencies.set(row, card.frequencies);
}

void NonlinearHarmonicFreqListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frequencies.keepRows(rows);
}

} // namespace oor::model
