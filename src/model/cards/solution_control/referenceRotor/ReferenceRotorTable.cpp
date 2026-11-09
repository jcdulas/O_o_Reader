#include "oor/model/cards/solution_control/referenceRotor/ReferenceRotorTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

// The rotors go up, with an increment that BY may give.
constexpr ContactRunRules RULES{.allowBy = true, .allowDescending = false};

constexpr int FIRST_ROTOR_FIELD = 3;

} // namespace

std::optional<ReferenceRotorCard> ReferenceRotorFormat::read(FieldReader& reader)
{
    ReferenceRotorCard card;
    card.id = reader.requiredInteger(1, "id");
    card.referenceRotorId = reader.requiredInteger(2, "referenceRotorId");
    card.rotors = readContactRuns(reader, FIRST_ROTOR_FIELD, reader.lastUsedField(), "nonFollowingRotorIds", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ReferenceRotorFormat::check(const ReferenceRotorCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.referenceRotorId, "referenceRotorId"))
        return problem;
    if (card.rotors.empty())
        return CardProblem{"card.missing-field", "the card needs at least one non-reference rotor",
                           "nonFollowingRotorIds"};
    return checkContactRuns(card.rotors, "nonFollowingRotorIds", RULES);
}

void ReferenceRotorFormat::write(const ReferenceRotorCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.referenceRotorId);
    writeContactRuns(writer, FIRST_ROTOR_FIELD, card.rotors);
}

std::uint32_t ReferenceRotorTable::append(const ReferenceRotorCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_referenceRotorId.push(card.referenceRotorId);
    m_cards.push(card);
    return row;
}

void ReferenceRotorTable::store(std::uint32_t row, const ReferenceRotorCard& card)
{
    m_id.set(row, card.id);
    m_referenceRotorId.set(row, card.referenceRotorId);
    m_cards.set(row, card);
}

void ReferenceRotorTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_referenceRotorId.keepRows(rows);
    m_cards.keepRows(rows);
}

void ReferenceRotorTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ReferenceRotorTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ReferenceRotorCard& card = m_cards.edit(row);
    visitId(visitor, "referenceRotorId", ReferenceTarget::Rotor, card.referenceRotorId);
    m_referenceRotorId.set(row, card.referenceRotorId);
    visitContactRuns(card.rotors, visitor, "nonFollowingRotorIds", ReferenceTarget::Rotor, ReferenceTarget::Rotor);
}

} // namespace oor::model
