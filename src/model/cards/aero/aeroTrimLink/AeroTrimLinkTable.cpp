#include "oor/model/cards/aero/aeroTrimLink/AeroTrimLinkTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr std::string_view ALWAYS_WORD = "Always";
constexpr int FIRST_TERM_FIELD = 3;

} // namespace

std::optional<AeroTrimLinkCard> AeroTrimLinkFormat::read(FieldReader& reader)
{
    AeroTrimLinkCard card;
    // The ID is an integer or the word ALWAYS (any case): both mean every subcase when they are 0.
    if ((reader.text(1) == ALWAYS_WORD))
        card.always = true;
    else
        card.id = reader.requiredInteger(1, "id");
    card.dependentLabel = std::string(reader.text(2));
    card.terms = readLabeledValues(reader, FIRST_TERM_FIELD, "independentLabels", "coefficients");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroTrimLinkFormat::check(const AeroTrimLinkCard& card)
{
    if (card.id < 0)
        return CardProblem{"card.bad-id", "{field} must be 0, ALWAYS or a positive integer", "id"};
    if (card.always && card.id != 0)
        return CardProblem{"card.bad-value", "{field} is written as ALWAYS, so it must be 0", "id"};
    if (const auto problem = checkTrimLabel(card.dependentLabel, "dependentLabel"))
        return problem;
    if (card.terms.empty())
        return CardProblem{"card.missing-field", "the card needs at least one independent variable ({field})",
                           "independentLabels"};
    return checkLabeledValues(card.terms, "independentLabels", "coefficients");
}

void AeroTrimLinkFormat::write(const AeroTrimLinkCard& card, FieldWriter& writer)
{
    if (card.always)
        writer.text(1, ALWAYS_WORD);
    else
        writer.integer(1, card.id);
    writer.text(2, card.dependentLabel);
    writeLabeledValues(writer, FIRST_TERM_FIELD, card.terms);
}

std::uint32_t AeroTrimLinkTable::append(const AeroTrimLinkCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroTrimLinkTable::store(std::uint32_t row, const AeroTrimLinkCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroTrimLinkTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroTrimLinkTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    AeroTrimLinkCard& card = m_cards.edit(row);
    card.id = id;
    card.always = card.always && id == 0;
}

} // namespace oor::model
