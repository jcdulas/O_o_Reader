#include "oor/model/cards/nonlinear_contact/contactUserService/ContactUserServiceTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr std::string_view INT = "IntegerData";
constexpr std::string_view REAL = "RealData";
constexpr std::string_view CHAR = "TextData";

// The name of the field of the list that starts with `keyword` (INT, REAL or CHAR).
const char* listFieldName(std::string_view keyword) noexcept
{
    if (keyword == INT)
        return "integerData";
    return keyword == REAL ? "realData" : "textData";
}

// The name of a keyword would be taken for the start of the next list.
bool isListKeyword(std::string_view word)
{
    return (word == INT) || (word == REAL) || (word == CHAR);
}

} // namespace

std::optional<ContactUserServiceCard> ContactUserServiceFormat::read(FieldReader& reader)
{
    ContactUserServiceCard card;
    card.bodyId = reader.requiredInteger(1, "bodyId");
    // The kind is a body card, by its neutral card name.
    card.bodyKind = readKeyword(reader, 2, "bodyKind", {"ContactBody", "ContactBody1"}, "");
    if (reader.isBlank(2))
        reader.error("card.missing-field",
                     "a " + std::string(reader.cardName()) + " needs " + std::string("bodyKind") + ", " +
                         std::string("ContactBody") + " or " + std::string("ContactBody1"),
                     "bodyKind");
    card.groupName = std::string(reader.word(3));
    card.userRoutineName = std::string(reader.word(4));
    const std::vector<ContactSection> sections = splitContactSections(reader, {INT, REAL, CHAR});
    std::vector<std::string> seen;
    for (const ContactSection& section : sections)
    {
        if (section.keyword.empty())
        {
            if (section.last > CONTACT_LINE_FIELDS)
                reader.error(
                    "card.bad-value",
                    "a continuation line of a " + std::string(reader.cardName()) + " starts with INT, REAL or CHAR",
                    "integerData");
            continue;
        }
        // Each list is given once: a second one would replace or add to the first, and the format reference has no such
        // thing.
        if (std::find(seen.begin(), seen.end(), section.keyword) != seen.end())
            reader.error("card.bad-value", section.keyword + " is given twice", listFieldName(section.keyword));
        seen.push_back(section.keyword);
        if (section.keyword == INT)
        {
            for (const base::EntityId value : readContactIds(reader, section.first + 1, section.last, "integerData"))
                card.integers.push_back(value);
        }
        else if (section.keyword == REAL)
            card.reals = readContactReals(reader, section.first + 1, section.last, "realData");
        else
            card.characters = readContactWords(reader, section.first + 1, section.last);
    }
    requireUnusedBlank(reader, {5, 6, 7, 8}, reader.lastUsedField());
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactUserServiceFormat::check(const ContactUserServiceCard& card)
{
    if (card.bodyId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "bodyId"};
    if (card.bodyKind != "ContactBody" && card.bodyKind != "ContactBody1")
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be " + CardProblem::cardNamed("ContactBody") +
                               " or " + CardProblem::cardNamed("ContactBody1"),
                           "bodyKind"};
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    if (const auto problem = checkNameWord(card.userRoutineName, "userRoutineName"))
        return problem;
    for (const std::string& word : card.characters)
    {
        if (const auto problem = checkNameWord(word, "textData"))
            return problem;
        if (isListKeyword(word))
            return CardProblem{"card.bad-value", "textData must not be a section key", "textData"};
    }
    return std::nullopt;
}

void ContactUserServiceFormat::write(const ContactUserServiceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.bodyId);
    writer.text(2, card.bodyKind);
    writer.text(3, card.groupName);
    writer.text(4, card.userRoutineName);
    int line = firstFieldOfLine(1);
    if (!card.integers.empty())
    {
        const std::vector<base::EntityId> ids(card.integers.begin(), card.integers.end());
        line = writeContactIdList(writer, line, INT, ids);
    }
    if (!card.reals.empty())
        line = writeContactRealList(writer, line, REAL, card.reals);
    if (!card.characters.empty())
        writeContactWordList(writer, line, CHAR, card.characters);
}

std::uint32_t ContactUserServiceTable::append(const ContactUserServiceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_bodyId.push(card.bodyId);
    m_cards.push(card);
    return row;
}

void ContactUserServiceTable::store(std::uint32_t row, const ContactUserServiceCard& card)
{
    m_bodyId.set(row, card.bodyId);
    m_cards.set(row, card);
}

void ContactUserServiceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_bodyId.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactUserServiceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactId(m_cards.edit(row).bodyId, visitor, "bodyId", ReferenceTarget::ContactBody);
    m_bodyId.set(row, m_cards.get(row).bodyId);
}

} // namespace oor::model
