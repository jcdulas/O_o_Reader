#include "oor/model/cards/nodes/axisymFluidRing/AxisymFluidRingTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr base::EntityId FLUID_POINT_LIMIT = 500000; // an IDF is below it (0 < Integer < 500000)
constexpr int SECOND_POINT_FIELD = 5;
constexpr int LAST_FIELD = 8;

// The fluid point whose four fields start at `first` (1 for the first point, 5 for the second).
AxisymFluidRingCard readPoint(FieldReader& reader, int first, const char* const (&names)[4])
{
    AxisymFluidRingCard card;
    card.id = reader.requiredInteger(first, names[0]);
    card.position1 = reader.requiredReal(first + 1, names[1]);
    card.position2 = reader.real(first + 2, names[2], 0.0); // blank or zero for a cylindrical system
    card.position3 = reader.real(first + 3, names[3], 0.0);
    if (!reader.failed())
        reader.report(AxisymFluidRingFormat::check(card));
    return card;
}

} // namespace

std::vector<AxisymFluidRingCard> AxisymFluidRingFormat::readAll(FieldReader& reader)
{
    static constexpr const char* NAMES_FIRST[4] = {"firstId", "firstPosition1", "firstPosition2", "firstPosition3"};
    static constexpr const char* NAMES_SECOND[4] = {"secondId", "secondPosition1", "secondPosition2",
                                                    "secondPosition3"};
    std::vector<AxisymFluidRingCard> points;
    points.push_back(readPoint(reader, 1, NAMES_FIRST));
    // The second point is there as soon as one of its fields is, and its IDFB and XB1 are then required.
    bool hasSecond = false;
    for (int field = SECOND_POINT_FIELD; field <= LAST_FIELD; ++field)
        hasSecond = hasSecond || !reader.isBlank(field);
    if (hasSecond)
        points.push_back(readPoint(reader, SECOND_POINT_FIELD, NAMES_SECOND));
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " defines at most two fluid points",
                     "secondPosition3");
    return points;
}

std::optional<CardProblem> AxisymFluidRingFormat::check(const AxisymFluidRingCard& card)
{
    if (card.id <= 0 || card.id >= FLUID_POINT_LIMIT)
        return CardProblem{"card.bad-id", "{field} must be an integer from 1 to 499999", "id"};
    if (const auto problem = checkReal(card.position1, "position1", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.position2, "position2", RealRule::Finite))
        return problem;
    return checkReal(card.position3, "position3", RealRule::Finite);
}

void AxisymFluidRingFormat::write(const AxisymFluidRingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.position1);
    writer.realUnlessDefault(3, card.position2, 0.0);
    writer.realUnlessDefault(4, card.position3, 0.0);
}

std::uint32_t AxisymFluidRingTable::append(const AxisymFluidRingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_position1.push(card.position1);
    m_position2.push(card.position2);
    m_position3.push(card.position3);
    return row;
}

AxisymFluidRingCard AxisymFluidRingTable::card(std::uint32_t row) const
{
    return AxisymFluidRingCard{m_id.get(row), m_position1.get(row), m_position2.get(row), m_position3.get(row)};
}

void AxisymFluidRingTable::store(std::uint32_t row, const AxisymFluidRingCard& card)
{
    m_id.set(row, card.id);
    m_position1.set(row, card.position1);
    m_position2.set(row, card.position2);
    m_position3.set(row, card.position3);
}

void AxisymFluidRingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_position1.keepRows(rows);
    m_position2.keepRows(rows);
    m_position3.keepRows(rows);
}

} // namespace oor::model
