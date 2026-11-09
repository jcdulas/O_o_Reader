#include "oor/model/cards/matrices/monitorGroup/MonitorGroupTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/MonitorPointFields.hpp"
#include "oor/model/cards/matrices/MonitorSumTerms.hpp"
#include "oor/model/cards/matrices/SpanTextRule.hpp"
#include "oor/model/core/CardRegistry.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// Each member is one line from the line after the head: NAME, CLASS, REAL, INT and the STRING over the last four
// fields.
constexpr int FIELDS_PER_LINE = 8;
constexpr int FIRST_MEMBER_LINE = 1;
constexpr int TEXT_ATTRIBUTE_FIELDS = 4;

bool isKnownClass(std::string_view word)
{
    for (const std::string_view known : MONITOR_GROUP_CLASS_WORDS)
    {
        if (known == word)
            return true;
    }
    return false;
}

// A class that starts with A is an aerodynamic monitor point (no neutral card name of the table starts with A).
bool isAerodynamicClass(std::string_view word)
{
    return !word.empty() && word.front() == 'A';
}

// True for an entry of the table that is the name of a card (the monitor point cards).
bool isCardName(std::string_view word)
{
    return CardRegistry::instance().typeIndexOf(word).has_value();
}

// The classes for a message: the cards by placeholders, which the reader turns into their keywords (CardProblem).
std::string classPhrase()
{
    std::string phrase;
    for (const std::string_view word : MONITOR_GROUP_CLASS_WORDS)
    {
        if (!phrase.empty())
            phrase += ", ";
        phrase += isCardName(word) ? CardProblem::cardNamed(word) : std::string(word);
    }
    return phrase;
}

} // namespace

std::optional<MonitorGroupCard> MonitorGroupFormat::read(FieldReader& reader)
{
    MonitorGroupCard card;
    card.groupName = std::string(reader.text(1));
    card.label = reader.spanText(2, 8);
    const int last = reader.lastUsedField();
    for (int line = FIRST_MEMBER_LINE; line * FIELDS_PER_LINE < last && !reader.failed(); ++line)
    {
        const int lineBase = line * FIELDS_PER_LINE;
        bool blankLine = true;
        for (int field = lineBase + 1; field <= lineBase + FIELDS_PER_LINE; ++field)
            blankLine = blankLine && reader.isBlank(field);
        if (blankLine)
            continue;
        MonitorGroupMember member;
        if (reader.isBlank(lineBase + 1))
            reader.error("card.missing-field",
                         std::string("memberName") + " is required on each line of a " + std::string(reader.cardName()),
                         "memberName");
        member.name = std::string(reader.text(lineBase + 1));
        if (reader.isBlank(lineBase + 2))
            reader.error(
                "card.missing-field",
                std::string("monitorClass") + " is required on each line of a " + std::string(reader.cardName()),
                "monitorClass");
        member.monitorClass = std::string(reader.text(lineBase + 2));
        member.real = reader.optionalReal(lineBase + 3, "realAttribute");
        member.integer = reader.optionalInteger(lineBase + 4, "integerAttribute");
        member.text = reader.spanText(lineBase + 5, lineBase + 4 + TEXT_ATTRIBUTE_FIELDS);
        card.members.push_back(std::move(member));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MonitorGroupFormat::check(const MonitorGroupCard& card)
{
    if (const auto problem = monitoredNameProblem(card.groupName, "groupName"))
        return problem;
    if (const auto problem = monitorLabelProblem(card.label))
        return problem;
    if (card.members.empty())
        return CardProblem{"card.missing-field", "a monitor group needs at least one {field}", "memberName"};
    for (const MonitorGroupMember& member : card.members)
    {
        if (const auto problem = monitoredNameProblem(member.name, "memberName"))
            return problem;
        if (!isKnownClass(member.monitorClass))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be one of " + classPhrase(),
                               "monitorClass"};
        if (member.real && !std::isfinite(*member.real))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                               "realAttribute"};
        if (const auto problem = spanTextProblem(member.text, "textAttribute"))
            return problem;
        // The aerodynamic and the structural monitor points cannot be mixed in a group.
        if (isAerodynamicClass(member.monitorClass) != isAerodynamicClass(card.members.front().monitorClass))
            return CardProblem{"card.bad-value",
                               "the aerodynamic and the structural monitor points cannot be mixed in a group",
                               "monitorClass"};
    }
    return std::nullopt;
}

void MonitorGroupFormat::write(const MonitorGroupCard& card, FieldWriter& writer)
{
    writer.text(1, card.groupName);
    writer.spanText(2, 7, card.label);
    int line = FIRST_MEMBER_LINE;
    for (const MonitorGroupMember& member : card.members)
    {
        const int lineBase = line * FIELDS_PER_LINE;
        writer.text(lineBase + 1, member.name);
        writer.text(lineBase + 2, member.monitorClass);
        if (member.real)
            writer.real(lineBase + 3, *member.real);
        if (member.integer)
            writer.integer(lineBase + 4, *member.integer);
        writer.spanText(lineBase + 5, TEXT_ATTRIBUTE_FIELDS, member.text);
        ++line;
    }
}

std::uint32_t MonitorGroupTable::append(const MonitorGroupCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_groupName.push(card.groupName);
    m_cards.push(card);
    return row;
}

void MonitorGroupTable::store(std::uint32_t row, const MonitorGroupCard& card)
{
    m_groupName.set(row, card.groupName);
    m_cards.set(row, card);
}

void MonitorGroupTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_groupName.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
