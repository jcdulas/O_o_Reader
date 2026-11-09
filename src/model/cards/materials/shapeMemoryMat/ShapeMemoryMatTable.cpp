#include "oor/model/cards/materials/shapeMemoryMat/ShapeMemoryMatTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int FIELD_AUSTENITE = 9;
constexpr int FIELD_MARTENSITE = 17;
// The thermo-mechanical values: VT, MFRAC, SIGMA-EFF and SIGMA-MAX on the line that starts at field 25, then the g
// function on the line that starts at field 33.
constexpr int THERMO_FIELDS[ShapeMemoryMatCard::THERMO_VALUES] = {25, 26, 27, 28, 33, 34, 35, 36, 37, 38, 39, 40};

constexpr const char* AUSTENITE_NAMES[ShapeMemoryMatCard::PHASE_VALUES] = {
    "austeniteYoungsModulus", "austenitePoissonRatio",     "austeniteThermalExpansion",  "austeniteStressLimit",
    "austeniteDensity",       "transformationStartStress", "transformationFinishStress", "austeniteStressSlope"};
constexpr const char* MARTENSITE_NAMES[ShapeMemoryMatCard::PHASE_VALUES] = {
    "martensiteYoungsModulus", "martensitePoissonRatio", "martensiteThermalExpansion", "martensiteStressLimit",
    "martensiteDensity",       "reverseStartStress",     "reverseFinishStress",        "martensiteStressSlope"};
constexpr const char* THERMO_NAMES[ShapeMemoryMatCard::THERMO_VALUES] = {"transformationVolumeChange",
                                                                         "martensiteFraction",
                                                                         "effectiveStressLimit",
                                                                         "maximumStress",
                                                                         "gFunction0",
                                                                         "gFunctionA",
                                                                         "gFunctionB",
                                                                         "gFunctionC",
                                                                         "gFunctionD",
                                                                         "gFunctionE",
                                                                         "gFunctionF",
                                                                         "gFunctionMaximum"};

template <std::size_t N>
bool allFinite(const std::array<double, N>& values)
{
    for (const double value : values)
    {
        if (!std::isfinite(value))
            return false;
    }
    return true;
}

} // namespace

std::optional<ShapeMemoryMatCard> ShapeMemoryMatFormat::read(FieldReader& reader)
{
    ShapeMemoryMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.model = reader.requiredInteger(2, "model");
    card.referenceTemperature = reader.requiredReal(3, "referenceTemperature");
    card.maximumDetwinningStrain = reader.requiredReal(4, "maximumDetwinningStrain");
    for (std::size_t index = 0; index < ShapeMemoryMatCard::PHASE_VALUES; ++index)
    {
        card.austenite[index] = reader.real(FIELD_AUSTENITE + static_cast<int>(index), AUSTENITE_NAMES[index], 0.0);
        card.martensite[index] = reader.real(FIELD_MARTENSITE + static_cast<int>(index), MARTENSITE_NAMES[index], 0.0);
    }
    bool anyThermo = false;
    for (const int field : THERMO_FIELDS)
        anyThermo = anyThermo || !reader.isBlank(field);
    if (anyThermo)
    {
        std::array<double, ShapeMemoryMatCard::THERMO_VALUES> values{};
        for (std::size_t index = 0; index < values.size(); ++index)
            values[index] = reader.real(THERMO_FIELDS[index], THERMO_NAMES[index], 0.0);
        card.thermo = values;
    }
    requireBlankFields(reader, {5, 6, 7, 8, 29, 30, 31, 32}, THERMO_FIELDS[ShapeMemoryMatCard::THERMO_VALUES - 1]);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShapeMemoryMatFormat::check(const ShapeMemoryMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.model != 1 && card.model != 2)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be 1 (mechanical) or 2 (thermo-mechanical)",
                           "model"};
    if (!(card.referenceTemperature > 0.0) || !std::isfinite(card.referenceTemperature))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "referenceTemperature"};
    if (!(card.maximumDetwinningStrain > 0.0) || !std::isfinite(card.maximumDetwinningStrain))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "maximumDetwinningStrain"};
    if (!allFinite(card.austenite) || !allFinite(card.martensite) || (card.thermo && !allFinite(*card.thermo)))
        return CardProblem{"card.bad-value", "a property of the alloy must be a finite number",
                           "austeniteYoungsModulus"};
    // the mechanical model is the thermo-mechanical one without its last two lines.
    if (card.model == 1 && card.thermo)
        return CardProblem{
            "card.bad-value",
            "the thermo-mechanical values (fields 25 to 40) need " + std::string(CardProblem::FIELD) + " = 2", "model"};
    return std::nullopt;
}

void ShapeMemoryMatFormat::write(const ShapeMemoryMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.model);
    writer.real(3, card.referenceTemperature);
    writer.real(4, card.maximumDetwinningStrain);
    for (std::size_t index = 0; index < ShapeMemoryMatCard::PHASE_VALUES; ++index)
    {
        writer.real(FIELD_AUSTENITE + static_cast<int>(index), card.austenite[index]);
        writer.real(FIELD_MARTENSITE + static_cast<int>(index), card.martensite[index]);
    }
    if (card.thermo)
    {
        for (std::size_t index = 0; index < card.thermo->size(); ++index)
            writer.real(THERMO_FIELDS[index], (*card.thermo)[index]);
    }
}

std::uint32_t ShapeMemoryMatTable::append(const ShapeMemoryMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ShapeMemoryMatTable::store(std::uint32_t row, const ShapeMemoryMatCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ShapeMemoryMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ShapeMemoryMatTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
