#include "oor/model/cards/nonlinear_contact/contactSlideline/ContactSlidelineTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>

namespace oor::model
{

std::optional<ContactSlidelineCard> ContactSlidelineFormat::read(FieldReader& reader)
{
    ContactSlidelineCard card;
    card.id = reader.requiredInteger(1, "id");
    card.secondaryLineId = reader.requiredInteger(2, "secondaryLineId");
    card.primaryLineId = reader.requiredInteger(3, "primaryLineId");
    requireUnusedBlank(reader, {4}, 8);
    card.penaltyScale = reader.real(5, "penaltyScale", 1.0);
    card.frictionId = reader.integer(6, "frictionId", 0);
    card.penetrationKind = reader.integer(7, "penetrationKind", 1);
    card.frameId = reader.integer(8, "frameId", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactSlidelineFormat::check(const ContactSlidelineCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.secondaryLineId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be the ID of a slideline",
                           "secondaryLineId"};
    if (card.primaryLineId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be the ID of a slideline",
                           "primaryLineId"};
    if (!(card.penaltyScale > 0.0) || !std::isfinite(card.penaltyScale))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "penaltyScale"};
    if (card.frictionId < 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be the ID of a slideline friction, or blank",
                           "frictionId"};
    if (card.penetrationKind != 1 && card.penetrationKind != 2)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 or 2", "penetrationKind"};
    if (card.frameId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a coordinate system ID",
                           "frameId"};
    return std::nullopt;
}

void ContactSlidelineFormat::write(const ContactSlidelineCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.secondaryLineId);
    writer.integer(3, card.primaryLineId);
    writer.realUnlessDefault(5, card.penaltyScale, 1.0);
    writer.integerUnlessDefault(6, card.frictionId, 0);
    writer.integerUnlessDefault(7, card.penetrationKind, 1);
    writer.integerUnlessDefault(8, card.frameId, 0);
}

std::uint32_t ContactSlidelineTable::append(const ContactSlidelineCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_secondaryLineId.push(card.secondaryLineId);
    m_primaryLineId.push(card.primaryLineId);
    m_penaltyScale.push(card.penaltyScale);
    m_frictionId.push(card.frictionId);
    m_penetrationKind.push(card.penetrationKind);
    m_frameId.push(card.frameId);
    return row;
}

ContactSlidelineCard ContactSlidelineTable::card(std::uint32_t row) const
{
    ContactSlidelineCard card;
    card.id = m_id.get(row);
    card.secondaryLineId = m_secondaryLineId.get(row);
    card.primaryLineId = m_primaryLineId.get(row);
    card.penaltyScale = m_penaltyScale.get(row);
    card.frictionId = m_frictionId.get(row);
    card.penetrationKind = m_penetrationKind.get(row);
    card.frameId = m_frameId.get(row);
    return card;
}

void ContactSlidelineTable::store(std::uint32_t row, const ContactSlidelineCard& card)
{
    m_id.set(row, card.id);
    m_secondaryLineId.set(row, card.secondaryLineId);
    m_primaryLineId.set(row, card.primaryLineId);
    m_penaltyScale.set(row, card.penaltyScale);
    m_frictionId.set(row, card.frictionId);
    m_penetrationKind.set(row, card.penetrationKind);
    m_frameId.set(row, card.frameId);
}

void ContactSlidelineTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_secondaryLineId.keepRows(rows);
    m_primaryLineId.keepRows(rows);
    m_penaltyScale.keepRows(rows);
    m_frictionId.keepRows(rows);
    m_penetrationKind.keepRows(rows);
    m_frameId.keepRows(rows);
}

void ContactSlidelineTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto visit = [&](Column<base::EntityId>& column, const char* field, ReferenceTarget target)
    {
        const base::EntityId id = column.get(row);
        if (id == 0)
            return;
        const base::EntityId updated = visitor.visit(field, target, id);
        if (updated != id)
            column.set(row, updated);
    };
    visit(m_secondaryLineId, "secondaryLineId", ReferenceTarget::SlidelineSegments);
    visit(m_primaryLineId, "primaryLineId", ReferenceTarget::SlidelineSegments);
    visit(m_frictionId, "frictionId", ReferenceTarget::SlidelineFriction);
    visit(m_frameId, "frameId", ReferenceTarget::Coordinate);
}

} // namespace oor::model
