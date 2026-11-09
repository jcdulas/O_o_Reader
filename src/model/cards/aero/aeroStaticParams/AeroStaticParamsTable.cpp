#include "oor/model/cards/aero/aeroStaticParams/AeroStaticParamsTable.hpp"

#include "oor/model/cards/aero/SymmetryKey.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/Model.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 7;

} // namespace

std::optional<AeroStaticParamsCard> AeroStaticParamsFormat::read(FieldReader& reader)
{
    AeroStaticParamsCard card;
    card.aeroFrameId = reader.integer(1, "aeroFrameId", 0);
    card.referenceFrameId = reader.integer(2, "referenceFrameId", 0);
    card.referenceChord = reader.requiredReal(3, "referenceChord");
    card.referenceSpan = reader.requiredReal(4, "referenceSpan");
    card.referenceArea = reader.requiredReal(5, "referenceArea");
    card.symmetricXz = reader.integer(6, "symmetricXz", 0);
    card.symmetricXy = reader.integer(7, "symmetricXy", 0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroStaticParamsFormat::check(const AeroStaticParamsCard& card)
{
    if (const auto problem = checkOptionalId(card.aeroFrameId, "aeroFrameId"))
        return problem;
    if (const auto problem = checkOptionalId(card.referenceFrameId, "referenceFrameId"))
        return problem;
    if (const auto problem = checkReal(card.referenceChord, "referenceChord", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.referenceSpan, "referenceSpan", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.referenceArea, "referenceArea", RealRule::Positive))
        return problem;
    if (const auto problem = checkSymmetryKey(card.symmetricXz, "symmetricXz"))
        return problem;
    return checkSymmetryKey(card.symmetricXy, "symmetricXy");
}

void AeroStaticParamsFormat::write(const AeroStaticParamsCard& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.aeroFrameId, 0);
    writer.integerUnlessDefault(2, card.referenceFrameId, 0);
    writer.real(3, card.referenceChord);
    writer.real(4, card.referenceSpan);
    writer.real(5, card.referenceArea);
    writer.integerUnlessDefault(6, card.symmetricXz, 0);
    writer.integerUnlessDefault(7, card.symmetricXy, 0);
}

std::uint32_t AeroStaticParamsTable::append(const AeroStaticParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_aeroFrameId.push(card.aeroFrameId);
    m_referenceFrameId.push(card.referenceFrameId);
    m_referenceChord.push(card.referenceChord);
    m_referenceSpan.push(card.referenceSpan);
    m_referenceArea.push(card.referenceArea);
    m_symmetricXz.push(card.symmetricXz);
    m_symmetricXy.push(card.symmetricXy);
    return row;
}

AeroStaticParamsCard AeroStaticParamsTable::card(std::uint32_t row) const
{
    AeroStaticParamsCard card;
    card.aeroFrameId = m_aeroFrameId.get(row);
    card.referenceFrameId = m_referenceFrameId.get(row);
    card.referenceChord = m_referenceChord.get(row);
    card.referenceSpan = m_referenceSpan.get(row);
    card.referenceArea = m_referenceArea.get(row);
    card.symmetricXz = m_symmetricXz.get(row);
    card.symmetricXy = m_symmetricXy.get(row);
    return card;
}

void AeroStaticParamsTable::store(std::uint32_t row, const AeroStaticParamsCard& card)
{
    m_aeroFrameId.set(row, card.aeroFrameId);
    m_referenceFrameId.set(row, card.referenceFrameId);
    m_referenceChord.set(row, card.referenceChord);
    m_referenceSpan.set(row, card.referenceSpan);
    m_referenceArea.set(row, card.referenceArea);
    m_symmetricXz.set(row, card.symmetricXz);
    m_symmetricXy.set(row, card.symmetricXy);
}

void AeroStaticParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_aeroFrameId.keepRows(rows);
    m_referenceFrameId.keepRows(rows);
    m_referenceChord.keepRows(rows);
    m_referenceSpan.keepRows(rows);
    m_referenceArea.keepRows(rows);
    m_symmetricXz.keepRows(rows);
    m_symmetricXy.keepRows(rows);
}

void AeroStaticParamsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // 0 is the basic coordinate system: nothing to follow.
    if (const base::EntityId aeroFrameId = m_aeroFrameId.get(row); aeroFrameId > 0)
        m_aeroFrameId.set(row, visitor.visit("aeroFrameId", ReferenceTarget::Coordinate, aeroFrameId));
    if (const base::EntityId referenceFrameId = m_referenceFrameId.get(row); referenceFrameId > 0)
        m_referenceFrameId.set(row, visitor.visit("referenceFrameId", ReferenceTarget::Coordinate, referenceFrameId));
}

void AeroStaticParamsTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-aero-static-params",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
