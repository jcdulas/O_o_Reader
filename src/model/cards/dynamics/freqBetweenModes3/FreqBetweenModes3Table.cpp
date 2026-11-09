#include "oor/model/cards/dynamics/freqBetweenModes3/FreqBetweenModes3Table.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

// TYPE: blank is LINEAR.
FrequencySpacing readType(FieldReader& reader, int field)
{
    const std::string_view text = reader.text(field);
    if (text.empty() || (text == "Linear"))
        return FrequencySpacing::Linear;
    if ((text == "Logarithmic"))
        return FrequencySpacing::Log;
    reader.error("card.bad-value", std::string("kind") + " must be LINEAR or LOG, found \"" + std::string(text) + "\"",
                 "kind");
    return FrequencySpacing::Linear;
}

} // namespace

std::optional<FreqBetweenModes3Card> FreqBetweenModes3Format::read(FieldReader& reader)
{
    FreqBetweenModes3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.frequency1 = reader.requiredReal(2, "frequency1");
    card.frequency2 = reader.real(3, "frequency2", card.frequency1); // a blank F2 is F1
    card.kind = readType(reader, 4);
    card.frequencyCount = reader.integer(5, "frequencyCount", 10);
    card.clustering = reader.real(6, "clustering", 1.0);
    if (reader.lastUsedField() > 6)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("clustering"), "clustering");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreqBetweenModes3Format::check(const FreqBetweenModes3Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the frequency set ID must be a positive integer", "id"};
    // The format reference asks for F1 > 0 on a logarithmic spread (the spread works on the logarithm of the
    // frequencies) and >= 0 on a linear one.
    if (card.kind == FrequencySpacing::Log ? !(card.frequency1 > 0.0) : !(card.frequency1 >= 0.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) +
                               (card.kind == FrequencySpacing::Log
                                    ? " must be greater than zero for " + CardProblem::fieldNamed("kind") + " = LOG"
                                    : std::string(" must not be negative")),
                           "frequency1"};
    // The format reference asks for F2 > 0 and F2 >= F1: a deck of the public set has F2 = F1 = 0.0 (F2 blank), so only
    // the order is checked.
    if (!(card.frequency2 >= card.frequency1))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must not be below " + CardProblem::fieldNamed("frequency1"),
            "frequency2"};
    if (card.frequencyCount < 2)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be an integer greater than one",
                           "frequencyCount"};
    if (!(card.clustering > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "clustering"};
    return std::nullopt;
}

void FreqBetweenModes3Format::write(const FreqBetweenModes3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.frequency1);
    writer.real(3, card.frequency2); // F2 depends on F1 when blank: it is always written
    writer.textUnlessDefault(4, card.kind == FrequencySpacing::Log ? "Logarithmic" : "Linear", "Linear");
    writer.integerUnlessDefault(5, card.frequencyCount, 10);
    writer.realUnlessDefault(6, card.clustering, 1.0);
}

std::uint32_t FreqBetweenModes3Table::append(const FreqBetweenModes3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frequency1.push(card.frequency1);
    m_frequency2.push(card.frequency2);
    m_kind.push(static_cast<std::uint8_t>(card.kind));
    m_frequencyCount.push(card.frequencyCount);
    m_clustering.push(card.clustering);
    return row;
}

FreqBetweenModes3Card FreqBetweenModes3Table::card(std::uint32_t row) const
{
    return FreqBetweenModes3Card{m_id.get(row),
                                 m_frequency1.get(row),
                                 m_frequency2.get(row),
                                 static_cast<FrequencySpacing>(m_kind.get(row)),
                                 m_frequencyCount.get(row),
                                 m_clustering.get(row)};
}

void FreqBetweenModes3Table::store(std::uint32_t row, const FreqBetweenModes3Card& card)
{
    m_id.set(row, card.id);
    m_frequency1.set(row, card.frequency1);
    m_frequency2.set(row, card.frequency2);
    m_kind.set(row, static_cast<std::uint8_t>(card.kind));
    m_frequencyCount.set(row, card.frequencyCount);
    m_clustering.set(row, card.clustering);
}

void FreqBetweenModes3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frequency1.keepRows(rows);
    m_frequency2.keepRows(rows);
    m_kind.keepRows(rows);
    m_frequencyCount.keepRows(rows);
    m_clustering.keepRows(rows);
}

} // namespace oor::model
