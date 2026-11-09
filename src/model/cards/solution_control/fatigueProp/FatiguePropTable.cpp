#include "oor/model/cards/solution_control/fatigueProp/FatiguePropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

std::optional<FatiguePropCard> FatiguePropFormat::read(FieldReader& reader)
{
    FatiguePropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.surfaceFinish = readKeyword(reader, 3, "surfaceFinish", FATIGUE_PROP_FINISH_WORDS, FatiguePropFinish::None);
    card.finishFactor = reader.optionalReal(4, "finishFactor");
    card.fatigueNotchFactor = reader.real(5, "fatigueNotchFactor", 1.0);
    card.scaleFactor = reader.real(6, "scaleFactor", 1.0);
    card.offset = reader.real(7, "offset", 0.0);
    // The first field of the continuation line is blank in the drawing of the format, and KTREAT is in its field 3.
    card.treatmentFactor = reader.real(10, "treatmentFactor", 1.0);
    card.diameter = reader.optionalReal(11, "diameter");
    card.thickness1 = reader.optionalReal(12, "thickness1");
    card.thickness2 = reader.optionalReal(13, "thickness2");
    card.spotWeldFlag = reader.integer(14, "spotWeldFlag", 0);
    requireBlankFields(reader, {2, 8, 9, 15, 16}, 16);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FatiguePropFormat::check(const FatiguePropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    const bool roughnessFinish =
        card.surfaceFinish == FatiguePropFinish::Krough || card.surfaceFinish == FatiguePropFinish::Ksurfc;
    if (roughnessFinish && !card.finishFactor)
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " is required when " +
                               CardProblem::fieldNamed("surfaceFinish") + " is KROUGH or KSURFC",
                           "finishFactor"};
    if (const auto problem = checkOptionalReal(card.finishFactor, "finishFactor", RealRule::Finite))
        return problem;
    if (card.finishFactor && card.surfaceFinish == FatiguePropFinish::Krough &&
        !(*card.finishFactor > 0.0 && *card.finishFactor < 1.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be greater than 0 and less than 1 when " +
                               CardProblem::fieldNamed("surfaceFinish") + " is KROUGH",
                           "finishFactor"};
    if (card.finishFactor && card.surfaceFinish == FatiguePropFinish::Ksurfc && *card.finishFactor < 0.0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must not be negative when " +
                               CardProblem::fieldNamed("surfaceFinish") + " is KSURFC",
                           "finishFactor"};
    if (const auto problem = checkReal(card.fatigueNotchFactor, "fatigueNotchFactor", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.scaleFactor, "scaleFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.offset, "offset", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.treatmentFactor, "treatmentFactor", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalReal(card.diameter, "diameter", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.thickness1, "thickness1", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.thickness2, "thickness2", RealRule::Positive))
        return problem;
    if (card.thickness1.has_value() != card.thickness2.has_value())
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("thickness2") +
                               " must both be given or both be blank",
                           card.thickness1 ? "thickness2" : "thickness1"};
    if (card.spotWeldFlag != 0 && card.spotWeldFlag != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "spotWeldFlag"};
    return std::nullopt;
}

void FatiguePropFormat::write(const FatiguePropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 3, FATIGUE_PROP_FINISH_WORDS, card.surfaceFinish, FatiguePropFinish::None);
    if (card.finishFactor)
        writer.real(4, *card.finishFactor);
    writer.realUnlessDefault(5, card.fatigueNotchFactor, 1.0);
    writer.realUnlessDefault(6, card.scaleFactor, 1.0);
    writer.realUnlessDefault(7, card.offset, 0.0);
    writer.realUnlessDefault(10, card.treatmentFactor, 1.0);
    if (card.diameter)
        writer.real(11, *card.diameter);
    if (card.thickness1)
        writer.real(12, *card.thickness1);
    if (card.thickness2)
        writer.real(13, *card.thickness2);
    writer.integerUnlessDefault(14, card.spotWeldFlag, 0);
}

std::uint32_t FatiguePropTable::append(const FatiguePropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FatiguePropTable::store(std::uint32_t row, const FatiguePropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FatiguePropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FatiguePropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
