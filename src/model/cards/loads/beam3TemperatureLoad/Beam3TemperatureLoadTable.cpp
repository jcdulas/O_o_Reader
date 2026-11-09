#include "oor/model/cards/loads/beam3TemperatureLoad/Beam3TemperatureLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>
#include <cstddef>

namespace oor::model
{

namespace
{

constexpr int FIRST_TEMPERATURE_FIELD = 3; // T(A), T(B), T(C)
constexpr int FIRST_GRADIENT_FIELD = 6;    // TPY(A), TPZ(A), TPY(B), TPZ(B), TPY(C), TPZ(C)
constexpr int FIRST_POINT_FIELD = 12;      // TC(A), TD(A), TE(A), TF(A), TC(B), ..., TF(C)
constexpr int UNUSED_FIELD = 24;           // the last cell of the third line: blank
constexpr int FIRST_ELEMENT_FIELD = 25;
constexpr std::size_t STATIONS = 3;
constexpr std::size_t POINT_KINDS = 4; // C, D, E, F
constexpr std::size_t POINT_COUNT = STATIONS * POINT_KINDS;

constexpr const char* TEMPERATURE_NAMES[STATIONS] = {"temperatureA", "temperatureB", "temperatureC"};
constexpr const char* GRADIENT_Y_NAMES[STATIONS] = {"gradientYA", "gradientYB", "gradientYC"};
constexpr const char* GRADIENT_Z_NAMES[STATIONS] = {"gradientZA", "gradientZB", "gradientZC"};
// The point temperatures in the order of the card: the four points of station A, then those of B, then those of C.
constexpr const char* POINT_NAMES[POINT_COUNT] = {"temperatureCA", "temperatureDA", "temperatureEA", "temperatureFA",
                                                  "temperatureCB", "temperatureDB", "temperatureEB", "temperatureFB",
                                                  "temperatureCC", "temperatureDC", "temperatureEC", "temperatureFC"};

// The point temperature number `index` in the order of the card (station A: C, D, E, F; then B; then C) and its setter.
double pointOf(const Beam3TemperatureLoadCard& card, std::size_t index)
{
    const std::size_t station = index / POINT_KINDS;
    switch (index % POINT_KINDS)
    {
    case 0:
        return card.temperatureC[station];
    case 1:
        return card.temperatureD[station];
    case 2:
        return card.temperatureE[station];
    default:
        return card.temperatureF[station];
    }
}

void setPoint(Beam3TemperatureLoadCard& card, std::size_t index, double value)
{
    const std::size_t station = index / POINT_KINDS;
    switch (index % POINT_KINDS)
    {
    case 0:
        card.temperatureC[station] = value;
        break;
    case 1:
        card.temperatureD[station] = value;
        break;
    case 2:
        card.temperatureE[station] = value;
        break;
    default:
        card.temperatureF[station] = value;
        break;
    }
}

} // namespace

std::optional<Beam3TemperatureLoadCard> Beam3TemperatureLoadFormat::read(FieldReader& reader)
{
    Beam3TemperatureLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.elemId = reader.requiredInteger(2, "elemId");
    for (std::size_t station = 0; station < STATIONS; ++station)
    {
        const int offset = static_cast<int>(station);
        card.temperature[station] = reader.real(FIRST_TEMPERATURE_FIELD + offset, TEMPERATURE_NAMES[station], 0.0);
        card.gradientY[station] = reader.real(FIRST_GRADIENT_FIELD + 2 * offset, GRADIENT_Y_NAMES[station], 0.0);
        card.gradientZ[station] = reader.real(FIRST_GRADIENT_FIELD + 2 * offset + 1, GRADIENT_Z_NAMES[station], 0.0);
    }

    // with no point temperature the card has none (linear gradients); with some, a blank one is 0.0.
    std::array<std::optional<double>, POINT_COUNT> points;
    bool anyGiven = false;
    for (std::size_t index = 0; index < POINT_COUNT; ++index)
    {
        points[index] = reader.optionalReal(FIRST_POINT_FIELD + static_cast<int>(index), POINT_NAMES[index]);
        anyGiven = anyGiven || points[index].has_value();
    }
    if (anyGiven)
    {
        for (std::size_t index = 0; index < POINT_COUNT; ++index)
            setPoint(card, index, points[index].value_or(0.0));
    }

    if (!reader.isBlank(UNUSED_FIELD))
        reader.error("card.bad-value", "field 24 of the card is not used and must be blank", "");
    card.elements = readThermalRuns(reader, FIRST_ELEMENT_FIELD, "elemId", true);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> Beam3TemperatureLoadFormat::check(const Beam3TemperatureLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.elemId, "elemId"))
        return problem;
    for (std::size_t station = 0; station < STATIONS; ++station)
    {
        if (const auto problem = checkReal(card.temperature[station], TEMPERATURE_NAMES[station], RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(card.gradientY[station], GRADIENT_Y_NAMES[station], RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(card.gradientZ[station], GRADIENT_Z_NAMES[station], RealRule::Finite))
            return problem;
    }
    // The point temperatures are all numbers or none (a blank one is 0.0 once some are given).
    std::size_t given = 0;
    for (std::size_t index = 0; index < POINT_COUNT; ++index)
        given += std::isnan(pointOf(card, index)) ? 0 : 1;
    if (given != 0 && given != POINT_COUNT)
        return CardProblem{"card.bad-value",
                           "the point temperatures " + std::string(CardProblem::FIELD) + " to " +
                               CardProblem::fieldNamed("temperatureFC") + " are all numbers, or all NaN (none)",
                           "temperatureCA"};
    if (given == POINT_COUNT)
    {
        for (std::size_t index = 0; index < POINT_COUNT; ++index)
        {
            if (const auto problem = checkReal(pointOf(card, index), POINT_NAMES[index], RealRule::Finite))
                return problem;
        }
    }
    return checkThermalRuns(card.elements, "elemId", false);
}

void Beam3TemperatureLoadFormat::write(const Beam3TemperatureLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.elemId);
    for (std::size_t station = 0; station < STATIONS; ++station)
    {
        const int offset = static_cast<int>(station);
        writer.realUnlessDefault(FIRST_TEMPERATURE_FIELD + offset, card.temperature[station], 0.0);
        writer.realUnlessDefault(FIRST_GRADIENT_FIELD + 2 * offset, card.gradientY[station], 0.0);
        writer.realUnlessDefault(FIRST_GRADIENT_FIELD + 2 * offset + 1, card.gradientZ[station], 0.0);
    }
    if (card.hasPointTemperatures())
    {
        // All twelve, zeros included: a deck that gives one of them has the other eleven as 0.0, and a card that gives
        // none has no point temperatures at all.
        for (std::size_t index = 0; index < POINT_COUNT; ++index)
            writer.real(FIRST_POINT_FIELD + static_cast<int>(index), pointOf(card, index));
    }
    writeThermalRuns(writer, FIRST_ELEMENT_FIELD, card.elements);
}

std::uint32_t Beam3TemperatureLoadTable::append(const Beam3TemperatureLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_elemId.push(card.elemId);
    m_temperature.push(card.temperature);
    m_gradientY.push(card.gradientY);
    m_gradientZ.push(card.gradientZ);
    m_temperatureC.push(card.temperatureC);
    m_temperatureD.push(card.temperatureD);
    m_temperatureE.push(card.temperatureE);
    m_temperatureF.push(card.temperatureF);
    m_runs.push(toThermalTriples(card.elements));
    return row;
}

Beam3TemperatureLoadCard Beam3TemperatureLoadTable::card(std::uint32_t row) const
{
    Beam3TemperatureLoadCard card;
    card.id = m_id.get(row);
    card.elemId = m_elemId.get(row);
    card.temperature = m_temperature.get(row);
    card.gradientY = m_gradientY.get(row);
    card.gradientZ = m_gradientZ.get(row);
    card.temperatureC = m_temperatureC.get(row);
    card.temperatureD = m_temperatureD.get(row);
    card.temperatureE = m_temperatureE.get(row);
    card.temperatureF = m_temperatureF.get(row);
    card.elements = toThermalRuns(m_runs.get(row));
    return card;
}

void Beam3TemperatureLoadTable::store(std::uint32_t row, const Beam3TemperatureLoadCard& card)
{
    m_id.set(row, card.id);
    m_elemId.set(row, card.elemId);
    m_temperature.set(row, card.temperature);
    m_gradientY.set(row, card.gradientY);
    m_gradientZ.set(row, card.gradientZ);
    m_temperatureC.set(row, card.temperatureC);
    m_temperatureD.set(row, card.temperatureD);
    m_temperatureE.set(row, card.temperatureE);
    m_temperatureF.set(row, card.temperatureF);
    m_runs.set(row, toThermalTriples(card.elements));
}

void Beam3TemperatureLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_elemId.keepRows(rows);
    m_temperature.keepRows(rows);
    m_gradientY.keepRows(rows);
    m_gradientZ.keepRows(rows);
    m_temperatureC.keepRows(rows);
    m_temperatureD.keepRows(rows);
    m_temperatureE.keepRows(rows);
    m_temperatureF.keepRows(rows);
    m_runs.keepRows(rows);
}

void Beam3TemperatureLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId elemId = m_elemId.get(row); elemId > 0)
        m_elemId.set(row, visitor.visit("elemId", ReferenceTarget::Element, elemId));
    visitThermalRuns(m_runs, row, visitor, "elements", ReferenceTarget::Element, ReferenceTarget::ElementInRange);
}

} // namespace oor::model
