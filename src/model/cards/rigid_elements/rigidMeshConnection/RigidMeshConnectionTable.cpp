#include "oor/model/cards/rigid_elements/rigidMeshConnection/RigidMeshConnectionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int CLOSE_GAPS_FIELD = 17;
constexpr int GAP_MEASURE_FIELD = 18;
constexpr int GAP_VALUE_FIELD = 19;
constexpr int LAST_FIELD = 19;

} // namespace

std::optional<RigidMeshConnectionCard> RigidMeshConnectionFormat::read(FieldReader& reader)
{
    RigidMeshConnectionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.secondaryType = readKeyword<RigidMeshConnectionSecondary>(
        reader, 2, "secondaryType", RIGID_MESH_CONNECTION_SECONDARY_WORDS, RigidMeshConnectionSecondary::Surface);
    card.primaryType = readKeyword<RigidMeshConnectionPrimary>(
        reader, 3, "primaryType", RIGID_MESH_CONNECTION_PRIMARY_WORDS, RigidMeshConnectionPrimary::Surface);
    card.secondaryId = reader.requiredInteger(4, "secondaryId");
    card.primaryId = reader.requiredInteger(5, "primaryId");
    card.option = readKeyword<RigidMeshConnectionOption>(reader, 6, "option", RIGID_MESH_CONNECTION_OPTION_WORDS,
                                                         RigidMeshConnectionOption::Normal);
    card.closeGaps = readKeyword<RigidMeshConnectionGapClosing>(reader, CLOSE_GAPS_FIELD, "closeGaps",
                                                                RIGID_MESH_CONNECTION_GAP_CLOSING_WORDS,
                                                                RigidMeshConnectionGapClosing::No);
    card.gapMeasure = readKeyword<RigidMeshConnectionGapMeasure>(reader, GAP_MEASURE_FIELD, "gapMeasure",
                                                                 RIGID_MESH_CONNECTION_GAP_MEASURE_WORDS,
                                                                 RigidMeshConnectionGapMeasure::Distance);
    card.gapValue = reader.real(GAP_VALUE_FIELD, "gapValue", RigidMeshConnectionCard::DEFAULT_GAP_VALUE);
    // The format draws a blank cell after OPTION and a blank line before CLSGAP.
    requireBlankFields(reader, {7, 8, 9, 10, 11, 12, 13, 14, 15, 16}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidMeshConnectionFormat::check(const RigidMeshConnectionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.secondaryId, "secondaryId"))
        return problem;
    if (const auto problem = checkRequiredId(card.primaryId, "primaryId"))
        return problem;
    // The format gives no sign or range for the gap value, which is a length or a factor: any number.
    return checkReal(card.gapValue, "gapValue", RealRule::Finite);
}

void RigidMeshConnectionFormat::write(const RigidMeshConnectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, RIGID_MESH_CONNECTION_SECONDARY_WORDS, card.secondaryType,
                              RigidMeshConnectionSecondary::Surface);
    writeKeywordUnlessDefault(writer, 3, RIGID_MESH_CONNECTION_PRIMARY_WORDS, card.primaryType,
                              RigidMeshConnectionPrimary::Surface);
    writer.integer(4, card.secondaryId);
    writer.integer(5, card.primaryId);
    writeKeywordUnlessDefault(writer, 6, RIGID_MESH_CONNECTION_OPTION_WORDS, card.option,
                              RigidMeshConnectionOption::Normal);
    // The second line of the card is the blank one of the format: it is written, empty, when a field of the third has a
    // value.
    writeKeywordUnlessDefault(writer, CLOSE_GAPS_FIELD, RIGID_MESH_CONNECTION_GAP_CLOSING_WORDS, card.closeGaps,
                              RigidMeshConnectionGapClosing::No);
    writeKeywordUnlessDefault(writer, GAP_MEASURE_FIELD, RIGID_MESH_CONNECTION_GAP_MEASURE_WORDS, card.gapMeasure,
                              RigidMeshConnectionGapMeasure::Distance);
    writer.realUnlessDefault(GAP_VALUE_FIELD, card.gapValue, RigidMeshConnectionCard::DEFAULT_GAP_VALUE);
}

std::uint32_t RigidMeshConnectionTable::append(const RigidMeshConnectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void RigidMeshConnectionTable::store(std::uint32_t row, const RigidMeshConnectionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void RigidMeshConnectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void RigidMeshConnectionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void RigidMeshConnectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    RigidMeshConnectionCard& card = m_cards.edit(row);
    // With STYPE GRID, SID is a SET1 or a BCGRID: two kinds of card the card cannot tell apart, so it is left alone.
    if (card.secondaryType == RigidMeshConnectionSecondary::Surface)
        visitId(visitor, "secondaryId", ReferenceTarget::ContactSurface, card.secondaryId);
    visitId(visitor, "primaryId", ReferenceTarget::ContactSurface, card.primaryId);
}

} // namespace oor::model
