#include "oor/model/cards/nonlinear_contact/contactSet/ContactSetTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <vector>

namespace oor::model
{

namespace
{

// A pair is five fields, the first one at field 2 and then one line (eight fields) later each; the three fields after
// it stay blank.
constexpr int LINE_FIELDS = 8;
constexpr int FIRST_PAIR_FIELD = 2;
constexpr int PAIR_FIELDS = 5;

constexpr int pairField(std::size_t index) noexcept
{
    return FIRST_PAIR_FIELD + LINE_FIELDS * static_cast<int>(index);
}

} // namespace

std::optional<ContactSetCard> ContactSetFormat::read(FieldReader& reader)
{
    ContactSetCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    std::vector<int> used = {1};
    for (std::size_t index = 0; pairField(index) <= last; ++index)
    {
        const int first = pairField(index);
        ContactSetPair pair;
        pair.sourceRegionId = reader.requiredInteger(first, "sourceRegionId");
        pair.targetRegionId = reader.requiredInteger(first + 1, "targetRegionId");
        pair.friction = reader.real(first + 2, "friction", 0.0);
        pair.minimumDistance = reader.real(first + 3, "minimumDistance", 0.0);
        pair.maximumDistance = reader.real(first + 4, "maximumDistance", 0.0);
        card.pairs.push_back(pair);
        for (int field = first; field < first + PAIR_FIELDS; ++field)
            used.push_back(field);
    }
    // The three fields after a pair and the first field of a continuation line hold nothing.
    requireOnlyFields(reader, used);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactSetFormat::check(const ContactSetCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.pairs.empty())
        return CardProblem{"card.missing-field", "the set needs at least one contact pair", "sourceRegionId"};
    for (const ContactSetPair& pair : card.pairs)
    {
        if (const auto problem = checkRequiredId(pair.sourceRegionId, "sourceRegionId"))
            return problem;
        if (const auto problem = checkRequiredId(pair.targetRegionId, "targetRegionId"))
            return problem;
        if (const auto problem = checkReal(pair.friction, "friction", RealRule::NonNegative))
            return problem;
        if (const auto problem = checkReal(pair.minimumDistance, "minimumDistance", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(pair.maximumDistance, "maximumDistance", RealRule::Finite))
            return problem;
    }
    return std::nullopt;
}

void ContactSetFormat::write(const ContactSetCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.pairs.size(); ++index)
    {
        const int first = pairField(index);
        const ContactSetPair& pair = card.pairs[index];
        writer.integer(first, pair.sourceRegionId);
        writer.integer(first + 1, pair.targetRegionId);
        writer.realUnlessDefault(first + 2, pair.friction, 0.0);
        writer.realUnlessDefault(first + 3, pair.minimumDistance, 0.0);
        writer.realUnlessDefault(first + 4, pair.maximumDistance, 0.0);
    }
}

std::uint32_t ContactSetTable::append(const ContactSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactSetTable::store(std::uint32_t row, const ContactSetCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactSetTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
