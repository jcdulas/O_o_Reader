#include "oor/model/cards/explicit/eulerBarrier/EulerBarrierTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int BOX_FIELD = 9;
constexpr int LAST_FIELD = 14;

} // namespace

std::optional<EulerBarrierCard> EulerBarrierFormat::read(FieldReader& reader)
{
    EulerBarrierCard card;
    card.id = reader.requiredInteger(1, "id");
    card.contactBodyId = reader.integer(2, "contactBodyId", 0);
    card.meshId = reader.integer(3, "meshId", 0);
    card.meshFace = readOptionalKeyword<BoundaryFace>(reader, 4, "meshFace", BOUNDARY_FACE_WORDS);
    card.skinFriction = reader.optionalReal(5, "skinFriction");
    card.box = readBoundaryBox(reader, BOX_FIELD);
    requireBlankFields(reader, {6, 7, 8}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerBarrierFormat::check(const EulerBarrierCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.contactBodyId, "contactBodyId"))
        return problem;
    if (const auto problem = checkOptionalId(card.meshId, "meshId"))
        return problem;
    if (const auto problem = checkOptionalReal(card.skinFriction, "skinFriction", RealRule::NonNegative))
        return problem;
    return checkBoundaryBox(card.box);
}

void EulerBarrierFormat::write(const EulerBarrierCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.contactBodyId, 0);
    writer.integerUnlessDefault(3, card.meshId, 0);
    writeOptionalKeyword(writer, 4, BOUNDARY_FACE_WORDS, card.meshFace);
    if (card.skinFriction)
        writer.real(5, *card.skinFriction);
    writeBoundaryBox(writer, BOX_FIELD, card.box);
}

std::uint32_t EulerBarrierTable::append(const EulerBarrierCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerBarrierTable::store(std::uint32_t row, const EulerBarrierCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerBarrierTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerBarrierTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerBarrierTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerBarrierCard& card = m_cards.edit(row);
    visitId(visitor, "contactBodyId", ReferenceTarget::ContactSurface, card.contactBodyId);
    visitId(visitor, "meshId", ReferenceTarget::EulerMesh, card.meshId);
}

} // namespace oor::model
