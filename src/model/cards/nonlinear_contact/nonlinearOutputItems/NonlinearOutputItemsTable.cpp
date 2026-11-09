#include "oor/model/cards/nonlinear_contact/nonlinearOutputItems/NonlinearOutputItemsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

namespace oor::model
{

namespace
{

constexpr std::string_view STATE_VARIABLE_KEYWORD = "ElemStateVariables";

// A name of the card: letters, digits and the underscore, as the format reference names are, not the keyword of the
// state variables.
bool isName(std::string_view word)
{
    if (word.empty())
        return false;
    for (const char character : word)
    {
        const bool letter = (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
        const bool digit = character >= '0' && character <= '9';
        if (!letter && !digit && character != '_')
            return false;
    }
    return true;
}

} // namespace

std::optional<NonlinearOutputItemsCard> NonlinearOutputItemsFormat::read(FieldReader& reader)
{
    NonlinearOutputItemsCard card;
    card.id = reader.requiredInteger(1, "id");
    // The words follow from field 2; the fields between them may be blank. The ones after ESV are state variables.
    bool afterKeyword = false;
    for (const std::string& word : readContactWords(reader, 2, reader.lastUsedField()))
    {
        if (word == STATE_VARIABLE_KEYWORD)
        {
            if (afterKeyword)
                reader.error("card.bad-value", "ESV is given twice", "stateVariableList");
            afterKeyword = true;
        }
        else
            (afterKeyword ? card.stateVariables : card.outputs).push_back(word);
    }
    if (afterKeyword && card.stateVariables.empty())
        reader.error("card.missing-field", "ESV needs the name of at least one state variable", "stateVariables");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearOutputItemsFormat::check(const NonlinearOutputItemsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    for (const std::string& word : card.outputs)
    {
        if (!isName(word))
            return CardProblem{"card.bad-value", "\"" + word + "\" is not an output request word", "outputs"};
        if ((word == STATE_VARIABLE_KEYWORD))
            return CardProblem{"card.bad-value", "ESV is the keyword of the state variables, not an output request",
                               "outputs"};
    }
    for (const std::string& word : card.stateVariables)
    {
        if (!isName(word))
            return CardProblem{"card.bad-value", "\"" + word + "\" is not a state variable name", "stateVariables"};
        if ((word == STATE_VARIABLE_KEYWORD))
            return CardProblem{"card.bad-value", "ESV is the keyword of the state variables, not a name",
                               "stateVariables"};
    }
    return std::nullopt;
}

void NonlinearOutputItemsFormat::write(const NonlinearOutputItemsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = 2;
    for (const std::string& word : card.outputs)
        writer.text(field++, word);
    if (card.stateVariables.empty())
        return;
    // common decks start the state variables on a line of their own.
    field = firstFieldOfNextLine(field - 1);
    writer.text(field++, STATE_VARIABLE_KEYWORD);
    for (const std::string& word : card.stateVariables)
        writer.text(field++, word);
}

std::uint32_t NonlinearOutputItemsTable::append(const NonlinearOutputItemsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void NonlinearOutputItemsTable::store(std::uint32_t row, const NonlinearOutputItemsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void NonlinearOutputItemsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void NonlinearOutputItemsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
