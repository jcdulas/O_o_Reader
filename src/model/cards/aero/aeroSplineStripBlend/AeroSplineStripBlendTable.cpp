#include "oor/model/cards/aero/aeroSplineStripBlend/AeroSplineStripBlendTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 12;

// True when the direction vector has a component that is not zero (a blank component is zero).
bool hasDirection(const AeroSplineStripBlendCard& card)
{
    return (card.directionX && *card.directionX != 0.0) || (card.directionY && *card.directionY != 0.0) ||
           (card.directionZ && *card.directionZ != 0.0);
}

} // namespace

std::optional<AeroSplineStripBlendCard> AeroSplineStripBlendFormat::read(FieldReader& reader)
{
    AeroSplineStripBlendCard card;
    card.id = reader.requiredInteger(1, "id");
    card.splineId1 = reader.requiredInteger(2, "splineId1");
    card.splineId2 = reader.requiredInteger(3, "splineId2");
    card.option = readKeyword<AeroSplineStripBlendOption>(reader, 4, "option", AERO_SPLINE_STRIP_BLEND_OPTION_WORDS,
                                                          AeroSplineStripBlendOption::WeightedAverage);
    card.weight1 = reader.real(5, "weight1", AeroSplineStripBlendCard::DEFAULT_WEIGHT);
    card.nodeId = reader.integer(6, "nodeId", 0);
    card.depth1 = reader.optionalReal(7, "depth1");
    card.depth2 = reader.optionalReal(8, "depth2");
    card.directionX = reader.optionalReal(9, "directionX");
    card.directionY = reader.optionalReal(10, "directionY");
    card.directionZ = reader.optionalReal(11, "directionZ");
    card.frameId = reader.integer(12, "frameId", 0);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroSplineStripBlendFormat::check(const AeroSplineStripBlendCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.splineId1, "splineId1"))
        return problem;
    if (const auto problem = checkRequiredId(card.splineId2, "splineId2"))
        return problem;
    // 0.0 < W1 <= 1.0, whatever OPT is: a weight outside the range is never meant.
    if (!(card.weight1 > 0.0 && card.weight1 <= 1.0))
        return CardProblem{"card.bad-value", "{field} must be greater than 0.0 and at most 1.0", "weight1"};
    if (const auto problem = checkOptionalId(card.nodeId, "nodeId"))
        return problem;
    if (const auto problem = checkOptionalReal(card.depth1, "depth1", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.depth2, "depth2", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.directionX, "directionX", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.directionY, "directionY", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.directionZ, "directionZ", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.frameId, "frameId"))
        return problem;
    // Only WAVG can do without the reference grid, the depths and the direction.
    if (card.option != AeroSplineStripBlendOption::WeightedAverage)
    {
        if (card.nodeId == 0)
            return CardProblem{"card.missing-field", "{field} is required unless {field:option} is WAVG", "nodeId"};
        if (!card.depth1)
            return CardProblem{"card.missing-field", "{field} is required unless {field:option} is WAVG", "depth1"};
        if (!card.depth2)
            return CardProblem{"card.missing-field", "{field} is required unless {field:option} is WAVG", "depth2"};
        if (!hasDirection(card))
            return CardProblem{"card.missing-field",
                               "the direction vector {field}, {field:directionY}, {field:directionZ} is required "
                               "unless {field:option} is WAVG",
                               "directionX"};
    }
    return std::nullopt;
}

void AeroSplineStripBlendFormat::write(const AeroSplineStripBlendCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.splineId1);
    writer.integer(3, card.splineId2);
    writeKeywordUnlessDefault(writer, 4, AERO_SPLINE_STRIP_BLEND_OPTION_WORDS, card.option,
                              AeroSplineStripBlendOption::WeightedAverage);
    writer.realUnlessDefault(5, card.weight1, AeroSplineStripBlendCard::DEFAULT_WEIGHT);
    writer.integerUnlessDefault(6, card.nodeId, 0);
    if (card.depth1)
        writer.real(7, *card.depth1);
    if (card.depth2)
        writer.real(8, *card.depth2);
    if (card.directionX)
        writer.real(9, *card.directionX);
    if (card.directionY)
        writer.real(10, *card.directionY);
    if (card.directionZ)
        writer.real(11, *card.directionZ);
    writer.integerUnlessDefault(12, card.frameId, 0);
}

std::uint32_t AeroSplineStripBlendTable::append(const AeroSplineStripBlendCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroSplineStripBlendTable::store(std::uint32_t row, const AeroSplineStripBlendCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroSplineStripBlendTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroSplineStripBlendTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroSplineStripBlendTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroSplineStripBlendCard& card = m_cards.edit(row);
    visitId(visitor, "splineId1", ReferenceTarget::AeroSpline, card.splineId1);
    visitId(visitor, "splineId2", ReferenceTarget::AeroSpline, card.splineId2);
    visitId(visitor, "nodeId", ReferenceTarget::AeroNode, card.nodeId);
    visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.frameId);
}

} // namespace oor::model
