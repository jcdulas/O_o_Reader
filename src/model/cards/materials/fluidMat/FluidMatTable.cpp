#include "oor/model/cards/materials/fluidMat/FluidMatTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

} // namespace

FluidMatTable::FluidMatTable() : m_normalizedAdmittance(notGiven()) {}

std::optional<CardProblem> FluidMatFormat::resolve(std::optional<double> bulkModulus, std::optional<double> density,
                                                   std::optional<double> soundSpeed, FluidMatCard& card)
{
    const int given = (bulkModulus ? 1 : 0) + (density ? 1 : 0) + (soundSpeed ? 1 : 0);
    if (given < 2)
        return CardProblem{"card.missing-field",
                           "two of " + std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("density") +
                               " and " + CardProblem::fieldNamed("soundSpeed") + " must be given",
                           "bulkModulus"};
    if (bulkModulus && density)
    {
        // when all three are given, BULK and RHO are the ones that count.
        card.bulkModulus = *bulkModulus;
        card.density = *density;
        if (soundSpeed)
        {
            card.soundSpeed = *soundSpeed;
        }
        else
        {
            if (!(*bulkModulus / *density >= 0.0))
                return CardProblem{"card.bad-value",
                                   CardProblem::fieldNamed("bulkModulus") + " and " +
                                       CardProblem::fieldNamed("density") +
                                       " must have the same sign for a speed of sound",
                                   "soundSpeed"};
            card.soundSpeed = std::sqrt(*bulkModulus / *density);
        }
    }
    else if (soundSpeed && density)
    {
        card.density = *density;
        card.soundSpeed = *soundSpeed;
        card.bulkModulus = *soundSpeed * *soundSpeed * *density;
    }
    else
    {
        if (*soundSpeed == 0.0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must not be 0.0 when " +
                                   CardProblem::fieldNamed("density") + " is left out",
                               "soundSpeed"};
        card.bulkModulus = *bulkModulus;
        card.soundSpeed = *soundSpeed;
        card.density = *bulkModulus / (*soundSpeed * *soundSpeed);
    }
    return std::nullopt;
}

std::optional<FluidMatCard> FluidMatFormat::read(FieldReader& reader)
{
    FluidMatCard card;
    card.id = reader.requiredInteger(1, "id");
    const std::optional<double> bulkModulus = reader.optionalReal(2, "bulkModulus");
    const std::optional<double> density = reader.optionalReal(3, "density");
    const std::optional<double> soundSpeed = reader.optionalReal(4, "soundSpeed");
    card.structuralDamping = reader.real(5, "structuralDamping", 0.0);
    card.normalizedAdmittance = reader.optionalReal(6, "normalizedAdmittance");
    requireBlankFields(reader, {}, 6);
    if (!reader.failed())
        reader.report(resolve(bulkModulus, density, soundSpeed, card));
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FluidMatFormat::check(const FluidMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!std::isfinite(card.bulkModulus) || !std::isfinite(card.density) || !std::isfinite(card.soundSpeed))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("density") + " and " +
                               CardProblem::fieldNamed("soundSpeed") + " must be finite numbers",
                           "bulkModulus"};
    if (!(card.soundSpeed > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "soundSpeed"};
    // With an ALPHA the bulk modulus and the density may be negative, but not zero.
    if (card.normalizedAdmittance)
    {
        if (card.bulkModulus == 0.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be 0.0", "bulkModulus"};
        if (card.density == 0.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be 0.0", "density"};
        return std::nullopt;
    }
    if (!(card.bulkModulus > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "bulkModulus"};
    if (!(card.density > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0", "density"};
    return std::nullopt;
}

void FluidMatFormat::write(const FluidMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    // BULK, RHO and C depend on each other: all three are always written, never left to a default.
    writer.real(2, card.bulkModulus);
    writer.real(3, card.density);
    writer.real(4, card.soundSpeed);
    writer.realUnlessDefault(5, card.structuralDamping, 0.0);
    if (card.normalizedAdmittance)
        writer.real(6, *card.normalizedAdmittance);
}

std::uint32_t FluidMatTable::append(const FluidMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_bulkModulus.push(card.bulkModulus);
    m_density.push(card.density);
    m_soundSpeed.push(card.soundSpeed);
    m_structuralDamping.push(card.structuralDamping);
    m_normalizedAdmittance.push(card.normalizedAdmittance.value_or(notGiven()));
    return row;
}

FluidMatCard FluidMatTable::card(std::uint32_t row) const
{
    FluidMatCard card;
    card.id = m_id.get(row);
    card.bulkModulus = m_bulkModulus.get(row);
    card.density = m_density.get(row);
    card.soundSpeed = m_soundSpeed.get(row);
    card.structuralDamping = m_structuralDamping.get(row);
    if (const double alpha = m_normalizedAdmittance.get(row); !std::isnan(alpha))
        card.normalizedAdmittance = alpha;
    return card;
}

void FluidMatTable::store(std::uint32_t row, const FluidMatCard& card)
{
    m_id.set(row, card.id);
    m_bulkModulus.set(row, card.bulkModulus);
    m_density.set(row, card.density);
    m_soundSpeed.set(row, card.soundSpeed);
    m_structuralDamping.set(row, card.structuralDamping);
    m_normalizedAdmittance.set(row, card.normalizedAdmittance.value_or(notGiven()));
}

void FluidMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_bulkModulus.keepRows(rows);
    m_density.keepRows(rows);
    m_soundSpeed.keepRows(rows);
    m_structuralDamping.keepRows(rows);
    m_normalizedAdmittance.keepRows(rows);
}

} // namespace oor::model
