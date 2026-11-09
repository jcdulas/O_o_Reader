#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"

namespace oor::model
{

bool IsoMatFormat::resolve(std::optional<double> youngsModulus, std::optional<double> shearModulus,
                           std::optional<double> poissonRatio, IsoMatCard& card)
{
    if (!youngsModulus && !shearModulus)
        return false; // E and G may not both be blank
    card.blankFields = static_cast<std::uint8_t>((youngsModulus ? 0 : IsoMatCard::BLANK_YOUNGS_MODULUS) |
                                                 (shearModulus ? 0 : IsoMatCard::BLANK_SHEAR_MODULUS) |
                                                 (poissonRatio ? 0 : IsoMatCard::BLANK_POISSON_RATIO));
    if (youngsModulus && shearModulus && poissonRatio)
    {
        card.youngsModulus = *youngsModulus;
        card.shearModulus = *shearModulus;
        card.poissonRatio = *poissonRatio;
    }
    else if (youngsModulus && shearModulus)
    {
        card.youngsModulus = *youngsModulus;
        card.shearModulus = *shearModulus;
        card.poissonRatio = *shearModulus != 0.0 ? *youngsModulus / (2.0 * *shearModulus) - 1.0 : 0.0;
    }
    else if (youngsModulus && poissonRatio)
    {
        card.youngsModulus = *youngsModulus;
        card.poissonRatio = *poissonRatio;
        card.shearModulus = *youngsModulus / (2.0 * (1.0 + *poissonRatio));
    }
    else if (shearModulus && poissonRatio)
    {
        card.shearModulus = *shearModulus;
        card.poissonRatio = *poissonRatio;
        card.youngsModulus = 2.0 * *shearModulus * (1.0 + *poissonRatio);
    }
    else
    {
        // Only one of E, G given: NU and the other are set to 0.0.
        card.youngsModulus = youngsModulus.value_or(0.0);
        card.shearModulus = shearModulus.value_or(0.0);
        card.poissonRatio = 0.0;
    }
    return true;
}

std::optional<IsoMatCard> IsoMatFormat::read(FieldReader& reader)
{
    IsoMatCard card;
    card.id = reader.requiredInteger(1, "id");
    const auto youngsModulus = reader.optionalReal(2, "youngsModulus");
    const auto shearModulus = reader.optionalReal(3, "shearModulus");
    const auto poissonRatio = reader.optionalReal(4, "poissonRatio");
    card.density = reader.real(5, "density", 0.0);
    card.thermalExpansion = reader.real(6, "thermalExpansion", 0.0);
    card.referenceTemperature = reader.real(7, "referenceTemperature", 0.0);
    card.structuralDamping = reader.real(8, "structuralDamping", 0.0);
    card.tensionAllowable = reader.real(9, "tensionAllowable", 0.0);
    card.compressionAllowable = reader.real(10, "compressionAllowable", 0.0);
    card.shearAllowable = reader.real(11, "shearAllowable", 0.0);
    card.stressFrameId = reader.integer(12, "stressFrameId", 0);
    if (reader.failed())
        return card;
    if (!resolve(youngsModulus, shearModulus, poissonRatio, card))
        reader.error("card.missing-field",
                     std::string("youngsModulus") + " and " + std::string("shearModulus") + " may not both be blank",
                     "youngsModulus");
    else
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> IsoMatFormat::check(const IsoMatCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "id"};
    if (card.stressFrameId < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "stressFrameId"};
    return std::nullopt;
}

namespace
{

// The fields of E, G and NU to leave blank when the card is written: the ones the card had blank, as long as the three
// values are still the ones the blanks give (an edit of E, G or NU through the API leaves the bits as they are, and a
// value that no longer follows from the others is then written).
std::uint8_t blanksToWrite(const IsoMatCard& card)
{
    if (card.blankFields == 0)
        return 0;
    const auto given = [&](std::uint8_t bit, double value)
    { return (card.blankFields & bit) != 0 ? std::nullopt : std::optional<double>(value); };
    IsoMatCard resolved = card;
    if (!IsoMatFormat::resolve(given(IsoMatCard::BLANK_YOUNGS_MODULUS, card.youngsModulus),
                               given(IsoMatCard::BLANK_SHEAR_MODULUS, card.shearModulus),
                               given(IsoMatCard::BLANK_POISSON_RATIO, card.poissonRatio), resolved))
        return 0;
    const bool unchanged = resolved.youngsModulus == card.youngsModulus && resolved.shearModulus == card.shearModulus &&
                           resolved.poissonRatio == card.poissonRatio;
    return unchanged ? card.blankFields : 0;
}

} // namespace

void IsoMatFormat::write(const IsoMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    // E, G and NU depend on each other: written unless the card left them blank and they still hold what the blank
    // gives.
    const std::uint8_t blank = blanksToWrite(card);
    if ((blank & IsoMatCard::BLANK_YOUNGS_MODULUS) == 0)
        writer.real(2, card.youngsModulus);
    if ((blank & IsoMatCard::BLANK_SHEAR_MODULUS) == 0)
        writer.real(3, card.shearModulus);
    if ((blank & IsoMatCard::BLANK_POISSON_RATIO) == 0)
        writer.real(4, card.poissonRatio);
    writer.realUnlessDefault(5, card.density, 0.0);
    writer.realUnlessDefault(6, card.thermalExpansion, 0.0);
    writer.realUnlessDefault(7, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(8, card.structuralDamping, 0.0);
    writer.realUnlessDefault(9, card.tensionAllowable, 0.0);
    writer.realUnlessDefault(10, card.compressionAllowable, 0.0);
    writer.realUnlessDefault(11, card.shearAllowable, 0.0);
    writer.integerUnlessDefault(12, card.stressFrameId, 0);
}

std::uint32_t IsoMatTable::append(const IsoMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_youngsModulus.push(card.youngsModulus);
    m_shearModulus.push(card.shearModulus);
    m_poissonRatio.push(card.poissonRatio);
    m_density.push(card.density);
    m_thermalExpansion.push(card.thermalExpansion);
    m_referenceTemperature.push(card.referenceTemperature);
    m_structuralDamping.push(card.structuralDamping);
    m_tensionAllowable.push(card.tensionAllowable);
    m_compressionAllowable.push(card.compressionAllowable);
    m_shearAllowable.push(card.shearAllowable);
    m_stressFrameId.push(card.stressFrameId);
    m_blankFields.push(card.blankFields);
    return row;
}

IsoMatCard IsoMatTable::card(std::uint32_t row) const
{
    return IsoMatCard{m_id.get(row),
                      m_youngsModulus.get(row),
                      m_shearModulus.get(row),
                      m_poissonRatio.get(row),
                      m_density.get(row),
                      m_thermalExpansion.get(row),
                      m_referenceTemperature.get(row),
                      m_structuralDamping.get(row),
                      m_tensionAllowable.get(row),
                      m_compressionAllowable.get(row),
                      m_shearAllowable.get(row),
                      m_stressFrameId.get(row),
                      m_blankFields.get(row)};
}

void IsoMatTable::store(std::uint32_t row, const IsoMatCard& card)
{
    m_id.set(row, card.id);
    m_youngsModulus.set(row, card.youngsModulus);
    m_shearModulus.set(row, card.shearModulus);
    m_poissonRatio.set(row, card.poissonRatio);
    m_density.set(row, card.density);
    m_thermalExpansion.set(row, card.thermalExpansion);
    m_referenceTemperature.set(row, card.referenceTemperature);
    m_structuralDamping.set(row, card.structuralDamping);
    m_tensionAllowable.set(row, card.tensionAllowable);
    m_compressionAllowable.set(row, card.compressionAllowable);
    m_shearAllowable.set(row, card.shearAllowable);
    m_stressFrameId.set(row, card.stressFrameId);
    m_blankFields.set(row, card.blankFields);
}

void IsoMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_youngsModulus.keepRows(rows);
    m_shearModulus.keepRows(rows);
    m_poissonRatio.keepRows(rows);
    m_density.keepRows(rows);
    m_thermalExpansion.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
    m_structuralDamping.keepRows(rows);
    m_tensionAllowable.keepRows(rows);
    m_compressionAllowable.keepRows(rows);
    m_shearAllowable.keepRows(rows);
    m_stressFrameId.keepRows(rows);
    m_blankFields.keepRows(rows);
}

void IsoMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId stressFrameId = m_stressFrameId.get(row); stressFrameId > 0)
        m_stressFrameId.set(row, visitor.visit("stressFrameId", ReferenceTarget::Coordinate, stressFrameId));
}

} // namespace oor::model
