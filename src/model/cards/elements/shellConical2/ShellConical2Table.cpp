#include "oor/model/cards/elements/shellConical2/ShellConical2Table.hpp"

namespace oor::model
{

std::optional<ShellConical2Card> ShellConical2Format::read(FieldReader& reader)
{
    ShellConical2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    card.ringIdA = reader.requiredInteger(3, "ringIdA");
    card.ringIdB = reader.requiredInteger(4, "ringIdB");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShellConical2Format::check(const ShellConical2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.ringIdA <= 0 || card.ringIdB <= 0)
        return CardProblem{"card.bad-id", "the ring IDs must be positive integers", "ringIdA"};
    if (card.ringIdA == card.ringIdB)
        return CardProblem{"card.duplicate-node", "{field:ringIdA} and {field} must be different", "ringIdB"};
    return std::nullopt;
}

void ShellConical2Format::write(const ShellConical2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.ringIdA);
    writer.integer(4, card.ringIdB);
}

std::uint32_t ShellConical2Table::append(const ShellConical2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_ringIdA.push(card.ringIdA);
    m_ringIdB.push(card.ringIdB);
    return row;
}

ShellConical2Card ShellConical2Table::card(std::uint32_t row) const
{
    return ShellConical2Card{m_id.get(row), m_propId.get(row), m_ringIdA.get(row), m_ringIdB.get(row)};
}

void ShellConical2Table::store(std::uint32_t row, const ShellConical2Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_ringIdA.set(row, card.ringIdA);
    m_ringIdB.set(row, card.ringIdB);
}

void ShellConical2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_ringIdA.keepRows(rows);
    m_ringIdB.keepRows(rows);
}

void ShellConical2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    // RA and RB are RINGAX IDs, which are in the node ID space.
    m_ringIdA.set(row, visitor.visit("ringIdA", ReferenceTarget::Node, m_ringIdA.get(row)));
    m_ringIdB.set(row, visitor.visit("ringIdB", ReferenceTarget::Node, m_ringIdB.get(row)));
}

} // namespace oor::model
