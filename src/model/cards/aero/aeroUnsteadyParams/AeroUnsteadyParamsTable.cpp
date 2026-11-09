#include "oor/model/cards/aero/aeroUnsteadyParams/AeroUnsteadyParamsTable.hpp"

#include "oor/model/cards/aero/BlankReal.hpp"
#include "oor/model/cards/aero/SymmetryKey.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/Model.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 6;

} // namespace

AeroUnsteadyParamsTable::AeroUnsteadyParamsTable() : m_velocity(BLANK_REAL) {}

std::optional<AeroUnsteadyParamsCard> AeroUnsteadyParamsFormat::read(FieldReader& reader)
{
    AeroUnsteadyParamsCard card;
    card.aeroFrameId = reader.integer(1, "aeroFrameId", 0);
    card.velocity = reader.optionalReal(2, "velocity");
    card.referenceChord = reader.requiredReal(3, "referenceChord");
    card.referenceDensity = reader.requiredReal(4, "referenceDensity");
    card.symmetricXz = reader.integer(5, "symmetricXz", 0);
    card.symmetricXy = reader.integer(6, "symmetricXy", 0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 6 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroUnsteadyParamsFormat::check(const AeroUnsteadyParamsCard& card)
{
    if (const auto problem = checkOptionalId(card.aeroFrameId, "aeroFrameId"))
        return problem;
    if (const auto problem = checkOptionalReal(card.velocity, "velocity", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.referenceChord, "referenceChord", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.referenceDensity, "referenceDensity", RealRule::Positive))
        return problem;
    if (const auto problem = checkSymmetryKey(card.symmetricXz, "symmetricXz"))
        return problem;
    return checkSymmetryKey(card.symmetricXy, "symmetricXy");
}

void AeroUnsteadyParamsFormat::write(const AeroUnsteadyParamsCard& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.aeroFrameId, 0);
    if (card.velocity)
        writer.real(2, *card.velocity);
    writer.real(3, card.referenceChord);
    writer.real(4, card.referenceDensity);
    writer.integerUnlessDefault(5, card.symmetricXz, 0);
    writer.integerUnlessDefault(6, card.symmetricXy, 0);
}

std::uint32_t AeroUnsteadyParamsTable::append(const AeroUnsteadyParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_aeroFrameId.push(card.aeroFrameId);
    m_velocity.push(realToColumn(card.velocity));
    m_referenceChord.push(card.referenceChord);
    m_referenceDensity.push(card.referenceDensity);
    m_symmetricXz.push(card.symmetricXz);
    m_symmetricXy.push(card.symmetricXy);
    return row;
}

AeroUnsteadyParamsCard AeroUnsteadyParamsTable::card(std::uint32_t row) const
{
    AeroUnsteadyParamsCard card;
    card.aeroFrameId = m_aeroFrameId.get(row);
    card.velocity = realFromColumn(m_velocity.get(row));
    card.referenceChord = m_referenceChord.get(row);
    card.referenceDensity = m_referenceDensity.get(row);
    card.symmetricXz = m_symmetricXz.get(row);
    card.symmetricXy = m_symmetricXy.get(row);
    return card;
}

void AeroUnsteadyParamsTable::store(std::uint32_t row, const AeroUnsteadyParamsCard& card)
{
    m_aeroFrameId.set(row, card.aeroFrameId);
    m_velocity.set(row, realToColumn(card.velocity));
    m_referenceChord.set(row, card.referenceChord);
    m_referenceDensity.set(row, card.referenceDensity);
    m_symmetricXz.set(row, card.symmetricXz);
    m_symmetricXy.set(row, card.symmetricXy);
}

void AeroUnsteadyParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_aeroFrameId.keepRows(rows);
    m_velocity.keepRows(rows);
    m_referenceChord.keepRows(rows);
    m_referenceDensity.keepRows(rows);
    m_symmetricXz.keepRows(rows);
    m_symmetricXy.keepRows(rows);
}

void AeroUnsteadyParamsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // 0 is the basic coordinate system: nothing to follow.
    if (const base::EntityId aeroFrameId = m_aeroFrameId.get(row); aeroFrameId > 0)
        m_aeroFrameId.set(row, visitor.visit("aeroFrameId", ReferenceTarget::Coordinate, aeroFrameId));
}

void AeroUnsteadyParamsTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-aero-unsteady-params",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
