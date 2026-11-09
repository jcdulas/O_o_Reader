#include "oor/model/cards/explicit/hourglassSuppression/HourglassSuppressionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

namespace oor::model
{

std::optional<HourglassSuppressionCard> HourglassSuppressionFormat::read(FieldReader& reader)
{
    HourglassSuppressionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propKind = readRequiredKeyword<HourglassSuppressionProperty>(reader, 2, "propKind", HOURGLASS_PROPERTY_WORDS);
    card.propId = reader.requiredInteger(3, "propId");
    card.method = readOptionalKeyword<HourglassMethod>(reader, 4, "method", HOURGLASS_METHOD_WORDS);
    if (!card.method)
        card.method = defaultHourglassMethod(card.propKind);
    card.membraneCoefficient = reader.real(5, "membraneCoefficient", HourglassSuppressionCard::DEFAULT_COEFFICIENT);
    card.warpCoefficient = reader.real(6, "warpCoefficient", HourglassSuppressionCard::DEFAULT_COEFFICIENT);
    card.twistCoefficient = reader.real(7, "twistCoefficient", HourglassSuppressionCard::DEFAULT_COEFFICIENT);
    card.solidCoefficient = reader.real(8, "solidCoefficient", HourglassSuppressionCard::DEFAULT_COEFFICIENT);
    card.rotationCorrection =
        readKeyword(reader, 9, "rotationCorrection", ROTATION_CORRECTION_WORDS, RotationCorrection::Off);
    card.stepCount = reader.integer(10, "stepCount", 0);
    if (reader.lastUsedField() > 10)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 10 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HourglassSuppressionFormat::check(const HourglassSuppressionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (defaultHourglassMethod(card.propKind) && !card.method)
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) +
                               " is required for a shell or solid property: it holds the method a blank field means",
                           "method"};
    if (const auto problem = checkRealRange(card.membraneCoefficient, "membraneCoefficient", 0.0,
                                            HourglassSuppressionCard::MAX_COEFFICIENT))
        return problem;
    if (const auto problem =
            checkRealRange(card.warpCoefficient, "warpCoefficient", 0.0, HourglassSuppressionCard::MAX_COEFFICIENT))
        return problem;
    if (const auto problem =
            checkRealRange(card.twistCoefficient, "twistCoefficient", 0.0, HourglassSuppressionCard::MAX_COEFFICIENT))
        return problem;
    if (const auto problem =
            checkRealRange(card.solidCoefficient, "solidCoefficient", 0.0, HourglassSuppressionCard::MAX_COEFFICIENT))
        return problem;
    if (card.stepCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero, or blank",
                           "stepCount"};
    return std::nullopt;
}

void HourglassSuppressionFormat::write(const HourglassSuppressionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeyword(writer, 2, HOURGLASS_PROPERTY_WORDS, card.propKind);
    writer.integer(3, card.propId);
    writeOptionalKeyword(writer, 4, HOURGLASS_METHOD_WORDS, card.method);
    writer.realUnlessDefault(5, card.membraneCoefficient, HourglassSuppressionCard::DEFAULT_COEFFICIENT);
    writer.realUnlessDefault(6, card.warpCoefficient, HourglassSuppressionCard::DEFAULT_COEFFICIENT);
    writer.realUnlessDefault(7, card.twistCoefficient, HourglassSuppressionCard::DEFAULT_COEFFICIENT);
    writer.realUnlessDefault(8, card.solidCoefficient, HourglassSuppressionCard::DEFAULT_COEFFICIENT);
    writeKeywordUnlessDefault(writer, 9, ROTATION_CORRECTION_WORDS, card.rotationCorrection, RotationCorrection::Off);
    writer.integerUnlessDefault(10, card.stepCount, 0);
}

std::uint32_t HourglassSuppressionTable::append(const HourglassSuppressionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void HourglassSuppressionTable::store(std::uint32_t row, const HourglassSuppressionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void HourglassSuppressionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void HourglassSuppressionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void HourglassSuppressionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "propId", ReferenceTarget::Property, m_cards.edit(row).propId);
}

} // namespace oor::model
