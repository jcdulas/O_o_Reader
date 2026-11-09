#include "oor/model/cards/nonlinear_contact/ContactSectionFields.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>

namespace oor::model
{

void readContactSectionFields(FieldReader& reader, const ContactSection& section,
                              std::span<const ContactFieldPosition> positions, std::initializer_list<int> ownOffsets,
                              std::vector<ContactParameter>& params, std::string_view owner)
{
    const int lineStart = firstFieldOfLine((section.first - 1) / CONTACT_LINE_FIELDS);
    std::vector<bool> used(static_cast<std::size_t>(section.last) + 1, false);
    used[static_cast<std::size_t>(section.first)] = true; // the keyword
    for (const int offset : ownOffsets)
    {
        if (const int field = lineStart + offset - 1; field <= section.last)
            used[static_cast<std::size_t>(field)] = true;
    }
    for (const ContactFieldPosition& position : positions)
    {
        if (position.keyword != section.keyword)
            continue;
        const int field = lineStart + CONTACT_LINE_FIELDS * position.line + position.offset - 1;
        if (field > section.last)
            continue;
        used[static_cast<std::size_t>(field)] = true;
        if (reader.isBlank(field))
            continue;
        params.push_back(ContactParameter{std::string(position.rule.name),
                                          readContactParameterValue(reader, field, position.rule, owner)});
    }
    for (int field = section.first + 1; field <= section.last; ++field)
    {
        if (!used[static_cast<std::size_t>(field)] && !reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of the " + section.keyword + " line is not used",
                         "params");
            return;
        }
    }
}

int contactSectionLines(const std::vector<ContactParameter>& params, std::string_view keyword,
                        std::span<const ContactFieldPosition> positions, int minimum)
{
    int lines = minimum;
    for (const ContactParameter& parameter : params)
    {
        const ContactFieldPosition* position = findContactFieldPosition(positions, parameter.name);
        if (position != nullptr && position->keyword == keyword)
            lines = std::max(lines, position->line + 1);
    }
    return lines;
}

int writeContactSectionFields(FieldWriter& writer, int lineStart, int keywordOffset, std::string_view keyword,
                              const std::vector<ContactParameter>& params,
                              std::span<const ContactFieldPosition> positions, int minimumLines)
{
    writer.text(lineStart + keywordOffset - 1, keyword);
    for (const ContactParameter& parameter : params)
    {
        const ContactFieldPosition* position = findContactFieldPosition(positions, parameter.name);
        if (position == nullptr || position->keyword != keyword)
            continue;
        writeContactValue(writer, lineStart + CONTACT_LINE_FIELDS * position->line + position->offset - 1,
                          parameter.value);
    }
    return lineStart + CONTACT_LINE_FIELDS * contactSectionLines(params, keyword, positions, minimumLines);
}

const ContactFieldPosition* findContactFieldPosition(std::span<const ContactFieldPosition> positions,
                                                     std::string_view name)
{
    for (const ContactFieldPosition& position : positions)
    {
        if (position.rule.name == name)
            return &position;
    }
    return nullptr;
}

std::vector<ContactParameterRule> contactFieldRules(std::span<const ContactFieldPosition> positions)
{
    std::vector<ContactParameterRule> rules;
    rules.reserve(positions.size());
    for (const ContactFieldPosition& position : positions)
        rules.push_back(position.rule);
    return rules;
}

std::vector<ContactNamedSection> readContactNamedSections(FieldReader& reader,
                                                          std::span<const std::string_view> keywords,
                                                          std::span<const ContactFieldPosition> positions,
                                                          std::string_view owner)
{
    std::vector<ContactNamedSection> result;
    for (const ContactSection& section : splitContactSections(reader, keywords))
    {
        if (section.keyword.empty())
        {
            // The head of the card, which the card read itself; more lines in it have no keyword to go with.
            if (section.last > CONTACT_LINE_FIELDS)
                reader.error("card.bad-value",
                             "a continuation line of " + std::string(reader.cardName()) + " must start with a keyword",
                             "sections");
            continue;
        }
        ContactNamedSection& entry = result.emplace_back();
        entry.keyword = section.keyword;
        readContactSectionFields(reader, section, positions, {}, entry.params, owner);
    }
    return result;
}

void writeContactNamedSections(FieldWriter& writer, const std::vector<ContactNamedSection>& sections,
                               std::span<const ContactFieldPosition> positions)
{
    int line = firstFieldOfLine(1);
    for (const ContactNamedSection& section : sections)
        line = writeContactSectionFields(writer, line, 1, section.keyword, section.params, positions);
}

std::optional<CardProblem> checkContactNamedSections(const std::vector<ContactNamedSection>& sections,
                                                     std::span<const std::string_view> keywords,
                                                     std::span<const ContactFieldPosition> positions,
                                                     std::span<const ContactParameterRule> rules,
                                                     std::string_view owner)
{
    for (std::size_t index = 0; index < sections.size(); ++index)
    {
        const ContactNamedSection& section = sections[index];
        if (std::find(keywords.begin(), keywords.end(), section.keyword) == keywords.end())
            return CardProblem{"card.bad-value", "\"" + section.keyword + "\" is not a keyword of this card",
                               "sections"};
        for (std::size_t other = 0; other < index; ++other)
        {
            if (sections[other].keyword == section.keyword)
                return CardProblem{"card.bad-value", section.keyword + " is given twice", "sections"};
        }
        if (auto problem = checkContactParameters(section.params, rules, owner))
        {
            problem->field = "sections";
            return problem;
        }
        for (std::size_t first = 0; first < section.params.size(); ++first)
        {
            const std::string& name = section.params[first].name;
            const std::string label = std::string(name);
            const ContactFieldPosition* position = findContactFieldPosition(positions, name);
            if (position == nullptr || position->keyword != section.keyword)
                return CardProblem{"card.bad-value", label + " is not a field of " + section.keyword, "sections"};
            for (std::size_t second = 0; second < first; ++second)
            {
                if (section.params[second].name == name)
                    return CardProblem{"card.bad-value", label + " is given twice", "sections"};
            }
        }
    }
    return std::nullopt;
}

} // namespace oor::model
