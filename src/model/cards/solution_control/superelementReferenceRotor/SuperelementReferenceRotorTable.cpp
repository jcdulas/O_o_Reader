#include "oor/model/cards/solution_control/superelementReferenceRotor/SuperelementReferenceRotorTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/RotorSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_PAIR_FIELD = 4;

} // namespace

std::optional<SuperelementReferenceRotorCard> SuperelementReferenceRotorFormat::read(FieldReader& reader)
{
    SuperelementReferenceRotorCard card;
    card.id = reader.requiredInteger(1, "id");
    card.referenceRotorId = reader.requiredInteger(2, "referenceRotorId");
    card.referenceSuperelementId = reader.integer(3, "referenceSuperelementId", 0);
    // The pairs follow each other with no break, so a pair may be split by the end of a line. A pair whose two fields
    // are blank is padding.
    const int last = reader.lastUsedField();
    for (int field = FIRST_PAIR_FIELD; field <= last; field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        RotorInSuperelement rotor;
        rotor.rotorId = reader.requiredInteger(field, "nonFollowingRotorIds");
        rotor.superelement = reader.integer(field + 1, "nonFollowingSuperelementIds", 0);
        card.rotors.push_back(rotor);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementReferenceRotorFormat::check(const SuperelementReferenceRotorCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.referenceRotorId, "referenceRotorId"))
        return problem;
    if (card.referenceSuperelementId < 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must not be negative (0 is the residual structure)",
                           "referenceSuperelementId"};
    if (card.rotors.empty())
        return CardProblem{"card.missing-field", "the card needs at least one non-reference rotor",
                           "nonFollowingRotorIds"};
    for (const RotorInSuperelement& rotor : card.rotors)
    {
        if (const auto problem = checkRequiredId(rotor.rotorId, "nonFollowingRotorIds"))
            return problem;
        if (rotor.superelement < 0)
            return CardProblem{"card.bad-id",
                               std::string(CardProblem::FIELD) + " must not be negative (0 is the residual structure)",
                               "nonFollowingSuperelementIds"};
    }
    return std::nullopt;
}

void SuperelementReferenceRotorFormat::write(const SuperelementReferenceRotorCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.referenceRotorId);
    writer.integerUnlessDefault(3, card.referenceSuperelementId, 0);
    int field = FIRST_PAIR_FIELD;
    for (const RotorInSuperelement& rotor : card.rotors)
    {
        writer.integer(field, rotor.rotorId);
        writer.integerUnlessDefault(field + 1, rotor.superelement, 0);
        field += 2;
    }
}

std::uint32_t SuperelementReferenceRotorTable::append(const SuperelementReferenceRotorCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_referenceRotorId.push(card.referenceRotorId);
    m_referenceSuperelementId.push(card.referenceSuperelementId);
    m_cards.push(card);
    return row;
}

void SuperelementReferenceRotorTable::store(std::uint32_t row, const SuperelementReferenceRotorCard& card)
{
    m_id.set(row, card.id);
    m_referenceRotorId.set(row, card.referenceRotorId);
    m_referenceSuperelementId.set(row, card.referenceSuperelementId);
    m_cards.set(row, card);
}

void SuperelementReferenceRotorTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_referenceRotorId.keepRows(rows);
    m_referenceSuperelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementReferenceRotorTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void SuperelementReferenceRotorTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementReferenceRotorCard& card = m_cards.edit(row);
    visitRotor(visitor, "referenceRotorId", card.referenceRotorId, card.referenceSuperelementId);
    m_referenceRotorId.set(row, card.referenceRotorId);
    for (RotorInSuperelement& rotor : card.rotors)
        visitRotor(visitor, "nonFollowingRotorIds", rotor.rotorId, rotor.superelement);
}

} // namespace oor::model
