#include "oor/model/cards/superelements/superelementMirrorPlane/SuperelementMirrorPlaneTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

std::optional<SuperelementMirrorPlaneCard> SuperelementMirrorPlaneFormat::read(FieldReader& reader)
{
    SuperelementMirrorPlaneCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    if (!(reader.text(2) == "Plane"))
        reader.error(
            "card.bad-value",
            "the second field of an " + std::string(reader.cardName()) + " is the word " + std::string("plane"),
            "plane");
    static constexpr const char* NAMES[] = {"point1", "point2", "point3"};
    for (int index = 0; index < 3; ++index)
        card.points[static_cast<std::size_t>(index)] = reader.requiredInteger(3 + index, NAMES[index]);
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementMirrorPlaneFormat::check(const SuperelementMirrorPlaneCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", false))
        return problem;
    for (const base::EntityId point : card.points)
    {
        if (const auto problem = checkRequiredId(point, "point"))
            return problem;
    }
    // Three noncolinear points are three different ones.
    if (card.points[0] == card.points[1] || card.points[0] == card.points[2] || card.points[1] == card.points[2])
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("point1") + ", " + CardProblem::fieldNamed("point2") + " and " +
                               CardProblem::fieldNamed("point3") + " must be three different points",
                           "point"};
    return std::nullopt;
}

void SuperelementMirrorPlaneFormat::write(const SuperelementMirrorPlaneCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writer.text(2, "Plane");
    for (int index = 0; index < 3; ++index)
        writer.integer(3 + index, card.points[static_cast<std::size_t>(index)]);
}

std::uint32_t SuperelementMirrorPlaneTable::append(const SuperelementMirrorPlaneCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_points.push(card.points);
    return row;
}

SuperelementMirrorPlaneCard SuperelementMirrorPlaneTable::card(std::uint32_t row) const
{
    return SuperelementMirrorPlaneCard{m_superelementId.get(row), m_points.get(row)};
}

void SuperelementMirrorPlaneTable::store(std::uint32_t row, const SuperelementMirrorPlaneCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_points.set(row, card.points);
}

void SuperelementMirrorPlaneTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_points.keepRows(rows);
}

void SuperelementMirrorPlaneTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementMirrorPlaneCard card = this->card(row);
    visitSuperelement(visitor, "superelementId", card.superelementId);
    for (base::EntityId& point : card.points)
        visitId(visitor, "point", ReferenceTarget::Node, point);
    store(row, card);
}

} // namespace oor::model
