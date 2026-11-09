#include "oor/model/cards/materials/axisymOrthoMat/AxisymOrthoMatTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

std::optional<AxisymOrthoMatCard> AxisymOrthoMatFormat::read(FieldReader& reader)
{
    AxisymOrthoMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulusX = reader.requiredReal(2, "youngsModulusX");
    card.youngsModulusTheta = reader.requiredReal(3, "youngsModulusTheta");
    card.youngsModulusZ = reader.requiredReal(4, "youngsModulusZ");
    card.poissonRatioXth = reader.real(5, "poissonRatioXth", 0.0);
    card.poissonRatioThz = reader.real(6, "poissonRatioThz", 0.0);
    card.poissonRatioZx = reader.real(7, "poissonRatioZx", 0.0);
    card.density = reader.real(8, "density", 0.0);
    // Fields 9 and 10 are the two blank cells at the start of the second line.
    card.shearModulusZx = reader.requiredReal(11, "shearModulusZx");
    card.thermalExpansionX = reader.real(12, "thermalExpansionX", 0.0);
    card.thermalExpansionTheta = reader.real(13, "thermalExpansionTheta", 0.0);
    card.thermalExpansionZ = reader.real(14, "thermalExpansionZ", 0.0);
    card.referenceTemperature = reader.real(15, "referenceTemperature", 0.0);
    card.structuralDamping = reader.real(16, "structuralDamping", 0.0);
    requireBlankFields(reader, {9, 10}, 16);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymOrthoMatFormat::check(const AxisymOrthoMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!(card.youngsModulusX > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "youngsModulusX"};
    if (!(card.youngsModulusTheta > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "youngsModulusTheta"};
    if (!(card.youngsModulusZ > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "youngsModulusZ"};
    if (!(card.shearModulusZx > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "shearModulusZx"};
    return std::nullopt;
}

void AxisymOrthoMatFormat::write(const AxisymOrthoMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.youngsModulusX);
    writer.real(3, card.youngsModulusTheta);
    writer.real(4, card.youngsModulusZ);
    writer.realUnlessDefault(5, card.poissonRatioXth, 0.0);
    writer.realUnlessDefault(6, card.poissonRatioThz, 0.0);
    writer.realUnlessDefault(7, card.poissonRatioZx, 0.0);
    writer.realUnlessDefault(8, card.density, 0.0);
    writer.real(11, card.shearModulusZx);
    writer.realUnlessDefault(12, card.thermalExpansionX, 0.0);
    writer.realUnlessDefault(13, card.thermalExpansionTheta, 0.0);
    writer.realUnlessDefault(14, card.thermalExpansionZ, 0.0);
    writer.realUnlessDefault(15, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(16, card.structuralDamping, 0.0);
}

std::uint32_t AxisymOrthoMatTable::append(const AxisymOrthoMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_youngsModulusX.push(card.youngsModulusX);
    m_youngsModulusTheta.push(card.youngsModulusTheta);
    m_youngsModulusZ.push(card.youngsModulusZ);
    m_poissonRatioXth.push(card.poissonRatioXth);
    m_poissonRatioThz.push(card.poissonRatioThz);
    m_poissonRatioZx.push(card.poissonRatioZx);
    m_density.push(card.density);
    m_shearModulusZx.push(card.shearModulusZx);
    m_thermalExpansionX.push(card.thermalExpansionX);
    m_thermalExpansionTheta.push(card.thermalExpansionTheta);
    m_thermalExpansionZ.push(card.thermalExpansionZ);
    m_referenceTemperature.push(card.referenceTemperature);
    m_structuralDamping.push(card.structuralDamping);
    return row;
}

AxisymOrthoMatCard AxisymOrthoMatTable::card(std::uint32_t row) const
{
    AxisymOrthoMatCard card;
    card.id = m_id.get(row);
    card.youngsModulusX = m_youngsModulusX.get(row);
    card.youngsModulusTheta = m_youngsModulusTheta.get(row);
    card.youngsModulusZ = m_youngsModulusZ.get(row);
    card.poissonRatioXth = m_poissonRatioXth.get(row);
    card.poissonRatioThz = m_poissonRatioThz.get(row);
    card.poissonRatioZx = m_poissonRatioZx.get(row);
    card.density = m_density.get(row);
    card.shearModulusZx = m_shearModulusZx.get(row);
    card.thermalExpansionX = m_thermalExpansionX.get(row);
    card.thermalExpansionTheta = m_thermalExpansionTheta.get(row);
    card.thermalExpansionZ = m_thermalExpansionZ.get(row);
    card.referenceTemperature = m_referenceTemperature.get(row);
    card.structuralDamping = m_structuralDamping.get(row);
    return card;
}

void AxisymOrthoMatTable::store(std::uint32_t row, const AxisymOrthoMatCard& card)
{
    m_id.set(row, card.id);
    m_youngsModulusX.set(row, card.youngsModulusX);
    m_youngsModulusTheta.set(row, card.youngsModulusTheta);
    m_youngsModulusZ.set(row, card.youngsModulusZ);
    m_poissonRatioXth.set(row, card.poissonRatioXth);
    m_poissonRatioThz.set(row, card.poissonRatioThz);
    m_poissonRatioZx.set(row, card.poissonRatioZx);
    m_density.set(row, card.density);
    m_shearModulusZx.set(row, card.shearModulusZx);
    m_thermalExpansionX.set(row, card.thermalExpansionX);
    m_thermalExpansionTheta.set(row, card.thermalExpansionTheta);
    m_thermalExpansionZ.set(row, card.thermalExpansionZ);
    m_referenceTemperature.set(row, card.referenceTemperature);
    m_structuralDamping.set(row, card.structuralDamping);
}

void AxisymOrthoMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_youngsModulusX.keepRows(rows);
    m_youngsModulusTheta.keepRows(rows);
    m_youngsModulusZ.keepRows(rows);
    m_poissonRatioXth.keepRows(rows);
    m_poissonRatioThz.keepRows(rows);
    m_poissonRatioZx.keepRows(rows);
    m_density.keepRows(rows);
    m_shearModulusZx.keepRows(rows);
    m_thermalExpansionX.keepRows(rows);
    m_thermalExpansionTheta.keepRows(rows);
    m_thermalExpansionZ.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
    m_structuralDamping.keepRows(rows);
}

} // namespace oor::model
