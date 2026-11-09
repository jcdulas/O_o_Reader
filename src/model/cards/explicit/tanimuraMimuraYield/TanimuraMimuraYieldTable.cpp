#include "oor/model/cards/explicit/tanimuraMimuraYield/TanimuraMimuraYieldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<TanimuraMimuraYieldCard> TanimuraMimuraYieldFormat::read(FieldReader& reader)
{
    TanimuraMimuraYieldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficientA = reader.requiredReal(2, "coefficientA");
    card.coefficientB = reader.real(3, "coefficientB", 0.0);
    card.coefficientC = reader.real(4, "coefficientC", 0.0);
    card.coefficientD = reader.real(5, "coefficientD", 0.0);
    card.coefficientM = reader.real(6, "coefficientM", 0.0);
    card.quasiStaticStrainRate = reader.real(7, "quasiStaticStrainRate", 1.0);
    card.specificHeat = reader.real(8, "specificHeat", TANIMURA_MIMURA_YIELD_NO_LIMIT);
    card.meltTemperature = reader.real(9, "meltTemperature", TANIMURA_MIMURA_YIELD_NO_LIMIT);
    card.roomTemperature = reader.real(10, "roomTemperature", TANIMURA_MIMURA_YIELD_ROOM_TEMPERATURE);
    card.criticalStress = reader.real(11, "criticalStress", 1.0);
    card.coefficientE = reader.real(12, "coefficientE", 0.0);
    card.coefficientK = reader.real(13, "coefficientK", 1.0);
    card.referenceStrainRate = reader.real(14, "referenceStrainRate", 1.0);
    if (reader.lastUsedField() > 14)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 14 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TanimuraMimuraYieldFormat::check(const TanimuraMimuraYieldCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.coefficientA, "coefficientA", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coefficientB, "coefficientB", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientC, "coefficientC", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientD, "coefficientD", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientM, "coefficientM", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.quasiStaticStrainRate, "quasiStaticStrainRate", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.specificHeat, "specificHeat", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.meltTemperature, "meltTemperature", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.roomTemperature, "roomTemperature", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.criticalStress, "criticalStress", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coefficientE, "coefficientE", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientK, "coefficientK", RealRule::Finite))
        return problem;
    return checkReal(card.referenceStrainRate, "referenceStrainRate", RealRule::Positive);
}

void TanimuraMimuraYieldFormat::write(const TanimuraMimuraYieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.coefficientA);
    writer.realUnlessDefault(3, card.coefficientB, 0.0);
    writer.realUnlessDefault(4, card.coefficientC, 0.0);
    writer.realUnlessDefault(5, card.coefficientD, 0.0);
    writer.realUnlessDefault(6, card.coefficientM, 0.0);
    writer.realUnlessDefault(7, card.quasiStaticStrainRate, 1.0);
    writer.realUnlessDefault(8, card.specificHeat, TANIMURA_MIMURA_YIELD_NO_LIMIT);
    writer.realUnlessDefault(9, card.meltTemperature, TANIMURA_MIMURA_YIELD_NO_LIMIT);
    writer.realUnlessDefault(10, card.roomTemperature, TANIMURA_MIMURA_YIELD_ROOM_TEMPERATURE);
    writer.realUnlessDefault(11, card.criticalStress, 1.0);
    writer.realUnlessDefault(12, card.coefficientE, 0.0);
    writer.realUnlessDefault(13, card.coefficientK, 1.0);
    writer.realUnlessDefault(14, card.referenceStrainRate, 1.0);
}

std::uint32_t TanimuraMimuraYieldTable::append(const TanimuraMimuraYieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TanimuraMimuraYieldTable::store(std::uint32_t row, const TanimuraMimuraYieldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TanimuraMimuraYieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TanimuraMimuraYieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
