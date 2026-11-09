#include "oor/model/cards/nonlinear_contact/contactBeamCap/ContactBeamCapTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

std::optional<ContactBeamCapCard> ContactBeamCapFormat::read(FieldReader& reader)
{
    ContactBeamCapCard card;
    card.id = reader.requiredInteger(1, "id");
    card.capKind = reader.integer(2, "capKind", 0);
    card.segmentCount = reader.integer(3, "segmentCount", 32);
    requireUnusedBlank(reader, {}, 3);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactBeamCapFormat::check(const ContactBeamCapCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive element ID", "id"};
    if (card.capKind < 0 || card.capKind > 5)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 to 5", "capKind"};
    if (card.segmentCount < 3)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be at least 3", "segmentCount"};
    return std::nullopt;
}

void ContactBeamCapFormat::write(const ContactBeamCapCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.capKind, 0);
    writer.integerUnlessDefault(3, card.segmentCount, 32);
}

std::uint32_t ContactBeamCapTable::append(const ContactBeamCapCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_capKind.push(card.capKind);
    m_segmentCount.push(card.segmentCount);
    return row;
}

ContactBeamCapCard ContactBeamCapTable::card(std::uint32_t row) const
{
    ContactBeamCapCard card;
    card.id = m_id.get(row);
    card.capKind = m_capKind.get(row);
    card.segmentCount = m_segmentCount.get(row);
    return card;
}

void ContactBeamCapTable::store(std::uint32_t row, const ContactBeamCapCard& card)
{
    m_id.set(row, card.id);
    m_capKind.set(row, card.capKind);
    m_segmentCount.set(row, card.segmentCount);
}

void ContactBeamCapTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_capKind.keepRows(rows);
    m_segmentCount.keepRows(rows);
}

void ContactBeamCapTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactIdAt(m_id, row, visitor, "id", ReferenceTarget::Element);
}

} // namespace oor::model
