#include "oor/model/cards/aero/aeroRigidTrimVariable/AeroRigidTrimVariableTable.hpp"

#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 2;

} // namespace

std::optional<AeroRigidTrimVariableCard> AeroRigidTrimVariableFormat::read(FieldReader& reader)
{
    AeroRigidTrimVariableCard card;
    card.id = reader.requiredInteger(1, "id");
    card.label = std::string(reader.text(2));
    if (reader.lastUsedField() > LAST_FIELD)
        reader.report(
            CardProblem{"card.bad-value",
                        "an " + std::string(reader.cardName()) + " has two fields: {field:id} and {field:label}", ""});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroRigidTrimVariableFormat::check(const AeroRigidTrimVariableCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkTrimLabel(card.label, "label");
}

void AeroRigidTrimVariableFormat::write(const AeroRigidTrimVariableCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.label);
}

std::uint32_t AeroRigidTrimVariableTable::append(const AeroRigidTrimVariableCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_label.push(card.label);
    return row;
}

AeroRigidTrimVariableCard AeroRigidTrimVariableTable::card(std::uint32_t row) const
{
    AeroRigidTrimVariableCard card;
    card.id = m_id.get(row);
    card.label = m_label.get(row);
    return card;
}

void AeroRigidTrimVariableTable::store(std::uint32_t row, const AeroRigidTrimVariableCard& card)
{
    m_id.set(row, card.id);
    m_label.set(row, card.label);
}

void AeroRigidTrimVariableTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_label.keepRows(rows);
}

void AeroRigidTrimVariableTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // The ID is ignored by the solver and the label is a name: the card refers to no other card.
}

} // namespace oor::model
