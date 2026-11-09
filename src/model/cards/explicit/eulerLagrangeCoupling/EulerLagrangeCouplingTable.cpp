#include "oor/model/cards/explicit/eulerLagrangeCoupling/EulerLagrangeCouplingTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<EulerLagrangeCouplingCard> EulerLagrangeCouplingFormat::read(FieldReader& reader)
{
    EulerLagrangeCouplingCard card;
    card.id = reader.requiredInteger(1, "id");
    card.surfaceId = reader.requiredInteger(2, "surfaceId");
    card.cover = readKeyword(reader, 3, "cover", COVER_SIDE_WORDS, CoverSide::Inside);
    card.reverseNormals = readKeyword(reader, 4, "reverseNormals", SWITCH_WORDS, Switch::On);
    card.checkNormals = readKeyword(reader, 5, "checkNormals", SWITCH_WORDS, Switch::On);
    card.porosityId = reader.integer(6, "porosityId", 0);
    card.optionId = reader.integer(7, "optionId", 0);
    card.couplingType =
        readKeyword(reader, 8, "couplingType", EULER_LAGRANGE_COUPLING_TYPE_WORDS, EulerLagrangeCouplingType::Standard);
    card.inflatorId = reader.integer(9, "inflatorId", 0);
    card.heatTransferId = reader.integer(10, "heatTransferId", 0);
    card.staticFriction = reader.real(11, "staticFriction", 0.0);
    card.kineticFriction = reader.real(12, "kineticFriction", 0.0);
    card.frictionDecay = reader.real(13, "frictionDecay", 0.0);
    card.initialGasId = reader.integer(14, "initialGasId", 0);
    card.elemSetId = reader.integer(17, "elemSetId", 0);
    card.meshId = reader.integer(18, "meshId", 0);
    card.deactivationTime = reader.real(19, "deactivationTime", EULER_LAGRANGE_COUPLING_DEFAULT_DEACTIVATION_TIME);
    card.failedSegmentFlowId = reader.integer(20, "failedSegmentFlowId", 0);
    card.hydrostaticId = reader.integer(21, "hydrostaticId", 0);
    card.skinFriction = reader.real(22, "skinFriction", 0.0);
    card.cohesiveFrictionSetId = reader.integer(23, "cohesiveFrictionSetId", 0);
    // Fields 15 and 16 are empty cells of the grid, and nothing follows field 23.
    requireOnlyFields(reader, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, 21, 22, 23});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerLagrangeCouplingFormat::check(const EulerLagrangeCouplingCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.surfaceId, "surfaceId"))
        return problem;
    if (const auto problem = checkOptionalId(card.porosityId, "porosityId"))
        return problem;
    if (const auto problem = checkOptionalId(card.optionId, "optionId"))
        return problem;
    if (const auto problem = checkOptionalId(card.inflatorId, "inflatorId"))
        return problem;
    if (const auto problem = checkOptionalId(card.heatTransferId, "heatTransferId"))
        return problem;
    if (const auto problem = checkReal(card.staticFriction, "staticFriction", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.kineticFriction, "kineticFriction", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.frictionDecay, "frictionDecay", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalId(card.initialGasId, "initialGasId"))
        return problem;
    if (const auto problem = checkOptionalId(card.elemSetId, "elemSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.meshId, "meshId"))
        return problem;
    // An Euler domain is tied to the surface by its elements or by its mesh, not by both.
    if (card.elemSetId > 0 && card.meshId > 0)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("elemSetId") + " and " + std::string(CardProblem::FIELD) +
                               " cannot both be given: give one of the two",
                           "meshId"};
    if (const auto problem = checkReal(card.deactivationTime, "deactivationTime", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalId(card.failedSegmentFlowId, "failedSegmentFlowId"))
        return problem;
    if (const auto problem = checkOptionalId(card.hydrostaticId, "hydrostaticId"))
        return problem;
    if (const auto problem = checkReal(card.skinFriction, "skinFriction", RealRule::NonNegative))
        return problem;
    return checkOptionalId(card.cohesiveFrictionSetId, "cohesiveFrictionSetId");
}

void EulerLagrangeCouplingFormat::write(const EulerLagrangeCouplingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.surfaceId);
    writeKeywordUnlessDefault(writer, 3, COVER_SIDE_WORDS, card.cover, CoverSide::Inside);
    writeKeywordUnlessDefault(writer, 4, SWITCH_WORDS, card.reverseNormals, Switch::On);
    writeKeywordUnlessDefault(writer, 5, SWITCH_WORDS, card.checkNormals, Switch::On);
    writer.integerUnlessDefault(6, card.porosityId, 0);
    writer.integerUnlessDefault(7, card.optionId, 0);
    writeKeywordUnlessDefault(writer, 8, EULER_LAGRANGE_COUPLING_TYPE_WORDS, card.couplingType,
                              EulerLagrangeCouplingType::Standard);
    writer.integerUnlessDefault(9, card.inflatorId, 0);
    writer.integerUnlessDefault(10, card.heatTransferId, 0);
    writer.realUnlessDefault(11, card.staticFriction, 0.0);
    writer.realUnlessDefault(12, card.kineticFriction, 0.0);
    writer.realUnlessDefault(13, card.frictionDecay, 0.0);
    writer.integerUnlessDefault(14, card.initialGasId, 0);
    writer.integerUnlessDefault(17, card.elemSetId, 0);
    writer.integerUnlessDefault(18, card.meshId, 0);
    writer.realUnlessDefault(19, card.deactivationTime, EULER_LAGRANGE_COUPLING_DEFAULT_DEACTIVATION_TIME);
    writer.integerUnlessDefault(20, card.failedSegmentFlowId, 0);
    writer.integerUnlessDefault(21, card.hydrostaticId, 0);
    writer.realUnlessDefault(22, card.skinFriction, 0.0);
    writer.integerUnlessDefault(23, card.cohesiveFrictionSetId, 0);
}

std::uint32_t EulerLagrangeCouplingTable::append(const EulerLagrangeCouplingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerLagrangeCouplingTable::store(std::uint32_t row, const EulerLagrangeCouplingCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerLagrangeCouplingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerLagrangeCouplingTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerLagrangeCouplingTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerLagrangeCouplingCard& card = m_cards.edit(row);
    visitId(visitor, "surfaceId", ReferenceTarget::ContactSurface, card.surfaceId);
    visitId(visitor, "porosityId", ReferenceTarget::Leakage, card.porosityId);
    visitId(visitor, "optionId", ReferenceTarget::CouplingOptions, card.optionId);
    visitId(visitor, "inflatorId", ReferenceTarget::InflatorSet, card.inflatorId);
    visitId(visitor, "heatTransferId", ReferenceTarget::HeatLossSet, card.heatTransferId);
    visitId(visitor, "initialGasId", ReferenceTarget::InitialGas, card.initialGasId);
    visitId(visitor, "elemSetId", ReferenceTarget::Set, card.elemSetId);
    visitId(visitor, "meshId", ReferenceTarget::EulerMesh, card.meshId);
    visitId(visitor, "failedSegmentFlowId", ReferenceTarget::CouplingFailure, card.failedSegmentFlowId);
    visitId(visitor, "hydrostaticId", ReferenceTarget::HydrostaticPreset, card.hydrostaticId);
    visitId(visitor, "cohesiveFrictionSetId", ReferenceTarget::CohesiveFrictionSet, card.cohesiveFrictionSetId);
}

} // namespace oor::model
