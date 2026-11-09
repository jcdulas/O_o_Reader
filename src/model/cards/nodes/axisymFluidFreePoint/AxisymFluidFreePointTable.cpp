#include "oor/model/cards/nodes/axisymFluidFreePoint/AxisymFluidFreePointTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int MAX_POINTS = 3;
constexpr int PAIR_FIELDS = 2;
constexpr int FIRST_PAIR_FIELD = 2;         // the layout of the format: IDP1 right after the IDF
constexpr int SHIFTED_FIRST_PAIR_FIELD = 3; // the layout of the d07d2 deck: a blank field 2 first

} // namespace

std::vector<AxisymFluidFreePointCard> AxisymFluidFreePointFormat::readAll(FieldReader& reader)
{
    std::vector<AxisymFluidFreePointCard> points;
    const base::EntityId fluidPoint = reader.requiredInteger(1, "fluidPointId");
    // A blank IDP1 is not valid in the layout of the format, so a blank field 2 can only be the other layout.
    const int firstField = reader.isBlank(FIRST_PAIR_FIELD) ? SHIFTED_FIRST_PAIR_FIELD : FIRST_PAIR_FIELD;
    const int lastField = firstField + MAX_POINTS * PAIR_FIELDS - 1;
    for (int index = 0; index < MAX_POINTS && !reader.failed(); ++index)
    {
        const int field = firstField + index * PAIR_FIELDS;
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        AxisymFluidFreePointCard card;
        card.fluidPointId = fluidPoint;
        card.pointId = reader.requiredInteger(field, "pointId");
        card.phiAngle = reader.requiredReal(field + 1, "phiAngle");
        points.push_back(card);
    }
    if (reader.lastUsedField() > lastField)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " defines at most three free surface points", "phiAngle3");
    if (!reader.failed() && points.empty())
        reader.error("card.missing-field",
                     "a " + std::string(reader.cardName()) + " needs at least one point " + std::string("pointId1") +
                         " " + std::string("phiAngle1"),
                     "pointId1");
    if (!reader.failed())
    {
        for (const AxisymFluidFreePointCard& card : points)
        {
            if (const auto problem = check(card))
            {
                reader.report(problem);
                break;
            }
        }
    }
    return points;
}

std::optional<CardProblem> AxisymFluidFreePointFormat::check(const AxisymFluidFreePointCard& card)
{
    if (const auto problem = checkRequiredId(card.fluidPointId, "fluidPointId"))
        return problem;
    if (const auto problem = checkRequiredId(card.pointId, "pointId"))
        return problem;
    return checkReal(card.phiAngle, "phiAngle", RealRule::Finite);
}

void AxisymFluidFreePointFormat::write(const AxisymFluidFreePointCard& card, FieldWriter& writer)
{
    writer.integer(1, card.fluidPointId);
    writer.integer(2, card.pointId);
    writer.real(3, card.phiAngle);
}

std::uint32_t AxisymFluidFreePointTable::append(const AxisymFluidFreePointCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_fluidPointId.push(card.fluidPointId);
    m_pointId.push(card.pointId);
    m_phiAngle.push(card.phiAngle);
    return row;
}

AxisymFluidFreePointCard AxisymFluidFreePointTable::card(std::uint32_t row) const
{
    return AxisymFluidFreePointCard{m_fluidPointId.get(row), m_pointId.get(row), m_phiAngle.get(row)};
}

void AxisymFluidFreePointTable::store(std::uint32_t row, const AxisymFluidFreePointCard& card)
{
    m_fluidPointId.set(row, card.fluidPointId);
    m_pointId.set(row, card.pointId);
    m_phiAngle.set(row, card.phiAngle);
}

void AxisymFluidFreePointTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_fluidPointId.keepRows(rows);
    m_pointId.keepRows(rows);
    m_phiAngle.keepRows(rows);
}

void AxisymFluidFreePointTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The IDF names a RINGFL fluid point, which is a point of the node ID space.
    if (const base::EntityId fluidPoint = m_fluidPointId.get(row); fluidPoint > 0)
        m_fluidPointId.set(row, visitor.visit("fluidPointId", ReferenceTarget::Node, fluidPoint));
}

} // namespace oor::model
