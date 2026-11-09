#include "oor/model/cards/solution_control/tireSelection/TireSelectionTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int KEYWORD_FIELD = 9;
constexpr int FIRST_NAME_FIELD = 10;
constexpr std::string_view EXTERNAL_TIRE_MODEL = "ExternalTireModel";

std::string externalTireModelWord()
{
    return std::string(EXTERNAL_TIRE_MODEL);
}

} // namespace

std::optional<TireSelectionCard> TireSelectionFormat::read(FieldReader& reader)
{
    TireSelectionCard card;
    card.id = reader.requiredInteger(1, "id");
    requireUnusedBlank(reader, {2, 3, 4, 5, 6, 7, 8}, reader.lastUsedField());
    const std::string word = externalTireModelWord();
    if (!(reader.text(KEYWORD_FIELD) == word))
        reader.error("card.missing-field",
                     "field " + std::to_string(KEYWORD_FIELD) + " of a " + std::string(reader.cardName()) +
                         " must be the word " + word,
                     "tireFileKeyword");
    const int last = reader.lastUsedField();
    for (int field = FIRST_NAME_FIELD; field <= last; ++field)
    {
        if (!reader.isBlank(field))
            card.names.emplace_back(reader.text(field));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TireSelectionFormat::check(const TireSelectionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.names.empty())
        return CardProblem{"card.missing-field", "the card needs the name of at least one tire", "tireNames"};
    for (const std::string& name : card.names)
    {
        if (const auto problem = checkNameWord(name, "tireNames"))
            return problem;
    }
    return std::nullopt;
}

void TireSelectionFormat::write(const TireSelectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(KEYWORD_FIELD, externalTireModelWord());
    int field = FIRST_NAME_FIELD;
    for (const std::string& name : card.names)
        writer.text(field++, name);
}

std::uint32_t TireSelectionTable::append(const TireSelectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TireSelectionTable::store(std::uint32_t row, const TireSelectionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TireSelectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TireSelectionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
