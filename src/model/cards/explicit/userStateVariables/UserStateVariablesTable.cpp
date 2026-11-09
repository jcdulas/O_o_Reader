#include "oor/model/cards/explicit/userStateVariables/UserStateVariablesTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <charconv>
#include <set>
#include <string>
#include <system_error>

namespace oor::model
{

namespace
{

constexpr int VARIABLE_COUNT_FIELD = 2;
constexpr int FIRST_PAIR_FIELD = 9; // SV2; each pair takes two fields

// The number i of a state variable from its default name SVi (in capitals), or nothing when the text is not that.
std::optional<std::int32_t> stateVariableNumber(const std::string& label)
{
    constexpr std::size_t PREFIX_LENGTH = 2;
    if (label.size() <= PREFIX_LENGTH || label.compare(0, PREFIX_LENGTH, "SV") != 0)
        return std::nullopt;
    const char* first = label.data() + PREFIX_LENGTH;
    const char* end = label.data() + label.size();
    if (*first < '0' || *first > '9')
        return std::nullopt;
    std::int32_t number = 0;
    const std::from_chars_result result = std::from_chars(first, end, number);
    if (result.ec != std::errc{} || result.ptr != end)
        return std::nullopt;
    return number;
}

std::string defaultName(std::int32_t index)
{
    return "SV" + std::to_string(index);
}

} // namespace

std::optional<UserStateVariablesCard> UserStateVariablesFormat::read(FieldReader& reader)
{
    UserStateVariablesCard card;
    const int last = reader.lastUsedField();
    requireBlankFields(reader, {1, 3, 4, 5, 6, 7, 8}, last);
    card.variableCount = reader.requiredInteger(VARIABLE_COUNT_FIELD, "variableCount");
    for (int field = FIRST_PAIR_FIELD; field <= last && !reader.failed(); field += 2)
    {
        const std::string label(reader.text(field));
        const std::string name = std::string(reader.text(field + 1));
        if (label.empty() && name.empty())
            continue;
        if (label.empty())
        {
            reader.error("card.missing-field",
                         "the name of a state variable is there without its " + std::string("variableLabel"),
                         "variableLabel");
            break;
        }
        const std::optional<std::int32_t> number = stateVariableNumber(label);
        if (!number)
        {
            reader.error("card.bad-value", "\"" + label + "\" is not SV followed by the number of a state variable",
                         "variableLabel");
            break;
        }
        card.names.push_back(UserStateVariablesName{*number, name.empty() ? defaultName(*number) : name});
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserStateVariablesFormat::check(const UserStateVariablesCard& card)
{
    if (card.variableCount < 1 || card.variableCount > UserStateVariablesCard::MAX_STATE_VARIABLES)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 1 and 100",
                           "variableCount"};
    std::set<std::int32_t> numbers;
    for (const UserStateVariablesName& entry : card.names)
    {
        if (entry.index < 1 || entry.index > card.variableCount)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be the number of one of the " +
                                   CardProblem::fieldNamed("variableCount") + " state variables",
                               "variableLabel"};
        if (!numbers.insert(entry.index).second)
            return CardProblem{"card.bad-value", "a state variable is named twice", "variableLabel"};
        if (const auto problem = checkNameWord(entry.name, "variableName"))
            return problem;
    }
    return std::nullopt;
}

void UserStateVariablesFormat::write(const UserStateVariablesCard& card, FieldWriter& writer)
{
    writer.integer(VARIABLE_COUNT_FIELD, card.variableCount);
    int field = FIRST_PAIR_FIELD;
    for (const UserStateVariablesName& entry : card.names)
    {
        writer.text(field, defaultName(entry.index));
        writer.text(field + 1, entry.name);
        field += 2;
    }
}

std::uint32_t UserStateVariablesTable::append(const UserStateVariablesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_variableCount.push(card.variableCount);
    m_cards.push(card);
    return row;
}

void UserStateVariablesTable::store(std::uint32_t row, const UserStateVariablesCard& card)
{
    m_variableCount.set(row, card.variableCount);
    m_cards.set(row, card);
}

void UserStateVariablesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_variableCount.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
