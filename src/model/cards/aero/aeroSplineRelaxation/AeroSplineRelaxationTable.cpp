#include "oor/model/cards/aero/aeroSplineRelaxation/AeroSplineRelaxationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5;

} // namespace

std::optional<AeroSplineRelaxationCard> AeroSplineRelaxationFormat::read(FieldReader& reader)
{
    AeroSplineRelaxationCard card;
    card.splineId1 = reader.requiredInteger(1, "splineId1");
    card.splineId2 = reader.requiredInteger(2, "splineId2");
    card.curvePointListId = reader.requiredInteger(3, "curvePointListId");
    card.referenceDistance = reader.requiredReal(4, "referenceDistance");
    card.relaxedPointListId = reader.integer(5, "relaxedPointListId", 0);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroSplineRelaxationFormat::check(const AeroSplineRelaxationCard& card)
{
    if (const auto problem = checkRequiredId(card.splineId1, "splineId1"))
        return problem;
    if (const auto problem = checkRequiredId(card.splineId2, "splineId2"))
        return problem;
    if (const auto problem = checkRequiredId(card.curvePointListId, "curvePointListId"))
        return problem;
    if (const auto problem = checkReal(card.referenceDistance, "referenceDistance", RealRule::Positive))
        return problem;
    return checkOptionalId(card.relaxedPointListId, "relaxedPointListId");
}

void AeroSplineRelaxationFormat::write(const AeroSplineRelaxationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.splineId1);
    writer.integer(2, card.splineId2);
    writer.integer(3, card.curvePointListId);
    writer.real(4, card.referenceDistance);
    writer.integerUnlessDefault(5, card.relaxedPointListId, 0);
}

std::uint32_t AeroSplineRelaxationTable::append(const AeroSplineRelaxationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_splineId1.push(card.splineId1);
    m_cards.push(card);
    return row;
}

void AeroSplineRelaxationTable::store(std::uint32_t row, const AeroSplineRelaxationCard& card)
{
    m_splineId1.set(row, card.splineId1);
    m_cards.set(row, card);
}

void AeroSplineRelaxationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_splineId1.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroSplineRelaxationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroSplineRelaxationCard& card = m_cards.edit(row);
    visitId(visitor, "splineId1", ReferenceTarget::AeroSpline, card.splineId1);
    m_splineId1.set(row, card.splineId1); // the column follows a renumbering of the splines
    visitId(visitor, "splineId2", ReferenceTarget::AeroSpline, card.splineId2);
    visitId(visitor, "curvePointListId", ReferenceTarget::AeroList, card.curvePointListId);
    visitId(visitor, "relaxedPointListId", ReferenceTarget::AeroList, card.relaxedPointListId);
}

} // namespace oor::model
