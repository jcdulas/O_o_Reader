#include "oor/model/cards/nonlinear_contact/contactBodyMove/ContactBodyMoveTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

std::optional<ContactBodyMoveCard> ContactBodyMoveFormat::read(FieldReader& reader)
{
    ContactBodyMoveCard card;
    card.id = reader.requiredInteger(1, "id");
    card.moveKind = readKeyword(reader, 2, "moveKind", {"Approach", "Release", "Synchronous"}, "Approach");
    card.bodies = readContactIds(reader, 3, reader.lastUsedField(), "bodies");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactBodyMoveFormat::check(const ContactBodyMoveCard& card)
{
    if (card.id < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "id"};
    if (auto problem = checkKeyword(card.moveKind, "moveKind", {"Approach", "Release", "Synchronous"}))
        return problem;
    // with APPROACH and SYNCHRON the fields after MTYPE stay blank.
    if (card.moveKind != "Release" && !card.bodies.empty())
        return CardProblem{"card.bad-value", "the bodies are given for RELEASE only", "bodies"};
    if (card.moveKind == "Release" && card.bodies.empty())
        return CardProblem{"card.missing-field", "RELEASE needs the bodies to release", "bodies"};
    for (const base::EntityId body : card.bodies)
    {
        if (body <= 0)
            return CardProblem{"card.bad-id", "a body ID must be a positive integer", "bodies"};
    }
    return std::nullopt;
}

void ContactBodyMoveFormat::write(const ContactBodyMoveCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, card.moveKind, "Approach");
    for (std::size_t index = 0; index < card.bodies.size(); ++index)
        writer.integer(3 + static_cast<int>(index), card.bodies[index]);
}

std::uint32_t ContactBodyMoveTable::append(const ContactBodyMoveCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_moveKind.push(card.moveKind);
    m_bodies.push(card.bodies);
    return row;
}

ContactBodyMoveCard ContactBodyMoveTable::card(std::uint32_t row) const
{
    ContactBodyMoveCard card;
    card.id = m_id.get(row);
    card.moveKind = m_moveKind.get(row);
    const auto bodies = m_bodies.get(row);
    card.bodies.assign(bodies.begin(), bodies.end());
    return card;
}

void ContactBodyMoveTable::store(std::uint32_t row, const ContactBodyMoveCard& card)
{
    m_id.set(row, card.id);
    m_moveKind.set(row, card.moveKind);
    m_bodies.set(row, card.bodies);
}

void ContactBodyMoveTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_moveKind.keepRows(rows);
    m_bodies.keepRows(rows);
}

void ContactBodyMoveTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactIdColumn(m_bodies, row, visitor, "bodies", ReferenceTarget::ContactBody);
}

} // namespace oor::model
