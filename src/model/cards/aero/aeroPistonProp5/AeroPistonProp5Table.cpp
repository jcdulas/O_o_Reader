#include "oor/model/cards/aero/aeroPistonProp5/AeroPistonProp5Table.hpp"

#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr int UNUSED_FIELD = 8;                    // the format reference grid leaves it blank
constexpr int FIRST_CONTROL_CHORD_RATIO_FIELD = 9; // the control surface chords are the continuation line, with no gap

} // namespace

std::optional<AeroPistonProp5Card> AeroPistonProp5Format::read(FieldReader& reader)
{
    AeroPistonProp5Card card;
    card.id = reader.requiredInteger(1, "id");
    card.angleOfAttackCount = reader.requiredInteger(2, "angleOfAttackCount");
    card.angleOfAttackListId = reader.requiredInteger(3, "angleOfAttackListId");
    card.chordCoordCount = reader.integer(4, "chordCoordCount", 0);
    card.chordCoordListId = reader.integer(5, "chordCoordListId", 0);
    card.thicknessRatioCount = reader.integer(6, "thicknessRatioCount", 0);
    card.thicknessRatioListId = reader.integer(7, "thicknessRatioListId", 0);
    if (!reader.isBlank(UNUSED_FIELD))
        reader.error("card.bad-value",
                     "field 8 of a " + std::string(reader.cardName()) + " is not used and must be blank", "");
    card.controlChordRatios =
        readRealList(reader, FIRST_CONTROL_CHORD_RATIO_FIELD, reader.slotCount(), "controlChordRatios");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroPistonProp5Format::check(const AeroPistonProp5Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.angleOfAttackCount <= 0)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "angleOfAttackCount"};
    if (const auto problem = checkRequiredId(card.angleOfAttackListId, "angleOfAttackListId"))
        return problem;
    if (card.chordCoordCount < 0)
        return CardProblem{"card.bad-value", "{field} cannot be negative", "chordCoordCount"};
    if (const auto problem = checkOptionalId(card.chordCoordListId, "chordCoordListId"))
        return problem;
    if (card.chordCoordCount > 0 && card.chordCoordListId == 0)
        return CardProblem{"card.missing-field", "{field} is required when {field:chordCoordCount} is not 0",
                           "chordCoordListId"};
    if (card.thicknessRatioCount < 0)
        return CardProblem{"card.bad-value", "{field} cannot be negative", "thicknessRatioCount"};
    if (const auto problem = checkOptionalId(card.thicknessRatioListId, "thicknessRatioListId"))
        return problem;
    if (card.thicknessRatioCount > 0 && card.thicknessRatioListId == 0)
        return CardProblem{"card.missing-field", "{field} is required when {field:thicknessRatioCount} is not 0",
                           "thicknessRatioListId"};
    if (card.controlChordRatios.empty())
        return CardProblem{"card.missing-field", "the continuation with {field} is required", "controlChordRatio1"};
    for (const double value : card.controlChordRatios)
    {
        if (const auto problem = checkReal(value, "controlChordRatios", RealRule::NonNegative))
            return problem;
    }
    // A control surface needs the hinge position, which the AEFACT of the chord coordinates gives.
    const bool hasControlSurface = std::any_of(card.controlChordRatios.begin(), card.controlChordRatios.end(),
                                               [](double value) { return value != 0.0; });
    if (hasControlSurface && card.chordCoordListId == 0)
        return CardProblem{"card.missing-field",
                           "{field} is required when a strip has a control surface "
                           "({field:controlChordRatios} not 0.0)",
                           "chordCoordListId"};
    return std::nullopt;
}

void AeroPistonProp5Format::write(const AeroPistonProp5Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.angleOfAttackCount);
    writer.integer(3, card.angleOfAttackListId);
    writer.integerUnlessDefault(4, card.chordCoordCount, 0);
    writer.integerUnlessDefault(5, card.chordCoordListId, 0);
    writer.integerUnlessDefault(6, card.thicknessRatioCount, 0);
    writer.integerUnlessDefault(7, card.thicknessRatioListId, 0);
    // The continuation is required, so a chord of 0.0 (no control surface) is written too.
    writeRealList(writer, FIRST_CONTROL_CHORD_RATIO_FIELD, card.controlChordRatios);
}

std::uint32_t AeroPistonProp5Table::append(const AeroPistonProp5Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroPistonProp5Table::store(std::uint32_t row, const AeroPistonProp5Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroPistonProp5Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroPistonProp5Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroPistonProp5Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroPistonProp5Card& card = m_cards.edit(row);
    visitId(visitor, "angleOfAttackListId", ReferenceTarget::AeroFactor, card.angleOfAttackListId);
    visitId(visitor, "chordCoordListId", ReferenceTarget::AeroFactor, card.chordCoordListId);
    visitId(visitor, "thicknessRatioListId", ReferenceTarget::AeroFactor, card.thicknessRatioListId);
}

} // namespace oor::model
