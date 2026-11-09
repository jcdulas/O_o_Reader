#include "oor/model/cards/properties/fastenerProp/FastenerPropTable.hpp"

namespace oor::model
{

std::optional<FastenerPropCard> FastenerPropFormat::read(FieldReader& reader)
{
    static constexpr const char* TRANSLATIONAL_STIFFNESS_NAMES[3] = {
        "translationalStiffness1", "translationalStiffness2", "translationalStiffness3"};
    static constexpr const char* ROTATIONAL_STIFFNESS_NAMES[3] = {"rotationalStiffness1", "rotationalStiffness2",
                                                                  "rotationalStiffness3"};
    FastenerPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.diameter = reader.requiredReal(2, "diameter");
    card.matFrameId = reader.integer(3, "matFrameId", FastenerPropCard::NO_FRAME);
    card.absoluteFrame = reader.integer(4, "absoluteFrame", 0);
    for (std::size_t index = 0; index < 3; ++index)
        card.translationalStiffness[index] =
            reader.requiredReal(5 + static_cast<int>(index), TRANSLATIONAL_STIFFNESS_NAMES[index]);
    // KR1 is the last field of the first line, KR2 and KR3 start the second.
    card.rotationalStiffness[0] = reader.real(8, ROTATIONAL_STIFFNESS_NAMES[0], 0.0);
    card.rotationalStiffness[1] = reader.real(9, ROTATIONAL_STIFFNESS_NAMES[1], 0.0);
    card.rotationalStiffness[2] = reader.real(10, ROTATIONAL_STIFFNESS_NAMES[2], 0.0);
    card.mass = reader.real(11, "mass", 0.0);
    card.structuralDamping = reader.real(12, "structuralDamping", 0.0);
    card.alphaCoefficient = reader.real(13, "alphaCoefficient", 0.0);
    card.referenceTemperature = reader.real(14, "referenceTemperature", 0.0);
    card.coincidentLength = reader.real(15, "coincidentLength", 0.0);
    if (reader.lastUsedField() > 15)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has fifteen fields",
                     "coincidentLength");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FastenerPropFormat::check(const FastenerPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (!(card.diameter > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0", "diameter"};
    if (card.matFrameId < FastenerPropCard::NO_FRAME)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be -1 (none) or a coordinate system ID",
                           "matFrameId"};
    if (card.absoluteFrame != 0 && card.absoluteFrame != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "absoluteFrame"};
    if (card.coincidentLength < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "coincidentLength"};
    return std::nullopt;
}

void FastenerPropFormat::write(const FastenerPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.diameter);
    writer.integerUnlessDefault(3, card.matFrameId, FastenerPropCard::NO_FRAME);
    writer.integerUnlessDefault(4, card.absoluteFrame, 0);
    for (std::size_t index = 0; index < 3; ++index)
        writer.real(5 + static_cast<int>(index), card.translationalStiffness[index]);
    for (std::size_t index = 0; index < 3; ++index)
        writer.realUnlessDefault(8 + static_cast<int>(index), card.rotationalStiffness[index], 0.0);
    writer.realUnlessDefault(11, card.mass, 0.0);
    writer.realUnlessDefault(12, card.structuralDamping, 0.0);
    writer.realUnlessDefault(13, card.alphaCoefficient, 0.0);
    writer.realUnlessDefault(14, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(15, card.coincidentLength, 0.0);
}

std::uint32_t FastenerPropTable::append(const FastenerPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_diameter.push(card.diameter);
    m_matFrameId.push(card.matFrameId);
    m_absoluteFrame.push(static_cast<std::uint8_t>(card.absoluteFrame));
    m_translationalStiffness.push(card.translationalStiffness);
    m_rotationalStiffness.push(card.rotationalStiffness);
    m_mass.push(card.mass);
    m_structuralDamping.push(card.structuralDamping);
    m_alphaCoefficient.push(card.alphaCoefficient);
    m_referenceTemperature.push(card.referenceTemperature);
    m_coincidentLength.push(card.coincidentLength);
    return row;
}

FastenerPropCard FastenerPropTable::card(std::uint32_t row) const
{
    FastenerPropCard card;
    card.id = m_id.get(row);
    card.diameter = m_diameter.get(row);
    card.matFrameId = m_matFrameId.get(row);
    card.absoluteFrame = m_absoluteFrame.get(row);
    card.translationalStiffness = m_translationalStiffness.get(row);
    card.rotationalStiffness = m_rotationalStiffness.get(row);
    card.mass = m_mass.get(row);
    card.structuralDamping = m_structuralDamping.get(row);
    card.alphaCoefficient = m_alphaCoefficient.get(row);
    card.referenceTemperature = m_referenceTemperature.get(row);
    card.coincidentLength = m_coincidentLength.get(row);
    return card;
}

void FastenerPropTable::store(std::uint32_t row, const FastenerPropCard& card)
{
    m_id.set(row, card.id);
    m_diameter.set(row, card.diameter);
    m_matFrameId.set(row, card.matFrameId);
    m_absoluteFrame.set(row, static_cast<std::uint8_t>(card.absoluteFrame));
    m_translationalStiffness.set(row, card.translationalStiffness);
    m_rotationalStiffness.set(row, card.rotationalStiffness);
    m_mass.set(row, card.mass);
    m_structuralDamping.set(row, card.structuralDamping);
    m_alphaCoefficient.set(row, card.alphaCoefficient);
    m_referenceTemperature.set(row, card.referenceTemperature);
    m_coincidentLength.set(row, card.coincidentLength);
}

void FastenerPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_diameter.keepRows(rows);
    m_matFrameId.keepRows(rows);
    m_absoluteFrame.keepRows(rows);
    m_translationalStiffness.keepRows(rows);
    m_rotationalStiffness.keepRows(rows);
    m_mass.keepRows(rows);
    m_structuralDamping.keepRows(rows);
    m_alphaCoefficient.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
    m_coincidentLength.keepRows(rows);
}

void FastenerPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // -1 is no system and 0 the basic one.
    if (const base::EntityId matFrameId = m_matFrameId.get(row); matFrameId > 0)
        m_matFrameId.set(row, visitor.visit("matFrameId", ReferenceTarget::Coordinate, matFrameId));
}

} // namespace oor::model
