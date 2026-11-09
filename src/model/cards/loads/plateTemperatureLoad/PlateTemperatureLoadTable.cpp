#include "oor/model/cards/loads/plateTemperatureLoad/PlateTemperatureLoadTable.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// Fields 7 and 8 of the first line are blank; the other elements start on the first continuation line.
constexpr int FIRST_MORE_ELEMENT_FIELD = 9;

} // namespace

std::optional<PlateTemperatureLoadCard> PlateTemperatureLoadFormat::read(FieldReader& reader)
{
    PlateTemperatureLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    const base::EntityId first = reader.requiredInteger(2, "elemId1");
    card.meanTemperature = reader.real(3, "meanTemperature", 0.0);
    card.temperatureGradient = reader.real(4, "temperatureGradient", 0.0);
    card.temperature1 = reader.real(5, "temperature1", PlateTemperatureLoadCard::BLANK_TEMPERATURE);
    card.temperature2 = reader.real(6, "temperature2", PlateTemperatureLoadCard::BLANK_TEMPERATURE);
    if (!reader.isBlank(7) || !reader.isBlank(8))
        reader.error("card.bad-value", "fields 7 and 8 of the card are blank", reader.isBlank(7) ? "8" : "7");
    card.elements.push_back(ElementRun{first, first});
    for (const ElementRun& run : readElementRuns(reader, FIRST_MORE_ELEMENT_FIELD, "elemId"))
        card.elements.push_back(run);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PlateTemperatureLoadFormat::check(const PlateTemperatureLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the temperature set ID must be a positive integer", "id"};
    if (card.elements.empty())
        return CardProblem{"card.missing-field", "the card needs at least one element", "elemId1"};
    if (card.elements.front().first != card.elements.front().last)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " is a single element: ranges go on the continuation lines", "elemId1"};
    return checkElementRuns(card.elements, "elemId1");
}

void PlateTemperatureLoadFormat::write(const PlateTemperatureLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.elements.empty() ? 0 : card.elements.front().first);
    writer.realUnlessDefault(3, card.meanTemperature, 0.0);
    writer.realUnlessDefault(4, card.temperatureGradient, 0.0);
    if (!std::isnan(card.temperature1))
        writer.real(5, card.temperature1);
    if (!std::isnan(card.temperature2))
        writer.real(6, card.temperature2);
    if (card.elements.size() > 1)
    {
        const std::vector<ElementRun> more(card.elements.begin() + 1, card.elements.end());
        writeElementRuns(writer, FIRST_MORE_ELEMENT_FIELD, more, writer.compressesRanges());
    }
}

std::uint32_t PlateTemperatureLoadTable::append(const PlateTemperatureLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_meanTemperature.push(card.meanTemperature);
    m_temperatureGradient.push(card.temperatureGradient);
    m_temperature1.push(card.temperature1);
    m_temperature2.push(card.temperature2);
    m_runs.push(toPairs(card.elements));
    return row;
}

PlateTemperatureLoadCard PlateTemperatureLoadTable::card(std::uint32_t row) const
{
    PlateTemperatureLoadCard card;
    card.id = m_id.get(row);
    card.meanTemperature = m_meanTemperature.get(row);
    card.temperatureGradient = m_temperatureGradient.get(row);
    card.temperature1 = m_temperature1.get(row);
    card.temperature2 = m_temperature2.get(row);
    card.elements = toRuns(m_runs.get(row));
    return card;
}

void PlateTemperatureLoadTable::store(std::uint32_t row, const PlateTemperatureLoadCard& card)
{
    m_id.set(row, card.id);
    m_meanTemperature.set(row, card.meanTemperature);
    m_temperatureGradient.set(row, card.temperatureGradient);
    m_temperature1.set(row, card.temperature1);
    m_temperature2.set(row, card.temperature2);
    m_runs.set(row, toPairs(card.elements));
}

void PlateTemperatureLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_meanTemperature.keepRows(rows);
    m_temperatureGradient.keepRows(rows);
    m_temperature1.keepRows(rows);
    m_temperature2.keepRows(rows);
    m_runs.keepRows(rows);
}

void PlateTemperatureLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitElementRuns(m_runs, row, visitor, "elemId1");
}

} // namespace oor::model
