#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

#include "oor/base/text/TextUtilities.hpp"

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

} // namespace

std::optional<std::size_t> findBehaviorRules(std::span<const BehaviorRules> rules, std::string_view text) noexcept
{
    for (std::size_t index = 0; index < rules.size(); ++index)
    {
        if ((std::string_view(rules[index].keyword) == text))
            return index;
    }
    return std::nullopt;
}

std::string behaviorKeywordList(std::span<const BehaviorRules> rules)
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

BehaviorGroup readBehaviorGroup(FieldReader& reader, int keywordField, const BehaviorRules& rules)
{
    BehaviorGroup group = defaultGroup(rules);
    group.behavior = readGroupWord<ElementBehavior>(reader, keywordField + 1, rules.behaviorName,
                                                    ELEMENT_BEHAVIOR_WORDS, rules.behaviors);
    group.integration = readGroupWord<IntegrationScheme>(reader, keywordField + 2, rules.integrationName,
                                                         INTEGRATION_SCHEME_WORDS, rules.integrations);
    int lastWordField = keywordField + 2;
    if (rules.hasHeat())
    {
        group.heatBehavior = readGroupWord<ElementBehavior>(reader, keywordField + 3, rules.heatBehaviorName,
                                                            ELEMENT_BEHAVIOR_WORDS, rules.heatBehaviors);
        group.heatIntegration = readGroupWord<IntegrationScheme>(reader, keywordField + 4, rules.heatIntegrationName,
                                                                 INTEGRATION_SCHEME_WORDS, rules.heatIntegrations);
        lastWordField = keywordField + 4;
    }
    for (int field = lastWordField + 1; field < keywordField + EXTENSION_LINE_FIELDS; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         std::string("the ") + rules.keyword + " line has its keyword and its " +
                             (rules.hasHeat() ? "four" : "two") + " words only",
                         rules.keyword);
            break;
        }
    }
    return group;
}

bool writesBehaviorGroup(const FieldWriter& writer, const BehaviorRules& rules, const BehaviorGroup& group)
{
    return !writer.omitsDefaults() || group != defaultGroup(rules);
}

void writeBehaviorGroup(FieldWriter& writer, int keywordField, const BehaviorRules& rules, const BehaviorGroup& group)
{
    const BehaviorGroup fallback = defaultGroup(rules);
    writer.text(keywordField, rules.keyword);
    writeKeywordUnlessDefault(writer, keywordField + 1, ELEMENT_BEHAVIOR_WORDS, group.behavior, fallback.behavior);
    writeKeywordUnlessDefault(writer, keywordField + 2, INTEGRATION_SCHEME_WORDS, group.integration,
                              fallback.integration);
    if (!rules.hasHeat())
        return;
    writeKeywordUnlessDefault(writer, keywordField + 3, ELEMENT_BEHAVIOR_WORDS, group.heatBehavior,
                              fallback.heatBehavior);
    writeKeywordUnlessDefault(writer, keywordField + 4, INTEGRATION_SCHEME_WORDS, group.heatIntegration,
                              fallback.heatIntegration);
}

std::optional<CardProblem> checkBehaviorGroup(const BehaviorGroup& group, const BehaviorRules& rules)
{
    const auto allows = [](auto allowed, auto word)
    { return std::find(allowed.begin(), allowed.end(), word) != allowed.end(); };
    if (!allows(rules.behaviors, group.behavior))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be " +
                               allowedWordList<ElementBehavior>(ELEMENT_BEHAVIOR_WORDS, rules.behaviors),
                           rules.behaviorName};
    if (!allows(rules.integrations, group.integration))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be " +
                               allowedWordList<IntegrationScheme>(INTEGRATION_SCHEME_WORDS, rules.integrations),
                           rules.integrationName};
    if (!rules.hasHeat())
        return std::nullopt;
    if (!allows(rules.heatBehaviors, group.heatBehavior))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be " +
                               allowedWordList<ElementBehavior>(ELEMENT_BEHAVIOR_WORDS, rules.heatBehaviors),
                           rules.heatBehaviorName};
    if (!allows(rules.heatIntegrations, group.heatIntegration))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be " +
                               allowedWordList<IntegrationScheme>(INTEGRATION_SCHEME_WORDS, rules.heatIntegrations),
                           rules.heatIntegrationName};
    return std::nullopt;
}

void readBehaviorLines(FieldReader& reader, int firstField, std::span<const BehaviorRules> rules,
                       std::span<BehaviorGroup> groups)
{
    std::vector<bool> given(rules.size(), false);
    for (int line = firstField; line <= reader.lastUsedField(); line += EXTENSION_LINE_FIELDS)
    {
        if (lineIsBlank(reader, line))
            continue;
        const std::string_view keyword = reader.text(line);
        const std::optional<std::size_t> index = findBehaviorRules(rules, keyword);
        if (!index)
        {
            reader.error("card.bad-value",
                         "a continuation line starts with " + behaviorKeywordList(rules) + ", not \"" +
                             std::string(keyword) + "\"",
                         "sectionKey");
            continue;
        }
        if (given[*index])
        {
            reader.error("card.bad-value", std::string("the keyword ") + rules[*index].keyword + " is given twice",
                         rules[*index].keyword);
            continue;
        }
        given[*index] = true;
        groups[*index] = readBehaviorGroup(reader, line, rules[*index]);
    }
}

int writeBehaviorLines(FieldWriter& writer, int firstField, std::span<const BehaviorRules> rules,
                       std::span<const BehaviorGroup> groups)
{
    int field = firstField;
    for (std::size_t index = 0; index < rules.size(); ++index)
    {
        if (!writesBehaviorGroup(writer, rules[index], groups[index]))
            continue;
        writeBehaviorGroup(writer, field, rules[index], groups[index]);
        field += EXTENSION_LINE_FIELDS;
    }
    return field;
}

std::optional<CardProblem> checkBehaviorLines(std::span<const BehaviorRules> rules,
                                              std::span<const BehaviorGroup> groups)
{
    for (std::size_t index = 0; index < rules.size(); ++index)
    {
        if (const auto problem = checkBehaviorGroup(groups[index], rules[index]))
            return problem;
    }
    return std::nullopt;
}

void requireUnusedFields(FieldReader& reader, std::initializer_list<int> fields)
{
    for (const int field : fields)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value", "field " + std::to_string(field + 1) + " of a " +
                                               std::string(reader.cardName()) +
                                               " is not used (the format reference leaves it blank)");
            return;
        }
    }
}

} // namespace oor::model
