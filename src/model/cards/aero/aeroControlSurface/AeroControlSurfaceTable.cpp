#include "oor/model/cards/aero/aeroControlSurface/AeroControlSurfaceTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 16;

// A limit of the hinge moment that is blank is NaN in its column.
double noLimit()
{
    return std::numeric_limits<double>::quiet_NaN();
}

std::optional<double> limitOf(double value)
{
    return std::isnan(value) ? std::nullopt : std::optional<double>(value);
}

} // namespace

AeroControlSurfaceTable::AeroControlSurfaceTable()
    : m_effectiveness(AeroControlSurfaceCard::DEFAULT_EFFECTIVENESS),
      m_referenceChord(AeroControlSurfaceCard::DEFAULT_REFERENCE_CHORD),
      m_referenceArea(AeroControlSurfaceCard::DEFAULT_REFERENCE_AREA),
      m_deflectionLowerLimit(AeroControlSurfaceCard::DEFAULT_DEFLECTION_LOWER_LIMIT),
      m_deflectionUpperLimit(AeroControlSurfaceCard::DEFAULT_DEFLECTION_UPPER_LIMIT),
      m_hingeMomentLowerLimit(noLimit()),
      m_hingeMomentUpperLimit(noLimit())
{
}

std::optional<AeroControlSurfaceCard> AeroControlSurfaceFormat::read(FieldReader& reader)
{
    AeroControlSurfaceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.label = std::string(reader.text(2));
    card.frameId1 = reader.requiredInteger(3, "frameId1");
    card.boxListId1 = reader.requiredInteger(4, "boxListId1");
    card.frameId2 = reader.integer(5, "frameId2", 0);
    card.boxListId2 = reader.integer(6, "boxListId2", 0);
    card.effectiveness = reader.real(7, "effectiveness", AeroControlSurfaceCard::DEFAULT_EFFECTIVENESS);
    card.linearDownwash = readKeyword(reader, 8, "linearDownwash", AERO_CONTROL_SURFACE_DOWNWASH_WORDS,
                                      AeroControlSurfaceDownwash::Computed);
    card.referenceChord = reader.real(9, "referenceChord", AeroControlSurfaceCard::DEFAULT_REFERENCE_CHORD);
    card.referenceArea = reader.real(10, "referenceArea", AeroControlSurfaceCard::DEFAULT_REFERENCE_AREA);
    card.deflectionLowerLimit =
        reader.real(11, "deflectionLowerLimit", AeroControlSurfaceCard::DEFAULT_DEFLECTION_LOWER_LIMIT);
    card.deflectionUpperLimit =
        reader.real(12, "deflectionUpperLimit", AeroControlSurfaceCard::DEFAULT_DEFLECTION_UPPER_LIMIT);
    card.hingeMomentLowerLimit = reader.optionalReal(13, "hingeMomentLowerLimit");
    card.hingeMomentUpperLimit = reader.optionalReal(14, "hingeMomentUpperLimit");
    card.lowerLimitTableId = reader.integer(15, "lowerLimitTableId", 0);
    card.upperLimitTableId = reader.integer(16, "upperLimitTableId", 0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 16 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroControlSurfaceFormat::check(const AeroControlSurfaceCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.label, "label"))
        return problem;
    if (const auto problem = checkRequiredId(card.frameId1, "frameId1"))
        return problem;
    if (const auto problem = checkRequiredId(card.boxListId1, "boxListId1"))
        return problem;
    if (const auto problem = checkOptionalId(card.frameId2, "frameId2"))
        return problem;
    if (const auto problem = checkOptionalId(card.boxListId2, "boxListId2"))
        return problem;
    // The second component is a pair, as the first one is: CID2 alone (not 0) has no boxes, ALID2 alone has no hinge
    // line.
    if (card.frameId2 > 0 && card.boxListId2 == 0)
        return CardProblem{"card.missing-field", "{field} is required with {field:frameId2}", "boxListId2"};
    if (card.boxListId2 > 0 && card.frameId2 == 0)
        return CardProblem{"card.missing-field", "{field} is required with {field:boxListId2}", "frameId2"};
    if (const auto problem = checkReal(card.effectiveness, "effectiveness", RealRule::Finite))
        return problem;
    if (card.effectiveness == 0.0)
        return CardProblem{"card.bad-value", "{field} cannot be zero", "effectiveness"};
    if (const auto problem = checkReal(card.referenceChord, "referenceChord", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.referenceArea, "referenceArea", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.deflectionLowerLimit, "deflectionLowerLimit", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.deflectionUpperLimit, "deflectionUpperLimit", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.hingeMomentLowerLimit, "hingeMomentLowerLimit", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.hingeMomentUpperLimit, "hingeMomentUpperLimit", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.lowerLimitTableId, "lowerLimitTableId"))
        return problem;
    if (const auto problem = checkOptionalId(card.upperLimitTableId, "upperLimitTableId"))
        return problem;
    // a position limit is the constant of PLLIM or PULIM or the table of TQLLIM or TQULIM. The constant
    // that is still the default counts as not given.
    if (card.lowerLimitTableId > 0 &&
        card.deflectionLowerLimit != AeroControlSurfaceCard::DEFAULT_DEFLECTION_LOWER_LIMIT)
        return CardProblem{"card.bad-value", "{field} cannot be given together with {field:deflectionLowerLimit}",
                           "lowerLimitTableId"};
    if (card.upperLimitTableId > 0 &&
        card.deflectionUpperLimit != AeroControlSurfaceCard::DEFAULT_DEFLECTION_UPPER_LIMIT)
        return CardProblem{"card.bad-value", "{field} cannot be given together with {field:deflectionUpperLimit}",
                           "upperLimitTableId"};
    return std::nullopt;
}

void AeroControlSurfaceFormat::write(const AeroControlSurfaceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.label);
    writer.integer(3, card.frameId1);
    writer.integer(4, card.boxListId1);
    writer.integerUnlessDefault(5, card.frameId2, 0);
    writer.integerUnlessDefault(6, card.boxListId2, 0);
    writer.realUnlessDefault(7, card.effectiveness, AeroControlSurfaceCard::DEFAULT_EFFECTIVENESS);
    writeKeywordUnlessDefault(writer, 8, AERO_CONTROL_SURFACE_DOWNWASH_WORDS, card.linearDownwash,
                              AeroControlSurfaceDownwash::Computed);
    // The continuation line is optional: it is written only when one of its fields is not blank.
    writer.realUnlessDefault(9, card.referenceChord, AeroControlSurfaceCard::DEFAULT_REFERENCE_CHORD);
    writer.realUnlessDefault(10, card.referenceArea, AeroControlSurfaceCard::DEFAULT_REFERENCE_AREA);
    writer.realUnlessDefault(11, card.deflectionLowerLimit, AeroControlSurfaceCard::DEFAULT_DEFLECTION_LOWER_LIMIT);
    writer.realUnlessDefault(12, card.deflectionUpperLimit, AeroControlSurfaceCard::DEFAULT_DEFLECTION_UPPER_LIMIT);
    if (card.hingeMomentLowerLimit)
        writer.real(13, *card.hingeMomentLowerLimit);
    if (card.hingeMomentUpperLimit)
        writer.real(14, *card.hingeMomentUpperLimit);
    writer.integerUnlessDefault(15, card.lowerLimitTableId, 0);
    writer.integerUnlessDefault(16, card.upperLimitTableId, 0);
}

std::uint32_t AeroControlSurfaceTable::append(const AeroControlSurfaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_label.push(card.label);
    m_frameId1.push(card.frameId1);
    m_boxListId1.push(card.boxListId1);
    m_frameId2.push(card.frameId2);
    m_boxListId2.push(card.boxListId2);
    m_effectiveness.push(card.effectiveness);
    m_linearDownwash.push(static_cast<std::uint8_t>(card.linearDownwash));
    m_referenceChord.push(card.referenceChord);
    m_referenceArea.push(card.referenceArea);
    m_deflectionLowerLimit.push(card.deflectionLowerLimit);
    m_deflectionUpperLimit.push(card.deflectionUpperLimit);
    m_hingeMomentLowerLimit.push(card.hingeMomentLowerLimit.value_or(noLimit()));
    m_hingeMomentUpperLimit.push(card.hingeMomentUpperLimit.value_or(noLimit()));
    m_lowerLimitTableId.push(card.lowerLimitTableId);
    m_upperLimitTableId.push(card.upperLimitTableId);
    return row;
}

AeroControlSurfaceCard AeroControlSurfaceTable::card(std::uint32_t row) const
{
    AeroControlSurfaceCard card;
    card.id = m_id.get(row);
    card.label = m_label.get(row);
    card.frameId1 = m_frameId1.get(row);
    card.boxListId1 = m_boxListId1.get(row);
    card.frameId2 = m_frameId2.get(row);
    card.boxListId2 = m_boxListId2.get(row);
    card.effectiveness = m_effectiveness.get(row);
    card.linearDownwash = static_cast<AeroControlSurfaceDownwash>(m_linearDownwash.get(row));
    card.referenceChord = m_referenceChord.get(row);
    card.referenceArea = m_referenceArea.get(row);
    card.deflectionLowerLimit = m_deflectionLowerLimit.get(row);
    card.deflectionUpperLimit = m_deflectionUpperLimit.get(row);
    card.hingeMomentLowerLimit = limitOf(m_hingeMomentLowerLimit.get(row));
    card.hingeMomentUpperLimit = limitOf(m_hingeMomentUpperLimit.get(row));
    card.lowerLimitTableId = m_lowerLimitTableId.get(row);
    card.upperLimitTableId = m_upperLimitTableId.get(row);
    return card;
}

void AeroControlSurfaceTable::store(std::uint32_t row, const AeroControlSurfaceCard& card)
{
    m_id.set(row, card.id);
    m_label.set(row, card.label);
    m_frameId1.set(row, card.frameId1);
    m_boxListId1.set(row, card.boxListId1);
    m_frameId2.set(row, card.frameId2);
    m_boxListId2.set(row, card.boxListId2);
    m_effectiveness.set(row, card.effectiveness);
    m_linearDownwash.set(row, static_cast<std::uint8_t>(card.linearDownwash));
    m_referenceChord.set(row, card.referenceChord);
    m_referenceArea.set(row, card.referenceArea);
    m_deflectionLowerLimit.set(row, card.deflectionLowerLimit);
    m_deflectionUpperLimit.set(row, card.deflectionUpperLimit);
    m_hingeMomentLowerLimit.set(row, card.hingeMomentLowerLimit.value_or(noLimit()));
    m_hingeMomentUpperLimit.set(row, card.hingeMomentUpperLimit.value_or(noLimit()));
    m_lowerLimitTableId.set(row, card.lowerLimitTableId);
    m_upperLimitTableId.set(row, card.upperLimitTableId);
}

void AeroControlSurfaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_label.keepRows(rows);
    m_frameId1.keepRows(rows);
    m_boxListId1.keepRows(rows);
    m_frameId2.keepRows(rows);
    m_boxListId2.keepRows(rows);
    m_effectiveness.keepRows(rows);
    m_linearDownwash.keepRows(rows);
    m_referenceChord.keepRows(rows);
    m_referenceArea.keepRows(rows);
    m_deflectionLowerLimit.keepRows(rows);
    m_deflectionUpperLimit.keepRows(rows);
    m_hingeMomentLowerLimit.keepRows(rows);
    m_hingeMomentUpperLimit.keepRows(rows);
    m_lowerLimitTableId.keepRows(rows);
    m_upperLimitTableId.keepRows(rows);
}

void AeroControlSurfaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId frameId1 = m_frameId1.get(row); frameId1 > 0)
        m_frameId1.set(row, visitor.visit("frameId1", ReferenceTarget::Coordinate, frameId1));
    if (const base::EntityId boxListId1 = m_boxListId1.get(row); boxListId1 > 0)
        m_boxListId1.set(row, visitor.visit("boxListId1", ReferenceTarget::AeroList, boxListId1));
    if (const base::EntityId frameId2 = m_frameId2.get(row); frameId2 > 0)
        m_frameId2.set(row, visitor.visit("frameId2", ReferenceTarget::Coordinate, frameId2));
    if (const base::EntityId boxListId2 = m_boxListId2.get(row); boxListId2 > 0)
        m_boxListId2.set(row, visitor.visit("boxListId2", ReferenceTarget::AeroList, boxListId2));
    if (const base::EntityId lowerLimitTableId = m_lowerLimitTableId.get(row); lowerLimitTableId > 0)
        m_lowerLimitTableId.set(row,
                                visitor.visit("lowerLimitTableId", ReferenceTarget::DynamicTable, lowerLimitTableId));
    if (const base::EntityId upperLimitTableId = m_upperLimitTableId.get(row); upperLimitTableId > 0)
        m_upperLimitTableId.set(row,
                                visitor.visit("upperLimitTableId", ReferenceTarget::DynamicTable, upperLimitTableId));
}

} // namespace oor::model
