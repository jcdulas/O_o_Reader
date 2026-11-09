#include "oor/model/cards/dynamics/rotorHybridDamping/RotorHybridDampingTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<RotorHybridDampingCard> RotorHybridDampingFormat::read(FieldReader& reader)
{
    RotorHybridDampingCard card;
    // The pairs follow each other, rotor first, four to a line; a pair whose two fields are blank is padding.
    const int last = reader.lastUsedField();
    for (int field = 1; field <= last; field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        RotorHybridPair pair;
        pair.rotorId = reader.requiredInteger(field, "rotorId");
        pair.hybridDampingId = reader.requiredInteger(field + 1, "hybridDampingId");
        card.pairs.push_back(pair);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RotorHybridDampingFormat::check(const RotorHybridDampingCard& card)
{
    if (card.pairs.empty())
        return CardProblem{"card.missing-field", "the card needs at least one pair of a rotor and a hybrid damping",
                           "rotorId1"};
    for (const RotorHybridPair& pair : card.pairs)
    {
        if (const auto problem = checkRequiredId(pair.rotorId, "rotorId"))
            return problem;
        if (const auto problem = checkRequiredId(pair.hybridDampingId, "hybridDampingId"))
            return problem;
    }
    return std::nullopt;
}

void RotorHybridDampingFormat::write(const RotorHybridDampingCard& card, FieldWriter& writer)
{
    int field = 1;
    for (const RotorHybridPair& pair : card.pairs)
    {
        writer.integer(field++, pair.rotorId);
        writer.integer(field++, pair.hybridDampingId);
    }
}

std::uint32_t RotorHybridDampingTable::append(const RotorHybridDampingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_pairCount.push(static_cast<std::int32_t>(card.pairs.size()));
    m_cards.push(card);
    return row;
}

void RotorHybridDampingTable::store(std::uint32_t row, const RotorHybridDampingCard& card)
{
    m_pairCount.set(row, static_cast<std::int32_t>(card.pairs.size()));
    m_cards.set(row, card);
}

void RotorHybridDampingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_pairCount.keepRows(rows);
    m_cards.keepRows(rows);
}

void RotorHybridDampingTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (RotorHybridPair& pair : m_cards.edit(row).pairs)
    {
        visitId(visitor, "rotorId", ReferenceTarget::Rotor, pair.rotorId);
        visitId(visitor, "hybridDampingId", ReferenceTarget::HybridDamping, pair.hybridDampingId);
    }
}

} // namespace oor::model
