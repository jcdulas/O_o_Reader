#include "oor/model/cards/materials/userMat/UserMatTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int FIELD_DENSITY = 4;
constexpr int FIELD_THERMAL_EXPANSION = 5; // A1 to A3: fields 5 to 7
constexpr int FIELD_REFERENCE_TEMPERATURE = 8;
constexpr int FIELD_DAMPING = 9;
constexpr int FIELD_TENSION_ALLOWABLE = 10;
constexpr int FIELD_COMPRESSION_ALLOWABLE = 11;
constexpr int FIELD_SHEAR_ALLOWABLE = 12;

constexpr const char* THERMAL_EXPANSION_NAMES[3] = {"thermalExpansion1", "thermalExpansion2", "thermalExpansion3"};

bool isKinematicFlag(int flag)
{
    return flag == 0 || flag == 1 || flag == 2 || flag == 3 || flag == 13 || flag == 23;
}

} // namespace

std::optional<UserMatCard> UserMatFormat::read(FieldReader& reader)
{
    UserMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.userFrameFlag = reader.integer(2, "userFrameFlag", 0);
    card.kinematicQuantities = reader.integer(3, "kinematicQuantities", 0);
    card.density = reader.requiredReal(FIELD_DENSITY, "density");
    for (std::size_t index = 0; index < card.thermalExpansion.size(); ++index)
        card.thermalExpansion[index] =
            reader.optionalReal(FIELD_THERMAL_EXPANSION + static_cast<int>(index), THERMAL_EXPANSION_NAMES[index]);
    card.referenceTemperature = reader.real(FIELD_REFERENCE_TEMPERATURE, "referenceTemperature", 0.0);
    card.structuralDamping = reader.real(FIELD_DAMPING, "structuralDamping", 0.0);
    card.tensionAllowable = reader.real(FIELD_TENSION_ALLOWABLE, "tensionAllowable", 0.0);
    card.compressionAllowable = reader.real(FIELD_COMPRESSION_ALLOWABLE, "compressionAllowable", 0.0);
    card.shearAllowable = reader.real(FIELD_SHEAR_ALLOWABLE, "shearAllowable", 0.0);
    requireBlankFields(reader, {}, FIELD_SHEAR_ALLOWABLE);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserMatFormat::check(const UserMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.userFrameFlag != 0 && card.userFrameFlag != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "userFrameFlag"};
    if (!isKinematicFlag(card.kinematicQuantities))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0, 1, 2, 3, 13 or 23",
                           "kinematicQuantities"};
    if (!(card.density > 0.0) || !std::isfinite(card.density))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero", "density"};
    for (const std::optional<double>& value : card.thermalExpansion)
    {
        if (value && !std::isfinite(*value))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("thermalExpansion2") +
                                   " and " + CardProblem::fieldNamed("thermalExpansion3") + " must be finite numbers",
                               "thermalExpansion1"};
    }
    if (!std::isfinite(card.referenceTemperature) || !std::isfinite(card.structuralDamping) ||
        !std::isfinite(card.tensionAllowable) || !std::isfinite(card.compressionAllowable) ||
        !std::isfinite(card.shearAllowable))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("structuralDamping") +
                               ", " + CardProblem::fieldNamed("tensionAllowable") + ", " +
                               CardProblem::fieldNamed("compressionAllowable") + " and " +
                               CardProblem::fieldNamed("shearAllowable") + " must be finite numbers",
                           "referenceTemperature"};
    return std::nullopt;
}

void UserMatFormat::write(const UserMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.userFrameFlag, 0);
    writer.integerUnlessDefault(3, card.kinematicQuantities, 0);
    writer.real(FIELD_DENSITY, card.density);
    for (std::size_t index = 0; index < card.thermalExpansion.size(); ++index)
    {
        if (card.thermalExpansion[index])
            writer.real(FIELD_THERMAL_EXPANSION + static_cast<int>(index), *card.thermalExpansion[index]);
    }
    writer.realUnlessDefault(FIELD_REFERENCE_TEMPERATURE, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(FIELD_DAMPING, card.structuralDamping, 0.0);
    writer.realUnlessDefault(FIELD_TENSION_ALLOWABLE, card.tensionAllowable, 0.0);
    writer.realUnlessDefault(FIELD_COMPRESSION_ALLOWABLE, card.compressionAllowable, 0.0);
    writer.realUnlessDefault(FIELD_SHEAR_ALLOWABLE, card.shearAllowable, 0.0);
}

std::uint32_t UserMatTable::append(const UserMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void UserMatTable::store(std::uint32_t row, const UserMatCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void UserMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void UserMatTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
