#include "oor/model/cards/materials/anisoMat2D/AnisoMat2DTable.hpp"

namespace oor::model
{

std::optional<AnisoMat2DCard> AnisoMat2DFormat::read(FieldReader& reader)
{
    static constexpr const char* STIFFNESS_NAMES[6] = {"stiffness11", "stiffness12", "stiffness13",
                                                       "stiffness22", "stiffness23", "stiffness33"};
    static constexpr const char* THERMAL_EXPANSION_NAMES[3] = {"thermalExpansion1", "thermalExpansion2",
                                                               "thermalExpansion3"};
    static constexpr const char* DAMPING_TERM_NAMES[6] = {"structuralDamping11", "structuralDamping12",
                                                          "structuralDamping13", "structuralDamping22",
                                                          "structuralDamping23", "structuralDamping33"};

    AnisoMat2DCard card;
    card.id = reader.requiredInteger(1, "id");
    for (int index = 0; index < 6; ++index)
        card.stiffness[static_cast<std::size_t>(index)] = reader.real(2 + index, STIFFNESS_NAMES[index], 0.0);
    card.density = reader.real(8, "density", 0.0);
    for (int index = 0; index < 3; ++index)
        card.thermalExpansion[static_cast<std::size_t>(index)] =
            reader.real(9 + index, THERMAL_EXPANSION_NAMES[index], 0.0);
    card.referenceTemperature = reader.real(12, "referenceTemperature", 0.0);
    card.structuralDamping = reader.real(13, "structuralDamping", 0.0);
    card.tensionAllowable = reader.real(14, "tensionAllowable", 0.0);
    card.compressionAllowable = reader.real(15, "compressionAllowable", 0.0);
    card.shearAllowable = reader.real(16, "shearAllowable", 0.0);
    card.stressFrameId = reader.integer(17, "stressFrameId", 0);
    for (int index = 0; index < 6; ++index)
        card.structuralDampingTerms[static_cast<std::size_t>(index)] =
            reader.real(18 + index, DAMPING_TERM_NAMES[index], 0.0);
    if (reader.lastUsedField() > 23)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 23 fields", "structuralDamping33");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AnisoMat2DFormat::check(const AnisoMat2DCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "id"};
    if (card.stressFrameId < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "stressFrameId"};
    return std::nullopt;
}

void AnisoMat2DFormat::write(const AnisoMat2DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (int index = 0; index < 6; ++index)
        writer.realUnlessDefault(2 + index, card.stiffness[static_cast<std::size_t>(index)], 0.0);
    writer.realUnlessDefault(8, card.density, 0.0);
    for (int index = 0; index < 3; ++index)
        writer.realUnlessDefault(9 + index, card.thermalExpansion[static_cast<std::size_t>(index)], 0.0);
    writer.realUnlessDefault(12, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(13, card.structuralDamping, 0.0);
    writer.realUnlessDefault(14, card.tensionAllowable, 0.0);
    writer.realUnlessDefault(15, card.compressionAllowable, 0.0);
    writer.realUnlessDefault(16, card.shearAllowable, 0.0);
    writer.integerUnlessDefault(17, card.stressFrameId, 0);
    for (int index = 0; index < 6; ++index)
        writer.realUnlessDefault(18 + index, card.structuralDampingTerms[static_cast<std::size_t>(index)], 0.0);
}

std::uint32_t AnisoMat2DTable::append(const AnisoMat2DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffness.push(card.stiffness);
    m_density.push(card.density);
    m_thermalExpansion.push(card.thermalExpansion);
    m_referenceTemperature.push(card.referenceTemperature);
    m_structuralDamping.push(card.structuralDamping);
    m_tensionAllowable.push(card.tensionAllowable);
    m_compressionAllowable.push(card.compressionAllowable);
    m_shearAllowable.push(card.shearAllowable);
    m_stressFrameId.push(card.stressFrameId);
    m_structuralDampingTerms.push(card.structuralDampingTerms);
    return row;
}

AnisoMat2DCard AnisoMat2DTable::card(std::uint32_t row) const
{
    return AnisoMat2DCard{m_id.get(row),
                          m_stiffness.get(row),
                          m_density.get(row),
                          m_thermalExpansion.get(row),
                          m_referenceTemperature.get(row),
                          m_structuralDamping.get(row),
                          m_tensionAllowable.get(row),
                          m_compressionAllowable.get(row),
                          m_shearAllowable.get(row),
                          m_stressFrameId.get(row),
                          m_structuralDampingTerms.get(row)};
}

void AnisoMat2DTable::store(std::uint32_t row, const AnisoMat2DCard& card)
{
    m_id.set(row, card.id);
    m_stiffness.set(row, card.stiffness);
    m_density.set(row, card.density);
    m_thermalExpansion.set(row, card.thermalExpansion);
    m_referenceTemperature.set(row, card.referenceTemperature);
    m_structuralDamping.set(row, card.structuralDamping);
    m_tensionAllowable.set(row, card.tensionAllowable);
    m_compressionAllowable.set(row, card.compressionAllowable);
    m_shearAllowable.set(row, card.shearAllowable);
    m_stressFrameId.set(row, card.stressFrameId);
    m_structuralDampingTerms.set(row, card.structuralDampingTerms);
}

void AnisoMat2DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffness.keepRows(rows);
    m_density.keepRows(rows);
    m_thermalExpansion.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
    m_structuralDamping.keepRows(rows);
    m_tensionAllowable.keepRows(rows);
    m_compressionAllowable.keepRows(rows);
    m_shearAllowable.keepRows(rows);
    m_stressFrameId.keepRows(rows);
    m_structuralDampingTerms.keepRows(rows);
}

void AnisoMat2DTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId stressFrameId = m_stressFrameId.get(row); stressFrameId > 0)
        m_stressFrameId.set(row, visitor.visit("stressFrameId", ReferenceTarget::Coordinate, stressFrameId));
}

} // namespace oor::model
