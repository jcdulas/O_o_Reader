#include "oor/model/cards/nodes/moduleLabel/ModuleLabelTable.hpp"

#include "oor/model/cards/nodes/ModuleBoundarySupport.hpp"

namespace oor::model
{

namespace
{

// The label runs over the seven fields after the MID.
constexpr int LABEL_FIRST_FIELD = 2;
constexpr int LABEL_LAST_FIELD = 8;

} // namespace

std::optional<ModuleLabelCard> ModuleLabelFormat::read(FieldReader& reader)
{
    ModuleLabelCard card;
    card.id = reader.integer(1, "id", 0);
    card.entryLabel = reader.spanText(LABEL_FIRST_FIELD, LABEL_LAST_FIELD);
    if (reader.lastUsedField() > LABEL_LAST_FIELD)
        reader.error("card.bad-value",
                     "the label of an " + std::string(reader.cardName()) + " has at most 56 characters", "entryLabel");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleLabelFormat::check(const ModuleLabelCard& card)
{
    if (const auto problem = checkBoundaryModule(card.id, "id"))
        return problem;
    return boundaryLabelProblem(card.entryLabel, "entryLabel");
}

void ModuleLabelFormat::write(const ModuleLabelCard& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.id, 0);
    writer.spanText(LABEL_FIRST_FIELD, LABEL_LAST_FIELD - LABEL_FIRST_FIELD + 1, card.entryLabel);
}

std::uint32_t ModuleLabelTable::append(const ModuleLabelCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_entryLabel.push(card.entryLabel);
    return row;
}

void ModuleLabelTable::store(std::uint32_t row, const ModuleLabelCard& card)
{
    m_id.set(row, card.id);
    m_entryLabel.set(row, card.entryLabel);
}

void ModuleLabelTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_entryLabel.keepRows(rows);
}

} // namespace oor::model
