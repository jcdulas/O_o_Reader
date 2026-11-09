#include "oor/model/cards/dynamics/frfExcitationDof/FrfExcitationDofTable.hpp"

#include "oor/model/cards/dynamics/FrfComponentReference.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_LABEL_FIELD = 3;
constexpr int LAST_LABEL_FIELD = 8;
constexpr int LABEL_FIELD_COUNT = LAST_LABEL_FIELD - FIRST_LABEL_FIELD + 1;

std::optional<CardProblem> labelProblem(const std::string& label)
{
    return checkNameWord(label, "entryLabel");
}

} // namespace

std::optional<FrfExcitationDofCard> FrfExcitationDofFormat::read(FieldReader& reader)
{
    FrfExcitationDofCard card;
    card.pointId = reader.requiredInteger(1, "pointId");
    card.components = reader.integer(2, "components", 0);
    // the label starts in its first field.
    if (reader.isBlank(FIRST_LABEL_FIELD) && reader.lastUsedField() > FIRST_LABEL_FIELD)
        reader.error(
            "card.bad-value",
            "the " + std::string("entryLabel") + " must begin in its first field (field 4 of the format reference)",
            "entryLabel");
    card.entryLabel = reader.spanText(FIRST_LABEL_FIELD, LAST_LABEL_FIELD);
    if (reader.lastUsedField() > LAST_LABEL_FIELD)
        reader.error("card.bad-value",
                     "the card has no field beyond its " + std::string("entryLabel") +
                         " (fields 4 to 9 of the format reference)",
                     "entryLabel");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FrfExcitationDofFormat::check(const FrfExcitationDofCard& card)
{
    if (const auto problem = checkRequiredId(card.pointId, "pointId"))
        return problem;
    if (const auto problem = checkSingleComponent(card.components, "components"))
        return problem;
    return labelProblem(card.entryLabel);
}

void FrfExcitationDofFormat::write(const FrfExcitationDofCard& card, FieldWriter& writer)
{
    writer.integer(1, card.pointId);
    writer.integerUnlessDefault(2, card.components, 0);
    writer.spanText(FIRST_LABEL_FIELD, LABEL_FIELD_COUNT, card.entryLabel);
}

std::uint32_t FrfExcitationDofTable::append(const FrfExcitationDofCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_pointId.push(card.pointId);
    m_components.push(card.components);
    m_entryLabel.push(card.entryLabel);
    return row;
}

FrfExcitationDofCard FrfExcitationDofTable::card(std::uint32_t row) const
{
    FrfExcitationDofCard card;
    card.pointId = m_pointId.get(row);
    card.components = m_components.get(row);
    card.entryLabel = m_entryLabel.get(row);
    return card;
}

void FrfExcitationDofTable::store(std::uint32_t row, const FrfExcitationDofCard& card)
{
    m_pointId.set(row, card.pointId);
    m_components.set(row, card.components);
    m_entryLabel.set(row, card.entryLabel);
}

void FrfExcitationDofTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_pointId.keepRows(rows);
    m_components.keepRows(rows);
    m_entryLabel.keepRows(rows);
}

void FrfExcitationDofTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId point = m_pointId.get(row); point > 0)
        m_pointId.set(row, visitor.visit("pointId", ReferenceTarget::ScalarOrGridPoint, point));
}

} // namespace oor::model
