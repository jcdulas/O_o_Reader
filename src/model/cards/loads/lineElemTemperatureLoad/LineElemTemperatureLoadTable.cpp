#include "oor/model/cards/loads/lineElemTemperatureLoad/LineElemTemperatureLoadTable.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// The point temperatures are the second continuation line (fields 9 to 16); the other elements the third (field 17 on).
constexpr int FIRST_POINT_FIELD = 9;
constexpr int FIRST_MORE_ELEMENT_FIELD = 17;

std::size_t countGiven(const LineElemTemperatureLoadCard& card)
{
    std::size_t given = 0;
    for (const double value : card.pointsA)
        given += std::isnan(value) ? 0 : 1;
    for (const double value : card.pointsB)
        given += std::isnan(value) ? 0 : 1;
    return given;
}

} // namespace

std::optional<LineElemTemperatureLoadCard> LineElemTemperatureLoadFormat::read(FieldReader& reader)
{
    LineElemTemperatureLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    const base::EntityId first = reader.requiredInteger(2, "elemId1");
    card.temperatureA = reader.real(3, "temperatureA", 0.0);
    card.temperatureB = reader.real(4, "temperatureB", 0.0);
    card.gradientY[0] = reader.real(5, "gradientYA", 0.0);
    card.gradientY[1] = reader.real(6, "gradientYB", 0.0);
    card.gradientZ[0] = reader.real(7, "gradientZA", 0.0);
    card.gradientZ[1] = reader.real(8, "gradientZB", 0.0);

    // The point temperatures: with none given the card has none; with some given a blank one is TA (end A) or TB (end
    // B).
    static constexpr const char* NAMES[8] = {"temperatureCA", "temperatureDA", "temperatureEA", "temperatureFA",
                                             "temperatureCB", "temperatureDB", "temperatureEB", "temperatureFB"};
    std::array<std::optional<double>, 8> points;
    bool anyGiven = false;
    for (int index = 0; index < 8; ++index)
    {
        points[static_cast<std::size_t>(index)] = reader.optionalReal(FIRST_POINT_FIELD + index, NAMES[index]);
        anyGiven = anyGiven || points[static_cast<std::size_t>(index)].has_value();
    }
    if (anyGiven)
    {
        for (std::size_t index = 0; index < 4; ++index)
        {
            card.pointsA[index] = points[index].value_or(card.temperatureA);
            card.pointsB[index] = points[index + 4].value_or(card.temperatureB);
        }
    }

    card.elements.push_back(ElementRun{first, first});
    for (const ElementRun& run : readElementRuns(reader, FIRST_MORE_ELEMENT_FIELD, "elemId"))
        card.elements.push_back(run);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> LineElemTemperatureLoadFormat::check(const LineElemTemperatureLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the temperature set ID must be a positive integer", "id"};
    if (card.elements.empty())
        return CardProblem{"card.missing-field", "the card needs at least one element", "elemId1"};
    if (card.elements.front().first != card.elements.front().last)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " is a single element: ranges go on the continuation lines", "elemId1"};
    // The point temperatures are all there or none (a blank one takes the temperature of its end).
    if (const std::size_t given = countGiven(card); given != 0 && given != 8)
        return CardProblem{"card.bad-value",
                           "the point temperatures " + std::string(CardProblem::FIELD) + " to " +
                               CardProblem::fieldNamed("temperatureFB") + " are all numbers, or all NaN (none)",
                           "temperatureCA"};
    return checkElementRuns(card.elements, "elemId1");
}

void LineElemTemperatureLoadFormat::write(const LineElemTemperatureLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.elements.empty() ? 0 : card.elements.front().first);
    writer.realUnlessDefault(3, card.temperatureA, 0.0);
    writer.realUnlessDefault(4, card.temperatureB, 0.0);
    writer.realUnlessDefault(5, card.gradientY[0], 0.0);
    writer.realUnlessDefault(6, card.gradientY[1], 0.0);
    writer.realUnlessDefault(7, card.gradientZ[0], 0.0);
    writer.realUnlessDefault(8, card.gradientZ[1], 0.0);
    if (card.hasPointTemperatures())
    {
        // Resolved from TA and TB: always written, so that a changed TA does not change them.
        for (std::size_t index = 0; index < 4; ++index)
        {
            writer.real(FIRST_POINT_FIELD + static_cast<int>(index), card.pointsA[index]);
            writer.real(FIRST_POINT_FIELD + 4 + static_cast<int>(index), card.pointsB[index]);
        }
    }
    if (card.elements.size() > 1)
    {
        const std::vector<ElementRun> more(card.elements.begin() + 1, card.elements.end());
        writeElementRuns(writer, FIRST_MORE_ELEMENT_FIELD, more, writer.compressesRanges());
    }
}

std::uint32_t LineElemTemperatureLoadTable::append(const LineElemTemperatureLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_temperatureA.push(card.temperatureA);
    m_temperatureB.push(card.temperatureB);
    m_gradientY.push(card.gradientY);
    m_gradientZ.push(card.gradientZ);
    m_pointsA.push(card.pointsA);
    m_pointsB.push(card.pointsB);
    m_runs.push(toPairs(card.elements));
    return row;
}

LineElemTemperatureLoadCard LineElemTemperatureLoadTable::card(std::uint32_t row) const
{
    LineElemTemperatureLoadCard card;
    card.id = m_id.get(row);
    card.temperatureA = m_temperatureA.get(row);
    card.temperatureB = m_temperatureB.get(row);
    card.gradientY = m_gradientY.get(row);
    card.gradientZ = m_gradientZ.get(row);
    card.pointsA = m_pointsA.get(row);
    card.pointsB = m_pointsB.get(row);
    card.elements = toRuns(m_runs.get(row));
    return card;
}

void LineElemTemperatureLoadTable::store(std::uint32_t row, const LineElemTemperatureLoadCard& card)
{
    m_id.set(row, card.id);
    m_temperatureA.set(row, card.temperatureA);
    m_temperatureB.set(row, card.temperatureB);
    m_gradientY.set(row, card.gradientY);
    m_gradientZ.set(row, card.gradientZ);
    m_pointsA.set(row, card.pointsA);
    m_pointsB.set(row, card.pointsB);
    m_runs.set(row, toPairs(card.elements));
}

void LineElemTemperatureLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_temperatureA.keepRows(rows);
    m_temperatureB.keepRows(rows);
    m_gradientY.keepRows(rows);
    m_gradientZ.keepRows(rows);
    m_pointsA.keepRows(rows);
    m_pointsB.keepRows(rows);
    m_runs.keepRows(rows);
}

void LineElemTemperatureLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitElementRuns(m_runs, row, visitor, "elemId1");
}

} // namespace oor::model
