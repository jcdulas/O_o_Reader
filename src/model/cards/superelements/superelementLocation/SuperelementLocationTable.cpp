#include "oor/model/cards/superelements/superelementLocation/SuperelementLocationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

// Three points are three different ones: a point given twice cannot make three noncolinear points.
std::optional<CardProblem> checkThreePoints(const std::array<base::EntityId, 3>& points, const char* field)
{
    for (const base::EntityId point : points)
    {
        if (const auto problem = checkRequiredId(point, field))
            return problem;
    }
    if (points[0] == points[1] || points[0] == points[2] || points[1] == points[2])
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be three different points",
                           field};
    return std::nullopt;
}

} // namespace

std::optional<SuperelementLocationCard> SuperelementLocationFormat::read(FieldReader& reader)
{
    SuperelementLocationCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    static constexpr const char* POINT_A_NAMES[] = {"pointA1", "pointA2", "pointA3"};
    static constexpr const char* POINT_B_NAMES[] = {"pointB1", "pointB2", "pointB3"};
    for (int index = 0; index < 3; ++index)
    {
        card.pointsA[static_cast<std::size_t>(index)] = reader.requiredInteger(2 + index, POINT_A_NAMES[index]);
        card.pointsB[static_cast<std::size_t>(index)] = reader.requiredInteger(5 + index, POINT_B_NAMES[index]);
    }
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementLocationFormat::check(const SuperelementLocationCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", false))
        return problem;
    if (const auto problem = checkThreePoints(card.pointsA, "pointA1"))
        return problem;
    return checkThreePoints(card.pointsB, "pointB1");
}

void SuperelementLocationFormat::write(const SuperelementLocationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    for (int index = 0; index < 3; ++index)
    {
        writer.integer(2 + index, card.pointsA[static_cast<std::size_t>(index)]);
        writer.integer(5 + index, card.pointsB[static_cast<std::size_t>(index)]);
    }
}

std::uint32_t SuperelementLocationTable::append(const SuperelementLocationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_pointsA.push(card.pointsA);
    m_pointsB.push(card.pointsB);
    return row;
}

SuperelementLocationCard SuperelementLocationTable::card(std::uint32_t row) const
{
    return SuperelementLocationCard{m_superelementId.get(row), m_pointsA.get(row), m_pointsB.get(row)};
}

void SuperelementLocationTable::store(std::uint32_t row, const SuperelementLocationCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_pointsA.set(row, card.pointsA);
    m_pointsB.set(row, card.pointsB);
}

void SuperelementLocationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_pointsA.keepRows(rows);
    m_pointsB.keepRows(rows);
}

void SuperelementLocationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementLocationCard card = this->card(row);
    visitSuperelement(visitor, "superelementId", card.superelementId);
    SuperelementScope scope(visitor, card.superelementId);
    for (base::EntityId& point : card.pointsA)
        visitId(scope, "pointA1", ReferenceTarget::Node, point);
    for (base::EntityId& point : card.pointsB)
        visitId(visitor, "pointB1", ReferenceTarget::Node, point);
    store(row, card);
}

} // namespace oor::model
