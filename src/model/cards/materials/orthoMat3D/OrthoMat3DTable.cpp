#include "oor/model/cards/materials/orthoMat3D/OrthoMat3DTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<OrthoMat3DCard> OrthoMat3DFormat::read(FieldReader& reader)
{
    OrthoMat3DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulus1 = reader.requiredReal(2, "youngsModulus1");
    card.youngsModulus2 = reader.requiredReal(3, "youngsModulus2");
    card.youngsModulus3 = reader.requiredReal(4, "youngsModulus3");
    card.poissonRatio12 = reader.requiredReal(5, "poissonRatio12");
    card.poissonRatio13 = reader.requiredReal(6, "poissonRatio13");
    card.poissonRatio23 = reader.requiredReal(7, "poissonRatio23");
    card.shearModulus12 = reader.requiredReal(8, "shearModulus12");
    card.shearModulus13 = reader.requiredReal(9, "shearModulus13");
    card.shearModulus23 = reader.requiredReal(10, "shearModulus23");
    card.density = reader.real(11, "density", 0.0);
    card.thermalExpansion1 = reader.real(12, "thermalExpansion1", 0.0);
    card.thermalExpansion2 = reader.real(13, "thermalExpansion2", 0.0);
    card.thermalExpansion3 = reader.real(14, "thermalExpansion3", 0.0);
    card.referenceTemperature = reader.real(15, "referenceTemperature", 0.0);
    card.structuralDamping = reader.real(16, "structuralDamping", 0.0);
    requireBlankFields(reader, {}, 16);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OrthoMat3DFormat::check(const OrthoMat3DCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!(card.youngsModulus1 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "youngsModulus1"};
    if (!(card.youngsModulus2 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "youngsModulus2"};
    if (!(card.youngsModulus3 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "youngsModulus3"};
    if (!(card.shearModulus12 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "shearModulus12"};
    if (!(card.shearModulus13 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "shearModulus13"};
    if (!(card.shearModulus23 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "shearModulus23"};
    return std::nullopt;
}

void OrthoMat3DFormat::write(const OrthoMat3DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.youngsModulus1);
    writer.real(3, card.youngsModulus2);
    writer.real(4, card.youngsModulus3);
    writer.real(5, card.poissonRatio12);
    writer.real(6, card.poissonRatio13);
    writer.real(7, card.poissonRatio23);
    writer.real(8, card.shearModulus12);
    writer.real(9, card.shearModulus13);
    writer.real(10, card.shearModulus23);
    writer.realUnlessDefault(11, card.density, 0.0);
    writer.realUnlessDefault(12, card.thermalExpansion1, 0.0);
    writer.realUnlessDefault(13, card.thermalExpansion2, 0.0);
    writer.realUnlessDefault(14, card.thermalExpansion3, 0.0);
    writer.realUnlessDefault(15, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(16, card.structuralDamping, 0.0);
}

std::uint32_t OrthoMat3DTable::append(const OrthoMat3DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_youngsModulus1.push(card.youngsModulus1);
    m_youngsModulus2.push(card.youngsModulus2);
    m_youngsModulus3.push(card.youngsModulus3);
    m_poissonRatio12.push(card.poissonRatio12);
    m_poissonRatio13.push(card.poissonRatio13);
    m_poissonRatio23.push(card.poissonRatio23);
    m_shearModulus12.push(card.shearModulus12);
    m_shearModulus13.push(card.shearModulus13);
    m_shearModulus23.push(card.shearModulus23);
    m_density.push(card.density);
    m_thermalExpansion1.push(card.thermalExpansion1);
    m_thermalExpansion2.push(card.thermalExpansion2);
    m_thermalExpansion3.push(card.thermalExpansion3);
    m_referenceTemperature.push(card.referenceTemperature);
    m_structuralDamping.push(card.structuralDamping);
    return row;
}

OrthoMat3DCard OrthoMat3DTable::card(std::uint32_t row) const
{
    OrthoMat3DCard card;
    card.id = m_id.get(row);
    card.youngsModulus1 = m_youngsModulus1.get(row);
    card.youngsModulus2 = m_youngsModulus2.get(row);
    card.youngsModulus3 = m_youngsModulus3.get(row);
    card.poissonRatio12 = m_poissonRatio12.get(row);
    card.poissonRatio13 = m_poissonRatio13.get(row);
    card.poissonRatio23 = m_poissonRatio23.get(row);
    card.shearModulus12 = m_shearModulus12.get(row);
    card.shearModulus13 = m_shearModulus13.get(row);
    card.shearModulus23 = m_shearModulus23.get(row);
    card.density = m_density.get(row);
    card.thermalExpansion1 = m_thermalExpansion1.get(row);
    card.thermalExpansion2 = m_thermalExpansion2.get(row);
    card.thermalExpansion3 = m_thermalExpansion3.get(row);
    card.referenceTemperature = m_referenceTemperature.get(row);
    card.structuralDamping = m_structuralDamping.get(row);
    return card;
}

void OrthoMat3DTable::store(std::uint32_t row, const OrthoMat3DCard& card)
{
    m_id.set(row, card.id);
    m_youngsModulus1.set(row, card.youngsModulus1);
    m_youngsModulus2.set(row, card.youngsModulus2);
    m_youngsModulus3.set(row, card.youngsModulus3);
    m_poissonRatio12.set(row, card.poissonRatio12);
    m_poissonRatio13.set(row, card.poissonRatio13);
    m_poissonRatio23.set(row, card.poissonRatio23);
    m_shearModulus12.set(row, card.shearModulus12);
    m_shearModulus13.set(row, card.shearModulus13);
    m_shearModulus23.set(row, card.shearModulus23);
    m_density.set(row, card.density);
    m_thermalExpansion1.set(row, card.thermalExpansion1);
    m_thermalExpansion2.set(row, card.thermalExpansion2);
    m_thermalExpansion3.set(row, card.thermalExpansion3);
    m_referenceTemperature.set(row, card.referenceTemperature);
    m_structuralDamping.set(row, card.structuralDamping);
}

void OrthoMat3DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_youngsModulus1.keepRows(rows);
    m_youngsModulus2.keepRows(rows);
    m_youngsModulus3.keepRows(rows);
    m_poissonRatio12.keepRows(rows);
    m_poissonRatio13.keepRows(rows);
    m_poissonRatio23.keepRows(rows);
    m_shearModulus12.keepRows(rows);
    m_shearModulus13.keepRows(rows);
    m_shearModulus23.keepRows(rows);
    m_density.keepRows(rows);
    m_thermalExpansion1.keepRows(rows);
    m_thermalExpansion2.keepRows(rows);
    m_thermalExpansion3.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
    m_structuralDamping.keepRows(rows);
}

} // namespace oor::model
