#include "oor/model/cards/explicit/userServiceData/UserServiceDataTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <array>
#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int LINE = 8;            // fields to a line
constexpr int VALUES_PER_LINE = 7; // the first field of a line holds the keyword, or nothing
constexpr int FIRST_BLOCK_FIELD = 9;

enum class Block : std::uint8_t
{
    Integers,
    Reals,
    Words,
    None
};

// The block that a keyword starts, or Block::None when the text is no keyword of the card.
Block blockOf(std::string_view text)
{
    if ((text == "IntegerBlock"))
        return Block::Integers;
    if ((text == "RealBlock"))
        return Block::Reals;
    if ((text == "TextBlock"))
        return Block::Words;
    return Block::None;
}

const char* keywordOf(Block block)
{
    switch (block)
    {
    case Block::Integers:
        return "IntegerBlock";
    case Block::Reals:
        return "RealBlock";
    case Block::Words:
        return "TextBlock";
    case Block::None:
        break;
    }
    return "";
}

// True when a field of the line that starts at `first`, after the field of the keyword, has a value.
bool lineHasValues(const FieldReader& reader, int first)
{
    for (int field = first + 1; field < first + LINE; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

// The number of lines that a block of `count` values takes, its keyword line included.
int linesOf(std::size_t count)
{
    return static_cast<int>((count + VALUES_PER_LINE - 1) / VALUES_PER_LINE);
}

// The field of value number `index` (from 0) of a block whose keyword is in field `first`.
int valueField(int first, std::size_t index)
{
    return first + LINE * static_cast<int>(index / VALUES_PER_LINE) + 1 + static_cast<int>(index % VALUES_PER_LINE);
}

} // namespace

std::optional<UserServiceDataCard> UserServiceDataFormat::read(FieldReader& reader)
{
    UserServiceDataCard card;
    card.serviceId = std::string(reader.text(1));
    const int last = reader.lastUsedField();
    requireBlankFields(reader, {2, 3, 4, 5, 6, 7, 8}, last);
    std::array<bool, 3> seen{false, false, false};
    Block current = Block::None;
    for (int line = FIRST_BLOCK_FIELD; line <= last && !reader.failed(); line += LINE)
    {
        const std::string_view keyword = reader.text(line);
        if (!keyword.empty())
        {
            current = blockOf(keyword);
            if (current == Block::None)
            {
                reader.error("card.bad-value",
                             "\"" + std::string(keyword) + "\" does not start a block of a " +
                                 std::string(reader.cardName()) + ": INT, REAL or CHAR",
                             "");
                break;
            }
            if (seen[static_cast<std::size_t>(current)])
            {
                reader.error("card.bad-value",
                             "the " + std::string(keywordOf(current)) + " block of a " +
                                 std::string(reader.cardName()) + " is there twice",
                             "");
                break;
            }
            seen[static_cast<std::size_t>(current)] = true;
        }
        else if (!lineHasValues(reader, line))
        {
            continue;
        }
        else if (current == Block::None)
        {
            reader.error(
                "card.bad-value",
                "a line of a " + std::string(reader.cardName()) + " after the first one starts with INT, REAL or CHAR",
                "");
            break;
        }
        bool gap = false;
        for (int field = line + 1; field < line + LINE; ++field)
        {
            if (reader.isBlank(field))
            {
                gap = true;
                continue;
            }
            if (gap)
            {
                reader.error("card.bad-value",
                             "field " + std::to_string(field - 1) + " of a " + std::string(reader.cardName()) +
                                 " is blank between data",
                             "");
                return card;
            }
            switch (current)
            {
            case Block::Integers:
                card.integers.push_back(reader.requiredInteger(field, "integerData"));
                break;
            case Block::Reals:
                card.reals.push_back(reader.requiredReal(field, "realData"));
                break;
            case Block::Words:
                card.words.push_back(std::string(reader.text(field)));
                break;
            case Block::None:
                break;
            }
        }
    }
    if (!reader.failed())
    {
        if (seen[static_cast<std::size_t>(Block::Integers)] && card.integers.empty())
            reader.error("card.missing-field", "the INT block of a " + std::string(reader.cardName()) + " has no data",
                         "integerData");
        if (seen[static_cast<std::size_t>(Block::Reals)] && card.reals.empty())
            reader.error("card.missing-field", "the REAL block of a " + std::string(reader.cardName()) + " has no data",
                         "realData");
        if (seen[static_cast<std::size_t>(Block::Words)] && card.words.empty())
            reader.error("card.missing-field", "the CHAR block of a " + std::string(reader.cardName()) + " has no data",
                         "textData");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserServiceDataFormat::check(const UserServiceDataCard& card)
{
    if (const auto problem = checkNameWord(card.serviceId, "serviceId"))
        return problem;
    for (const double value : card.reals)
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                               "realData"};
    }
    for (const std::string& word : card.words)
    {
        if (const auto problem = checkNameWord(word, "textData"))
            return problem;
        if (blockOf(word) != Block::None)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " cannot be the word INT, REAL or CHAR", "textData"};
    }
    return std::nullopt;
}

void UserServiceDataFormat::write(const UserServiceDataCard& card, FieldWriter& writer)
{
    writer.text(1, card.serviceId);
    int line = FIRST_BLOCK_FIELD;
    if (!card.integers.empty())
    {
        writer.text(line, keywordOf(Block::Integers));
        for (std::size_t index = 0; index < card.integers.size(); ++index)
            writer.integer(valueField(line, index), card.integers[index]);
        line += LINE * linesOf(card.integers.size());
    }
    if (!card.reals.empty())
    {
        writer.text(line, keywordOf(Block::Reals));
        for (std::size_t index = 0; index < card.reals.size(); ++index)
            writer.real(valueField(line, index), card.reals[index]);
        line += LINE * linesOf(card.reals.size());
    }
    if (!card.words.empty())
    {
        writer.text(line, keywordOf(Block::Words));
        for (std::size_t index = 0; index < card.words.size(); ++index)
            writer.text(valueField(line, index), card.words[index]);
    }
}

std::uint32_t UserServiceDataTable::append(const UserServiceDataCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_serviceId.push(card.serviceId);
    m_cards.push(card);
    return row;
}

void UserServiceDataTable::store(std::uint32_t row, const UserServiceDataCard& card)
{
    m_serviceId.set(row, card.serviceId);
    m_cards.set(row, card);
}

void UserServiceDataTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_serviceId.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
