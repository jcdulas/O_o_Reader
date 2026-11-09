#include "oor/model/cards/properties/ShellExtensionWords.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace oor::model
{

namespace
{

// True when every field of the line that starts at `first` is blank.
bool lineIsBlank(const FieldReader& reader, int first)
{
    for (int field = first; field < first + EXTENSION_LINE_FIELDS; ++field)
    {
        if (!reader.isBlank(field))
            return false;
    }
    return true;
}

// True when some row of `table` has `word` in the member `member` (the behaviour or the integration).
template <typename Word>
bool tableHasWord(std::span<const ShellWords> table, Word ShellWords::* member, Word word) noexcept
{
    return std::any_of(table.begin(), table.end(), [&](const ShellWords& row) { return row.*member == word; });
}

// The words of that member that the table lists, each once, in the order of the table: "DCT, DCTN or MB".
template <typename Word, std::size_t COUNT>
std::string tableWordList(std::span<const ShellWords> table, Word ShellWords::* member,
                          const std::array<std::string_view, COUNT>& words)
{
    std::vector<Word> listed;
    for (const ShellWords& row : table)
    {
        if (std::find(listed.begin(), listed.end(), row.*member) == listed.end())
            listed.push_back(row.*member);
    }
    std::string phrase;
    for (std::size_t index = 0; index < listed.size(); ++index)
    {
        if (index > 0)
            phrase += index + 1 == listed.size() ? " or " : ", ";
        phrase += words[static_cast<std::size_t>(listed[index])];
    }
    return phrase;
}

// The first integration the table lists for `behavior`.
std::optional<ShellIntegration> firstIntegrationOf(std::span<const ShellWords> table, ShellBehavior behavior) noexcept
{
    for (const ShellWords& row : table)
    {
        if (row.behavior == behavior)
            return row.integration;
    }
    return std::nullopt;
}

ShellWords resolveWords(std::span<const ShellWords> table, const std::optional<ShellBehavior>& behavior,
                        const std::optional<ShellIntegration>& integration) noexcept
{
    ShellWords words = table.front();
    if (behavior)
    {
        words.behavior = *behavior;
        words.integration = firstIntegrationOf(table, *behavior).value_or(table.front().integration);
    }
    if (integration)
        words.integration = *integration;
    return words;
}

// The word in `field`, if the field is not blank: a word of the format reference that the table of the line lists.
template <typename Word, std::size_t COUNT>
std::optional<Word> readWord(FieldReader& reader, int field, const char* name, std::span<const ShellWords> table,
                             Word ShellWords::* member, const std::array<std::string_view, COUNT>& words)
{
    const std::string_view text = reader.text(field);
    if (text.empty())
        return std::nullopt;
    if (const std::optional<Word> word = parseKeyword<Word>(words, text); word && tableHasWord(table, member, *word))
        return word;
    reader.error(
        "card.bad-value",
        std::string(name) + " must be " + tableWordList(table, member, words) + ", not \"" + std::string(text) + "\"",
        name);
    return std::nullopt;
}

ShellLine readShellLine(FieldReader& reader, int keywordField, const ShellLineRules& rules)
{
    ShellLineGiven given;
    given.behavior = readWord(reader, keywordField + 1, rules.behaviorName, rules.structural, &ShellWords::behavior,
                              SHELL_BEHAVIOR_WORDS);
    given.integration = readWord(reader, keywordField + 2, rules.integrationName, rules.structural,
                                 &ShellWords::integration, SHELL_INTEGRATION_WORDS);
    given.heatBehavior = readWord(reader, keywordField + 3, rules.heatBehaviorName, rules.heat, &ShellWords::behavior,
                                  SHELL_BEHAVIOR_WORDS);
    given.heatIntegration = readWord(reader, keywordField + 4, rules.heatIntegrationName, rules.heat,
                                     &ShellWords::integration, SHELL_INTEGRATION_WORDS);
    for (int field = keywordField + 5; field < keywordField + EXTENSION_LINE_FIELDS; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         std::string("the ") + rules.keyword + " line has its keyword and its four words only",
                         rules.keyword);
            break;
        }
    }
    return resolveShellLine(rules, given);
}

// "C3, C4, C6 or C8": the keywords of the rules, for a message.
std::string lineKeywordList(std::span<const ShellLineRules> rules)
{
    std::string phrase;
    for (std::size_t index = 0; index < rules.size(); ++index)
    {
        if (index > 0)
            phrase += index + 1 == rules.size() ? " or " : ", ";
        phrase += rules[index].keyword;
    }
    return phrase;
}

} // namespace

ShellLine resolveShellLine(const ShellLineRules& rules, const ShellLineGiven& given) noexcept
{
    return ShellLine{resolveWords(rules.structural, given.behavior, given.integration),
                     resolveWords(rules.heat, given.heatBehavior, given.heatIntegration)};
}

std::optional<std::size_t> findShellLineRules(std::span<const ShellLineRules> rules, std::string_view text) noexcept
{
    for (std::size_t index = 0; index < rules.size(); ++index)
    {
        if ((std::string_view(rules[index].keyword) == text))
            return index;
    }
    return std::nullopt;
}

void readShellLines(FieldReader& reader, int firstField, std::span<const ShellLineRules> rules,
                    std::span<ShellLine> lines)
{
    std::vector<bool> given(rules.size(), false);
    for (int line = firstField; line <= reader.lastUsedField(); line += EXTENSION_LINE_FIELDS)
    {
        if (lineIsBlank(reader, line))
            continue;
        const std::string_view keyword = reader.text(line);
        const std::optional<std::size_t> index = findShellLineRules(rules, keyword);
        if (!index)
        {
            reader.error(
                "card.bad-value",
                "a continuation line starts with " + lineKeywordList(rules) + ", not \"" + std::string(keyword) + "\"",
                "sectionKey");
            continue;
        }
        if (given[*index])
        {
            reader.error("card.bad-value", std::string("the keyword ") + rules[*index].keyword + " is given twice",
                         std::string(rules[*index].keyword).c_str());
            continue;
        }
        given[*index] = true;
        lines[*index] = readShellLine(reader, line, rules[*index]);
    }
}

int writeShellLines(FieldWriter& writer, int firstField, std::span<const ShellLineRules> rules,
                    std::span<const ShellLine> lines)
{
    int field = firstField;
    for (std::size_t index = 0; index < rules.size(); ++index)
    {
        if (writer.omitsDefaults() && lines[index] == defaultLine(rules[index]))
            continue;
        writer.text(field, rules[index].keyword);
        writer.text(field + 1, SHELL_BEHAVIOR_WORDS[static_cast<std::size_t>(lines[index].structural.behavior)]);
        writer.text(field + 2, SHELL_INTEGRATION_WORDS[static_cast<std::size_t>(lines[index].structural.integration)]);
        writer.text(field + 3, SHELL_BEHAVIOR_WORDS[static_cast<std::size_t>(lines[index].heat.behavior)]);
        writer.text(field + 4, SHELL_INTEGRATION_WORDS[static_cast<std::size_t>(lines[index].heat.integration)]);
        field += EXTENSION_LINE_FIELDS;
    }
    return field;
}

std::optional<CardProblem> checkShellLines(std::span<const ShellLineRules> rules, std::span<const ShellLine> lines)
{
    for (std::size_t index = 0; index < rules.size(); ++index)
    {
        const ShellLineRules& line = rules[index];
        const ShellLine& words = lines[index];
        if (!tableHasWord(line.structural, &ShellWords::behavior, words.structural.behavior))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be " +
                                   tableWordList(line.structural, &ShellWords::behavior, SHELL_BEHAVIOR_WORDS),
                               line.behaviorName};
        if (!tableHasWord(line.structural, &ShellWords::integration, words.structural.integration))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be " +
                                   tableWordList(line.structural, &ShellWords::integration, SHELL_INTEGRATION_WORDS),
                               line.integrationName};
        if (!tableHasWord(line.heat, &ShellWords::behavior, words.heat.behavior))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be " +
                                   tableWordList(line.heat, &ShellWords::behavior, SHELL_BEHAVIOR_WORDS),
                               line.heatBehaviorName};
        if (!tableHasWord(line.heat, &ShellWords::integration, words.heat.integration))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be " +
                                   tableWordList(line.heat, &ShellWords::integration, SHELL_INTEGRATION_WORDS),
                               line.heatIntegrationName};
    }
    return std::nullopt;
}

} // namespace oor::model
