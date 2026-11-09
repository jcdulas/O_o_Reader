#include "oor/model/cards/explicit/initialSurfaceVolume/InitialSurfaceVolumeTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<InitialSurfaceVolumeCard> InitialSurfaceVolumeFormat::read(FieldReader& reader)
{
    InitialSurfaceVolumeCard card;
    card.id = reader.requiredInteger(1, "id");
    card.surfaceId = reader.requiredInteger(2, "surfaceId");
    card.cover = readKeyword(reader, 3, "cover", COVER_SIDE_WORDS, CoverSide::Inside);
    card.reverse = readKeyword(reader, 4, "reverseNormals", SWITCH_WORDS, Switch::On);
    card.check = readKeyword(reader, 5, "checkNormals", SWITCH_WORDS, Switch::On);
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> InitialSurfaceVolumeFormat::check(const InitialSurfaceVolumeCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.surfaceId, "surfaceId"))
        return problem;
    // NONE is a word of the COVER keyword of a COUPLE, not of a SURFINI.
    if (card.cover == CoverSide::None)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be INSIDE or OUTSIDE", "cover"};
    return std::nullopt;
}

void InitialSurfaceVolumeFormat::write(const InitialSurfaceVolumeCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.surfaceId);
    writeKeywordUnlessDefault(writer, 3, COVER_SIDE_WORDS, card.cover, CoverSide::Inside);
    writeKeywordUnlessDefault(writer, 4, SWITCH_WORDS, card.reverse, Switch::On);
    writeKeywordUnlessDefault(writer, 5, SWITCH_WORDS, card.check, Switch::On);
}

std::uint32_t InitialSurfaceVolumeTable::append(const InitialSurfaceVolumeCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void InitialSurfaceVolumeTable::store(std::uint32_t row, const InitialSurfaceVolumeCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void InitialSurfaceVolumeTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void InitialSurfaceVolumeTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void InitialSurfaceVolumeTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "surfaceId", ReferenceTarget::ContactSurface, m_cards.edit(row).surfaceId);
}

} // namespace oor::model
