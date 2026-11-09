#include "oor/model/cards/superelements/superelementGeneralizedPointCount/SuperelementGeneralizedPointCountTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<SuperelementGeneralizedPointCountCard> SuperelementGeneralizedPointCountFormat::read(FieldReader& reader)
{
    SuperelementGeneralizedPointCountCard card;
    if (reader.isBlank(1))
        reader.error("card.missing-field", std::string("superelementId") + " is required: a superelement or ALL",
                     "superelementId");
    else
        card.superelementId = readSuperelementOrAll(reader, 1, "superelementId", std::nullopt);
    card.count = reader.integer(2, "count", 0);
    if (reader.lastUsedField() > 2)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 2 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementGeneralizedPointCountFormat::check(
    const SuperelementGeneralizedPointCountCard& card)
{
    if (card.superelementId)
    {
        if (const auto problem = checkSuperelementId(*card.superelementId, "superelementId", false))
            return problem;
    }
    return checkOptionalId(card.count, "count");
}

void SuperelementGeneralizedPointCountFormat::write(const SuperelementGeneralizedPointCountCard& card,
                                                    FieldWriter& writer)
{
    writeSuperelementOrAll(writer, 1, card.superelementId, false);
    writer.integerUnlessDefault(2, card.count, 0);
}

std::uint32_t SuperelementGeneralizedPointCountTable::append(const SuperelementGeneralizedPointCountCard& card,
                                                             base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId.value_or(ALL_IN_COLUMN));
    m_count.push(card.count);
    return row;
}

SuperelementGeneralizedPointCountCard SuperelementGeneralizedPointCountTable::card(std::uint32_t row) const
{
    SuperelementGeneralizedPointCountCard card;
    if (const base::EntityId superelementId = m_superelementId.get(row); superelementId != ALL_IN_COLUMN)
        card.superelementId = superelementId;
    card.count = m_count.get(row);
    return card;
}

void SuperelementGeneralizedPointCountTable::store(std::uint32_t row, const SuperelementGeneralizedPointCountCard& card)
{
    m_superelementId.set(row, card.superelementId.value_or(ALL_IN_COLUMN));
    m_count.set(row, card.count);
}

void SuperelementGeneralizedPointCountTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_count.keepRows(rows);
}

void SuperelementGeneralizedPointCountTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const base::EntityId superelementId = m_superelementId.get(row);
    if (superelementId <= 0)
        return; // ALL
    m_superelementId.set(row, visitor.visit("superelementId", ReferenceTarget::Superelement, superelementId));
}

} // namespace oor::model
