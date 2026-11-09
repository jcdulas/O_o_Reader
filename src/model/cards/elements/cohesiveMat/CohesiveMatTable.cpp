#include "oor/model/cards/elements/cohesiveMat/CohesiveMatTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_TABLE_ID = 4;
constexpr int FIELD_COHESIVE_ENERGY = 9;
constexpr int FIELD_CRITICAL_OPENING = 10;
constexpr int FIELD_MAX_OPENING = 11;
constexpr int FIELD_SHEAR_NORMAL_STRESS_RATIO = 12;
constexpr int FIELD_EXPONENT = 13;
constexpr int FIELD_VISCOUS_DISSIPATION = 14;
constexpr int FIELD_REFERENCE_DISPLACEMENT_RATE = 15;
constexpr int FIELD_COMPRESSION_STIFFENING = 16;
constexpr int FIELD_SHEAR_NORMAL_ENERGY_RATIO = 17;
constexpr int FIELD_IN_PLANE_STIFFNESS = 19;
constexpr int FIELD_NORMAL_STIFFNESS = 20;

bool isKnownModel(int model)
{
    return model == COHESIVE_MAT_MODEL_BILINEAR || model == COHESIVE_MAT_MODEL_EXPONENTIAL ||
           model == COHESIVE_MAT_MODEL_LINEAR_EXPONENTIAL || model == COHESIVE_MAT_MODEL_USER ||
           model == COHESIVE_MAT_MODEL_LINEAR;
}

} // namespace

std::optional<CohesiveMatCard> CohesiveMatFormat::read(FieldReader& reader)
{
    CohesiveMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.cohesiveModel = reader.integer(2, "cohesiveModel", COHESIVE_MAT_MODEL_BILINEAR);
    card.tableId = reader.integer(FIELD_TABLE_ID, "tableId", 0);
    card.cohesiveEnergy = reader.optionalReal(FIELD_COHESIVE_ENERGY, "cohesiveEnergy");
    card.criticalOpening = reader.optionalReal(FIELD_CRITICAL_OPENING, "criticalOpening");
    card.maxOpening = reader.optionalReal(FIELD_MAX_OPENING, "maxOpening");
    card.shearNormalStressRatio = reader.real(FIELD_SHEAR_NORMAL_STRESS_RATIO, "shearNormalStressRatio", 1.0);
    card.exponent = reader.real(FIELD_EXPONENT, "exponent", 1.0);
    card.viscousDissipation = reader.real(FIELD_VISCOUS_DISSIPATION, "viscousDissipation", 0.0);
    card.referenceDisplacementRate = reader.real(FIELD_REFERENCE_DISPLACEMENT_RATE, "referenceDisplacementRate", 0.0);
    card.compressionStiffening = reader.real(FIELD_COMPRESSION_STIFFENING, "compressionStiffening", 1.0);
    card.shearNormalEnergyRatio = reader.real(FIELD_SHEAR_NORMAL_ENERGY_RATIO, "shearNormalEnergyRatio", 1.0);
    card.inPlaneStiffness = reader.real(FIELD_IN_PLANE_STIFFNESS, "inPlaneStiffness", 0.0);
    card.normalStiffness = reader.real(FIELD_NORMAL_STIFFNESS, "normalStiffness", 0.0);
    requireBlankFields(reader, {3, 5, 6, 7, 8, 18}, FIELD_NORMAL_STIFFNESS);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CohesiveMatFormat::check(const CohesiveMatCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (!isKnownModel(card.cohesiveModel))
        return CardProblem{"card.bad-value", "{field} must be 1, 2, 3, -1 or -2", "cohesiveModel"};
    if (const auto problem = checkOptionalId(card.tableId, "tableId"))
        return problem;
    if (const auto problem = checkOptionalReal(card.cohesiveEnergy, "cohesiveEnergy", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalReal(card.criticalOpening, "criticalOpening", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalReal(card.maxOpening, "maxOpening", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.shearNormalStressRatio, "shearNormalStressRatio", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.exponent, "exponent", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.viscousDissipation, "viscousDissipation", RealRule::NonNegative))
        return problem;
    if (const auto problem =
            checkReal(card.referenceDisplacementRate, "referenceDisplacementRate", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.compressionStiffening, "compressionStiffening", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.shearNormalEnergyRatio, "shearNormalEnergyRatio", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.inPlaneStiffness, "inPlaneStiffness", RealRule::NonNegative))
        return problem;
    return checkReal(card.normalStiffness, "normalStiffness", RealRule::NonNegative);
}

void CohesiveMatFormat::write(const CohesiveMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.cohesiveModel, COHESIVE_MAT_MODEL_BILINEAR);
    writer.integerUnlessDefault(FIELD_TABLE_ID, card.tableId, 0);
    if (card.cohesiveEnergy)
        writer.real(FIELD_COHESIVE_ENERGY, *card.cohesiveEnergy);
    if (card.criticalOpening)
        writer.real(FIELD_CRITICAL_OPENING, *card.criticalOpening);
    if (card.maxOpening)
        writer.real(FIELD_MAX_OPENING, *card.maxOpening);
    writer.realUnlessDefault(FIELD_SHEAR_NORMAL_STRESS_RATIO, card.shearNormalStressRatio, 1.0);
    writer.realUnlessDefault(FIELD_EXPONENT, card.exponent, 1.0);
    writer.realUnlessDefault(FIELD_VISCOUS_DISSIPATION, card.viscousDissipation, 0.0);
    writer.realUnlessDefault(FIELD_REFERENCE_DISPLACEMENT_RATE, card.referenceDisplacementRate, 0.0);
    writer.realUnlessDefault(FIELD_COMPRESSION_STIFFENING, card.compressionStiffening, 1.0);
    writer.realUnlessDefault(FIELD_SHEAR_NORMAL_ENERGY_RATIO, card.shearNormalEnergyRatio, 1.0);
    writer.realUnlessDefault(FIELD_IN_PLANE_STIFFNESS, card.inPlaneStiffness, 0.0);
    writer.realUnlessDefault(FIELD_NORMAL_STIFFNESS, card.normalStiffness, 0.0);
}

std::uint32_t CohesiveMatTable::append(const CohesiveMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CohesiveMatTable::store(std::uint32_t row, const CohesiveMatCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CohesiveMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CohesiveMatTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void CohesiveMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "tableId", ReferenceTarget::StressTable, m_cards.edit(row).tableId);
}

} // namespace oor::model
