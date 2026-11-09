#include "oor/model/cards/materials/anisoMat3D/AnisoMat3DTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int FIRST_STIFFNESS_FIELD = 2;
constexpr int DENSITY_FIELD = 23;
constexpr int FIRST_THERMAL_EXPANSION_FIELD = 24;
constexpr int REFERENCE_TEMPERATURE_FIELD = 30;
constexpr int DAMPING_FIELD = 31;
constexpr int FIRST_DAMPING_TERM_FIELD = 33;
constexpr int LAST_FIELD = 53;

constexpr const char* STIFFNESS_NAMES[21] = {
    "stiffness11", "stiffness12", "stiffness13", "stiffness14", "stiffness15", "stiffness16", "stiffness22",
    "stiffness23", "stiffness24", "stiffness25", "stiffness26", "stiffness33", "stiffness34", "stiffness35",
    "stiffness36", "stiffness44", "stiffness45", "stiffness46", "stiffness55", "stiffness56", "stiffness66"};
constexpr const char* DAMPING_TERM_NAMES[21] = {
    "structuralDamping11", "structuralDamping12", "structuralDamping13", "structuralDamping14", "structuralDamping15",
    "structuralDamping16", "structuralDamping22", "structuralDamping23", "structuralDamping24", "structuralDamping25",
    "structuralDamping26", "structuralDamping33", "structuralDamping34", "structuralDamping35", "structuralDamping36",
    "structuralDamping44", "structuralDamping45", "structuralDamping46", "structuralDamping55", "structuralDamping56",
    "structuralDamping66"};
constexpr const char* THERMAL_EXPANSION_NAMES[6] = {"thermalExpansion1", "thermalExpansion2", "thermalExpansion3",
                                                    "thermalExpansion4", "thermalExpansion5", "thermalExpansion6"};

bool allFinite(const std::array<double, 21>& values)
{
    for (const double value : values)
    {
        if (!std::isfinite(value))
            return false;
    }
    return true;
}

} // namespace

std::optional<AnisoMat3DCard> AnisoMat3DFormat::read(FieldReader& reader)
{
    AnisoMat3DCard card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < 21; ++index)
        card.stiffness[index] =
            reader.real(FIRST_STIFFNESS_FIELD + static_cast<int>(index), STIFFNESS_NAMES[index], 0.0);
    card.density = reader.real(DENSITY_FIELD, "density", 0.0);
    for (std::size_t index = 0; index < 6; ++index)
        card.thermalExpansion[index] =
            reader.real(FIRST_THERMAL_EXPANSION_FIELD + static_cast<int>(index), THERMAL_EXPANSION_NAMES[index], 0.0);
    card.referenceTemperature = reader.real(REFERENCE_TEMPERATURE_FIELD, "referenceTemperature", 0.0);
    card.structuralDamping = reader.real(DAMPING_FIELD, "structuralDamping", 0.0);
    // The structural damping matrix is there when any of its fields is.
    bool hasStructuralDampingTerms = false;
    for (int field = FIRST_DAMPING_TERM_FIELD; field <= LAST_FIELD; ++field)
        hasStructuralDampingTerms = hasStructuralDampingTerms || !reader.isBlank(field);
    if (hasStructuralDampingTerms)
    {
        std::array<double, 21> structuralDampingTerms = {};
        for (std::size_t index = 0; index < 21; ++index)
            structuralDampingTerms[index] =
                reader.real(FIRST_DAMPING_TERM_FIELD + static_cast<int>(index), DAMPING_TERM_NAMES[index], 0.0);
        card.structuralDampingTerms = structuralDampingTerms;
    }
    requireBlankFields(reader, {32}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AnisoMat3DFormat::check(const AnisoMat3DCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!allFinite(card.stiffness))
        return CardProblem{"card.bad-value", "the terms of the material property matrix must be finite numbers",
                           "stiffness11"};
    if (card.structuralDampingTerms && !allFinite(*card.structuralDampingTerms))
        return CardProblem{"card.bad-value", "the terms of the damping matrix must be finite numbers",
                           "structuralDamping11"};
    return std::nullopt;
}

void AnisoMat3DFormat::write(const AnisoMat3DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < 21; ++index)
        writer.realUnlessDefault(FIRST_STIFFNESS_FIELD + static_cast<int>(index), card.stiffness[index], 0.0);
    writer.realUnlessDefault(DENSITY_FIELD, card.density, 0.0);
    for (std::size_t index = 0; index < 6; ++index)
        writer.realUnlessDefault(FIRST_THERMAL_EXPANSION_FIELD + static_cast<int>(index), card.thermalExpansion[index],
                                 0.0);
    writer.realUnlessDefault(REFERENCE_TEMPERATURE_FIELD, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(DAMPING_FIELD, card.structuralDamping, 0.0);
    if (card.structuralDampingTerms)
    {
        // Every term of a present matrix is written, so that the matrix is still there when all of them are zero.
        for (std::size_t index = 0; index < 21; ++index)
            writer.real(FIRST_DAMPING_TERM_FIELD + static_cast<int>(index), (*card.structuralDampingTerms)[index]);
    }
}

std::uint32_t AnisoMat3DTable::append(const AnisoMat3DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffness.push(card.stiffness);
    m_density.push(card.density);
    m_thermalExpansion.push(card.thermalExpansion);
    m_referenceTemperature.push(card.referenceTemperature);
    m_structuralDamping.push(card.structuralDamping);
    m_hasStructuralDampingTerms.push(card.structuralDampingTerms ? 1 : 0);
    m_structuralDampingTerms.push(card.structuralDampingTerms.value_or(std::array<double, 21>{}));
    return row;
}

AnisoMat3DCard AnisoMat3DTable::card(std::uint32_t row) const
{
    AnisoMat3DCard card;
    card.id = m_id.get(row);
    card.stiffness = m_stiffness.get(row);
    card.density = m_density.get(row);
    card.thermalExpansion = m_thermalExpansion.get(row);
    card.referenceTemperature = m_referenceTemperature.get(row);
    card.structuralDamping = m_structuralDamping.get(row);
    if (m_hasStructuralDampingTerms.get(row) != 0)
        card.structuralDampingTerms = m_structuralDampingTerms.get(row);
    return card;
}

void AnisoMat3DTable::store(std::uint32_t row, const AnisoMat3DCard& card)
{
    m_id.set(row, card.id);
    m_stiffness.set(row, card.stiffness);
    m_density.set(row, card.density);
    m_thermalExpansion.set(row, card.thermalExpansion);
    m_referenceTemperature.set(row, card.referenceTemperature);
    m_structuralDamping.set(row, card.structuralDamping);
    m_hasStructuralDampingTerms.set(row, card.structuralDampingTerms ? 1 : 0);
    m_structuralDampingTerms.set(row, card.structuralDampingTerms.value_or(std::array<double, 21>{}));
}

void AnisoMat3DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffness.keepRows(rows);
    m_density.keepRows(rows);
    m_thermalExpansion.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
    m_structuralDamping.keepRows(rows);
    m_hasStructuralDampingTerms.keepRows(rows);
    m_structuralDampingTerms.keepRows(rows);
}

} // namespace oor::model
