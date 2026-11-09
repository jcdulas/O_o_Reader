#include "oor/model/cards/superelements/superelementLabel/SuperelementLabelTable.hpp"

#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

// The label runs over the seven fields after the SEID.
constexpr int LABEL_FIRST_FIELD = 2;
constexpr int LABEL_LAST_FIELD = 8;

} // namespace

std::optional<SuperelementLabelCard> SuperelementLabelFormat::read(FieldReader& reader)
{
    SuperelementLabelCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.entryLabel = reader.spanText(LABEL_FIRST_FIELD, LABEL_LAST_FIELD);
    if (reader.lastUsedField() > LABEL_LAST_FIELD)
        reader.error("card.bad-value",
                     "the label of an " + std::string(reader.cardName()) + " has at most 56 characters", "entryLabel");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementLabelFormat::check(const SuperelementLabelCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", false))
        return problem;
    return superelementLabelProblem(card.entryLabel, "entryLabel");
}

void SuperelementLabelFormat::write(const SuperelementLabelCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writer.spanText(LABEL_FIRST_FIELD, LABEL_LAST_FIELD - LABEL_FIRST_FIELD + 1, card.entryLabel);
}

std::uint32_t SuperelementLabelTable::append(const SuperelementLabelCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_entryLabel.push(card.entryLabel);
    return row;
}

SuperelementLabelCard SuperelementLabelTable::card(std::uint32_t row) const
{
    return SuperelementLabelCard{m_superelementId.get(row), m_entryLabel.get(row)};
}

void SuperelementLabelTable::store(std::uint32_t row, const SuperelementLabelCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_entryLabel.set(row, card.entryLabel);
}

void SuperelementLabelTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_entryLabel.keepRows(rows);
}

void SuperelementLabelTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    base::EntityId superelementId = m_superelementId.get(row);
    visitSuperelement(visitor, "superelementId", superelementId);
    m_superelementId.set(row, superelementId);
}

} // namespace oor::model
