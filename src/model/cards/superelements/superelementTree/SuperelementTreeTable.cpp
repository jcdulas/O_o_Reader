#include "oor/model/cards/superelements/superelementTree/SuperelementTreeTable.hpp"

#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_UPSTREAM_FIELD = 2;

} // namespace

std::optional<SuperelementTreeCard> SuperelementTreeFormat::read(FieldReader& reader)
{
    SuperelementTreeCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    const int last = reader.lastUsedField();
    for (int field = FIRST_UPSTREAM_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field))
            card.upstream.push_back(reader.requiredInteger(field, "upstreamSuperelementId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementTreeFormat::check(const SuperelementTreeCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", true))
        return problem;
    if (card.upstream.empty())
        return CardProblem{"card.missing-field",
                           "the card needs the upstream superelement " + std::string(CardProblem::FIELD),
                           "upstreamSuperelementId1"};
    for (const base::EntityId upstream : card.upstream)
    {
        if (const auto problem = checkSuperelementId(upstream, "upstreamSuperelementId", false))
            return problem;
    }
    return std::nullopt;
}

void SuperelementTreeFormat::write(const SuperelementTreeCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    int field = FIRST_UPSTREAM_FIELD;
    for (const base::EntityId upstream : card.upstream)
        writer.integer(field++, upstream);
}

std::uint32_t SuperelementTreeTable::append(const SuperelementTreeCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_cards.push(card);
    return row;
}

void SuperelementTreeTable::store(std::uint32_t row, const SuperelementTreeCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_cards.set(row, card);
}

void SuperelementTreeTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementTreeTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementTreeCard& card = m_cards.edit(row);
    visitSuperelement(visitor, "superelementId", card.superelementId);
    m_superelementId.set(row, card.superelementId);
    for (base::EntityId& upstream : card.upstream)
        visitSuperelement(visitor, "upstreamSuperelementId", upstream);
}

} // namespace oor::model
