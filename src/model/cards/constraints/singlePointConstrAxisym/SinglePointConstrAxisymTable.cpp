#include "oor/model/cards/constraints/singlePointConstrAxisym/SinglePointConstrAxisymTable.hpp"

#include "oor/base/text/DigitSets.hpp"

namespace oor::model
{

std::optional<SinglePointConstrAxisymCard> SinglePointConstrAxisymFormat::read(FieldReader& reader)
{
    SinglePointConstrAxisymCard card;
    card.id = reader.requiredInteger(1, "id");
    card.ringId = reader.requiredInteger(2, "ringId");
    card.harmonic = reader.requiredInteger(3, "harmonicId");
    card.components = base::sortedDigits(reader.requiredInteger(4, "components"));
    card.enforced = reader.requiredReal(5, "enforcedValue");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SinglePointConstrAxisymFormat::check(const SinglePointConstrAxisymCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the constraint set ID must be a positive integer", "id"};
    if (card.ringId <= 0)
        return CardProblem{"card.bad-id", "the ring ID must be a positive integer", "ringId"};
    if (card.harmonic < 0)
        return CardProblem{"card.bad-value", "the harmonic ID must not be negative", "harmonicId"};
    if (card.components <= 0 || !base::isComponentSet(card.components))
        return CardProblem{"card.bad-value", "C must be a list of the digits 1 to 6, each at most once", "components"};
    return std::nullopt;
}

void SinglePointConstrAxisymFormat::write(const SinglePointConstrAxisymCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.ringId);
    writer.integer(3, card.harmonic);
    writer.integer(4, card.components);
    writer.real(5, card.enforced);
}

std::uint32_t SinglePointConstrAxisymTable::append(const SinglePointConstrAxisymCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_ringId.push(card.ringId);
    m_harmonic.push(card.harmonic);
    m_components.push(card.components);
    m_enforced.push(card.enforced);
    return row;
}

SinglePointConstrAxisymCard SinglePointConstrAxisymTable::card(std::uint32_t row) const
{
    return SinglePointConstrAxisymCard{m_id.get(row), m_ringId.get(row), m_harmonic.get(row), m_components.get(row),
                                       m_enforced.get(row)};
}

void SinglePointConstrAxisymTable::store(std::uint32_t row, const SinglePointConstrAxisymCard& card)
{
    m_id.set(row, card.id);
    m_ringId.set(row, card.ringId);
    m_harmonic.set(row, card.harmonic);
    m_components.set(row, card.components);
    m_enforced.set(row, card.enforced);
}

void SinglePointConstrAxisymTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_ringId.keepRows(rows);
    m_harmonic.keepRows(rows);
    m_components.keepRows(rows);
    m_enforced.keepRows(rows);
}

void SinglePointConstrAxisymTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A RINGAX point, which is in the node ID space.
    m_ringId.set(row, visitor.visit("ringId", ReferenceTarget::Node, m_ringId.get(row)));
}

} // namespace oor::model
