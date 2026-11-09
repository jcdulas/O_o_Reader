#include "oor/model/cards/aero/aeroController/AeroControllerTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 3;

} // namespace

std::optional<AeroControllerCard> AeroControllerFormat::read(FieldReader& reader)
{
    AeroControllerCard card;
    card.id = reader.requiredInteger(1, "id");
    card.label = std::string(reader.text(2));
    card.units = std::string(reader.text(3));
    if (reader.lastUsedField() > LAST_FIELD)
        reader.report(CardProblem{
            "card.bad-value",
            "an " + std::string(reader.cardName()) + " has three fields: {field:id}, {field:label} and {field:units}",
            ""});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroControllerFormat::check(const AeroControllerCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.label, "label"))
        return problem;
    // Blank units are allowed (no units are attached to the controller); a text must read back, like a label.
    if (!card.units.empty())
        return checkNameWord(card.units, "units");
    return std::nullopt;
}

void AeroControllerFormat::write(const AeroControllerCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.label);
    if (!card.units.empty())
        writer.text(3, card.units);
}

std::uint32_t AeroControllerTable::append(const AeroControllerCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_label.push(card.label);
    m_units.push(card.units);
    return row;
}

AeroControllerCard AeroControllerTable::card(std::uint32_t row) const
{
    AeroControllerCard card;
    card.id = m_id.get(row);
    card.label = m_label.get(row);
    card.units = m_units.get(row);
    return card;
}

void AeroControllerTable::store(std::uint32_t row, const AeroControllerCard& card)
{
    m_id.set(row, card.id);
    m_label.set(row, card.label);
    m_units.set(row, card.units);
}

void AeroControllerTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_label.keepRows(rows);
    m_units.keepRows(rows);
}

void AeroControllerTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // The ID is ignored by the solver and the label is a name: the card refers to no other card.
}

} // namespace oor::model
