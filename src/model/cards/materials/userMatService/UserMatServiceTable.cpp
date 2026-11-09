#include "oor/model/cards/materials/userMatService/UserMatServiceTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/core/CardRegistry.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int LINE = 8;            // fields to a line
constexpr int VALUES_PER_LINE = 7; // the first field of a line holds the keyword (or nothing)
constexpr int FIRST_BLOCK_FIELD = 9;

enum class Block
{
    None,
    Integers,
    Reals,
    Words
};

Block parseKeyword(std::string_view text)
{
    if ((text == "IntegerBlock"))
        return Block::Integers;
    if ((text == "RealBlock"))
        return Block::Reals;
    if ((text == "TextBlock"))
        return Block::Words;
    return Block::None;
}

bool lineHasValues(const FieldReader& reader, int first)
{
    for (int field = first; field < first + LINE; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

bool isKeyword(const std::string& word)
{
    return parseKeyword(word) != Block::None;
}

// The number of lines that a block of `count` values takes (the keyword line included).
int linesOf(std::size_t count)
{
    return count == 0 ? 1 : static_cast<int>((count + VALUES_PER_LINE - 1) / VALUES_PER_LINE);
}

// The field of value `index` of a block that starts on the line at `first`.
int valueField(int first, std::size_t index)
{
    return first + LINE * static_cast<int>(index / VALUES_PER_LINE) + 1 + static_cast<int>(index % VALUES_PER_LINE);
}

} // namespace

std::optional<UserMatServiceCard> UserMatServiceFormat::read(FieldReader& reader)
{
    UserMatServiceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.materialCardType = std::string(reader.text(2));
    card.groupName = std::string(reader.text(3));
    card.userRoutineName = std::string(reader.text(4));
    for (int field = 5; field < FIRST_BLOCK_FIELD; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is not used and must be blank",
                         "");
            return card;
        }
    }

    bool seen[4] = {false, false, false, false};
    Block current = Block::None;
    const int last = reader.lastUsedField();
    for (int line = FIRST_BLOCK_FIELD; line <= last && !reader.failed(); line += LINE)
    {
        if (const std::string_view keyword = reader.text(line); !keyword.empty())
        {
            current = parseKeyword(keyword);
            if (current == Block::None)
            {
                reader.error("card.bad-value",
                             "\"" + std::string(keyword) + "\" does not start a block of a " +
                                 std::string(reader.cardName()) + ": INT, REAL or CHAR",
                             "");
                return card;
            }
            if (seen[static_cast<int>(current)])
            {
                reader.error(
                    "card.bad-value",
                    "a block of a " + std::string(reader.cardName()) + " is there twice: " + std::string(keyword), "");
                return card;
            }
            seen[static_cast<int>(current)] = true;
        }
        else if (!lineHasValues(reader, line))
            continue;
        else if (current == Block::None)
        {
            reader.error(
                "card.bad-value",
                "a line of a " + std::string(reader.cardName()) + " after the first starts with INT, REAL or CHAR", "");
            return card;
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
                card.words.emplace_back(reader.text(field));
                break;
            case Block::None:
                break;
            }
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserMatServiceFormat::check(const UserMatServiceCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkNameWord(card.materialCardType, "materialCardType"))
        return problem;
    if (card.groupName.empty())
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be the group name of the CONNECT SERVICE statement ", "groupName"};
    if (card.userRoutineName.empty())
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a subroutine name",
                           "userRoutineName"};
    for (const double value : card.reals)
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                               "realData"};
    }
    for (const std::string& word : card.words)
    {
        if (word.empty())
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be empty", "textData"};
        if (isKeyword(word))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " cannot be the word INT, REAL or CHAR", "textData"};
    }
    return std::nullopt;
}

void UserMatServiceFormat::write(const UserMatServiceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.materialCardType);
    writer.text(3, card.groupName);
    writer.text(4, card.userRoutineName);
    int line = FIRST_BLOCK_FIELD;
    if (!card.integers.empty())
    {
        writer.text(line, "IntegerBlock");
        for (std::size_t index = 0; index < card.integers.size(); ++index)
            writer.integer(valueField(line, index), card.integers[index]);
        line += LINE * linesOf(card.integers.size());
    }
    if (!card.reals.empty())
    {
        writer.text(line, "RealBlock");
        for (std::size_t index = 0; index < card.reals.size(); ++index)
            writer.real(valueField(line, index), card.reals[index]);
        line += LINE * linesOf(card.reals.size());
    }
    if (!card.words.empty())
    {
        writer.text(line, "TextBlock");
        for (std::size_t index = 0; index < card.words.size(); ++index)
            writer.text(valueField(line, index), card.words[index]);
    }
}

std::uint32_t UserMatServiceTable::append(const UserMatServiceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void UserMatServiceTable::store(std::uint32_t row, const UserMatServiceCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void UserMatServiceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void UserMatServiceTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void UserMatServiceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the material that the card serves.
    UserMatServiceCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
}

} // namespace oor::model
