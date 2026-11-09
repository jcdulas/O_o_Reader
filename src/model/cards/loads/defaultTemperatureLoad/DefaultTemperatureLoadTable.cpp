#include "oor/model/cards/loads/defaultTemperatureLoad/DefaultTemperatureLoadTable.hpp"

namespace oor::model
{

namespace
{

constexpr int MAX_PAIRS = 4;

} // namespace

std::vector<DefaultTemperatureLoadCard> DefaultTemperatureLoadFormat::readAll(FieldReader& reader)
{
    static constexpr const char* SET_NAMES[MAX_PAIRS] = {"id1", "id2", "id3", "id4"};
    static constexpr const char* TEMPERATURE_NAMES[MAX_PAIRS] = {"temperature1", "temperature2", "temperature3",
                                                                 "temperature4"};
    std::vector<DefaultTemperatureLoadCard> cards;
    for (int pair = 0; pair < MAX_PAIRS; ++pair)
    {
        const int first = 1 + 2 * pair;
        // The first pair is required; a later one is there as soon as one of its fields is.
        if (pair > 0 && reader.isBlank(first) && reader.isBlank(first + 1))
            continue;
        DefaultTemperatureLoadCard card;
        card.id = reader.requiredInteger(first, SET_NAMES[pair]);
        card.temperature = reader.requiredReal(first + 1, TEMPERATURE_NAMES[pair]);
        if (!reader.failed())
            reader.report(check(card));
        cards.push_back(card);
    }
    if (reader.lastUsedField() > 2 * MAX_PAIRS)
        reader.error("card.bad-value", "the card defines at most four temperatures", "temperature4");
    return cards;
}

std::optional<CardProblem> DefaultTemperatureLoadFormat::check(const DefaultTemperatureLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the temperature set ID must be a positive integer", "id"};
    return std::nullopt;
}

void DefaultTemperatureLoadFormat::write(const DefaultTemperatureLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.temperature);
}

std::uint32_t DefaultTemperatureLoadTable::append(const DefaultTemperatureLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_temperature.push(card.temperature);
    return row;
}

DefaultTemperatureLoadCard DefaultTemperatureLoadTable::card(std::uint32_t row) const
{
    return DefaultTemperatureLoadCard{m_id.get(row), m_temperature.get(row)};
}

void DefaultTemperatureLoadTable::store(std::uint32_t row, const DefaultTemperatureLoadCard& card)
{
    m_id.set(row, card.id);
    m_temperature.set(row, card.temperature);
}

void DefaultTemperatureLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_temperature.keepRows(rows);
}

void DefaultTemperatureLoadTable::visitReferences(std::uint32_t /*row*/, ReferenceVisitor& /*visitor*/)
{
    // A default temperature refers to nothing: its ID is the set it belongs to.
}

} // namespace oor::model
