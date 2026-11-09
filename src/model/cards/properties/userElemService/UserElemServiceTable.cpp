#include "oor/model/cards/properties/userElemService/UserElemServiceTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int LAST_HEAD_FIELD = 6;      // NMLSPD
constexpr int LAST_HEAD_LINE_FIELD = 8; // fields 7 and 8 are blank cells of the first line
constexpr int FIRST_SECTION_FIELD = 9;  // the sections begin on the second line

bool isSectionWord(std::string_view word)
{
    return (word == USER_ELEM_SERVICE_INTEGER_WORD) || (word == USER_ELEM_SERVICE_REAL_WORD) ||
           (word == USER_ELEM_SERVICE_CHARACTER_WORD);
}

// A section may be given once only: reports the second one, naming the data of the section.
bool takeSection(FieldReader& reader, bool& seen, std::string_view word, const char* dataName)
{
    if (seen)
    {
        reader.error(
            "card.bad-value",
            "the " + std::string(word) + " section of an " + std::string(reader.cardName()) + " is given twice",
            dataName);
        return false;
    }
    seen = true;
    return true;
}

} // namespace

std::optional<UserElemServiceCard> UserElemServiceFormat::read(FieldReader& reader)
{
    UserElemServiceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propType =
        readRequiredKeyword<UserElemServicePropertyType>(reader, 2, "propType", USER_ELEM_SERVICE_PROPERTY_WORDS);
    card.groupName = std::string(reader.text(3));
    card.userRoutineName = std::string(reader.text(4));
    card.dependence =
        readKeyword(reader, 5, "dependence", USER_ELEM_SERVICE_DEPENDENCE_WORDS, UserElemServiceDependence::Nominal);
    card.nominalSpeed = reader.optionalReal(LAST_HEAD_FIELD, "nominalSpeed");
    for (int field = LAST_HEAD_FIELD + 1; field <= LAST_HEAD_LINE_FIELD; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of an " + std::string(reader.cardName()) +
                             " is blank: the data start on the second line",
                         "");
            break;
        }
    }
    // The lines after the first begin a section each when their first field is INT, REAL or CHAR (or their second, when
    // the first is blank). A line that begins no section goes on with the one above it; before the first section there
    // is none.
    const std::vector<ContactSection> sections = splitContactSections(
        reader, {USER_ELEM_SERVICE_INTEGER_WORD, USER_ELEM_SERVICE_REAL_WORD, USER_ELEM_SERVICE_CHARACTER_WORD});
    if (sections.front().last > LAST_HEAD_LINE_FIELD)
        reader.error(
            "card.bad-value",
            "a line of an " + std::string(reader.cardName()) + " after the first begins with INT, REAL or CHAR", "");
    bool sawIntegers = false;
    bool sawReals = false;
    bool sawCharacters = false;
    for (std::size_t index = 1; index < sections.size(); ++index)
    {
        const ContactSection& section = sections[index];
        const int first = section.first + 1;
        if (section.keyword == USER_ELEM_SERVICE_INTEGER_WORD)
        {
            if (takeSection(reader, sawIntegers, USER_ELEM_SERVICE_INTEGER_WORD, "integerData"))
                card.integerData = readContactIds(reader, first, section.last, "integerData");
        }
        else if (section.keyword == USER_ELEM_SERVICE_REAL_WORD)
        {
            if (takeSection(reader, sawReals, USER_ELEM_SERVICE_REAL_WORD, "realData"))
                card.realData = readContactReals(reader, first, section.last, "realData");
        }
        else if (takeSection(reader, sawCharacters, USER_ELEM_SERVICE_CHARACTER_WORD, "textData"))
        {
            card.textData = readContactWords(reader, first, section.last);
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserElemServiceFormat::check(const UserElemServiceCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    if (!card.userRoutineName.empty())
    {
        if (const auto problem = checkNameWord(card.userRoutineName, "userRoutineName"))
            return problem;
    }
    if (const auto problem = checkOptionalReal(card.nominalSpeed, "nominalSpeed", RealRule::Finite))
        return problem;
    for (const double value : card.realData)
    {
        if (const auto problem = checkReal(value, "realData", RealRule::Finite))
            return problem;
    }
    for (const std::string& word : card.textData)
    {
        if (const auto problem = checkNameWord(word, "textData"))
            return problem;
        // the three words would open a section when the card is read again.
        if (isSectionWord(word))
            return CardProblem{"card.bad-value",
                               "a " + std::string(CardProblem::FIELD) + " cannot be the word INT, REAL or CHAR",
                               "textData"};
    }
    return std::nullopt;
}

void UserElemServiceFormat::write(const UserElemServiceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeyword(writer, 2, USER_ELEM_SERVICE_PROPERTY_WORDS, card.propType);
    writer.text(3, card.groupName);
    writer.text(4, card.userRoutineName);
    writeKeywordUnlessDefault(writer, 5, USER_ELEM_SERVICE_DEPENDENCE_WORDS, card.dependence,
                              UserElemServiceDependence::Nominal);
    if (card.nominalSpeed)
        writer.real(LAST_HEAD_FIELD, *card.nominalSpeed);
    int next = FIRST_SECTION_FIELD;
    if (!card.integerData.empty())
        next = writeContactIdList(writer, next, USER_ELEM_SERVICE_INTEGER_WORD, card.integerData);
    if (!card.realData.empty())
        next = writeContactRealList(writer, next, USER_ELEM_SERVICE_REAL_WORD, card.realData);
    if (!card.textData.empty())
        writeContactWordList(writer, next, USER_ELEM_SERVICE_CHARACTER_WORD, card.textData);
}

std::uint32_t UserElemServiceTable::append(const UserElemServiceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void UserElemServiceTable::store(std::uint32_t row, const UserElemServiceCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void UserElemServiceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void UserElemServiceTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void UserElemServiceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the PBUSH2D the card extends. The ID column follows the card (a companion owns its ID: the
    // renumbering of the properties moves both).
    UserElemServiceCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Property, card.id);
    m_id.set(row, card.id);
}

} // namespace oor::model
