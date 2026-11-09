#include "oor/model/cards/optimisation/DesignGroups.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>
#include <string>

namespace oor::model
{

namespace
{

// The fields of one line of the card: the keyword and then the values.
constexpr int FIELDS_PER_LINE = 8;
constexpr int VALUES_PER_LINE = 7;
constexpr int COMPONENT_VALUES_PER_LINE = 6; // three pairs and a blank field, as the format reference draws DNODE

int valuesPerLine(GroupContent content) noexcept
{
    switch (content)
    {
    case GroupContent::Integers:
    case GroupContent::Words:
        return VALUES_PER_LINE;
    case GroupContent::Components:
        return COMPONENT_VALUES_PER_LINE;
    case GroupContent::Text:
        break;
    }
    return 1;
}

std::optional<std::size_t> indexOfKeyword(std::span<const GroupKeyword> keywords, std::string_view keyword) noexcept
{
    for (std::size_t index = 0; index < keywords.size(); ++index)
    {
        if (keywords[index].keyword == keyword)
            return index;
    }
    return std::nullopt;
}

// "DESVAR, DTABLE or DRESP1": the keywords of a card, for a message; `labelOf` gives the label of one keyword.
template <typename LabelOf>
std::string keywordPhrase(std::span<const GroupKeyword> keywords, const LabelOf& labelOf)
{
    std::string phrase;
    for (std::size_t index = 0; index < keywords.size(); ++index)
    {
        if (index > 0)
            phrase += index + 1 == keywords.size() ? " or " : ", ";
        phrase += labelOf(keywords[index].keyword);
    }
    return phrase;
}

std::string keywordPhrase(std::span<const GroupKeyword> keywords)
{
    return keywordPhrase(keywords, [](std::string_view name) { return CardProblem::cardNamed(name); });
}

bool isWordCharacter(char character) noexcept
{
    const auto code = static_cast<unsigned char>(character);
    return code > 0x20 && code < 0x7F && character != ',' && character != '$' && character != '\'' && character != '"';
}

} // namespace

std::vector<DesignGroup> readDesignGroups(FieldReader& reader, int firstField, std::span<const GroupKeyword> keywords)
{
    std::vector<DesignGroup> groups;
    std::optional<std::size_t> current; // the index in `keywords` of the group being read
    const auto deckPhrase = [&]()
    { return keywordPhrase(keywords, [&](std::string_view name) { return std::string(name); }); };
    const int last = reader.lastUsedField();
    for (int line = firstField; line <= last; line += FIELDS_PER_LINE)
    {
        const std::string_view text = reader.text(line);
        if (!text.empty())
        {
            const std::string word(text);
            const std::optional<std::size_t> index = indexOfKeyword(keywords, word);
            if (!index)
            {
                reader.error("card.bad-value", "\"" + word + "\" is not a keyword of this card: " + deckPhrase(),
                             "groupKeyword");
                return groups;
            }
            const bool goesOn = current && *current == *index;
            if (!goesOn)
            {
                if (current && *index < *current)
                {
                    reader.error("card.bad-value",
                                 word + " must come before " + std::string(keywords[*current].keyword) +
                                     ": the keywords go in the order " + deckPhrase(),
                                 "groupKeyword");
                    return groups;
                }
                const std::string_view keyword = keywords[*index].keyword;
                if (std::any_of(groups.begin(), groups.end(),
                                [&](const DesignGroup& group) { return group.keyword == keyword; }))
                {
                    reader.error("card.bad-value", word + " is given twice", "groupKeyword");
                    return groups;
                }
                DesignGroup group;
                group.keyword = std::string(keyword);
                groups.push_back(std::move(group));
                current = *index;
            }
        }
        else if (!current)
        {
            bool hasValue = false;
            for (int field = line + 1; field < line + FIELDS_PER_LINE; ++field)
                hasValue = hasValue || !reader.isBlank(field);
            if (!hasValue)
                continue;
            reader.error("card.missing-field", "values with no keyword before them: use " + deckPhrase(),
                         "groupKeyword");
            return groups;
        }
        if (!current)
            continue;
        const GroupKeyword& spec = keywords[*current];
        DesignGroup& group = groups.back();
        if (spec.content == GroupContent::Text)
        {
            const std::string lineText = reader.spanText(line + 1, line + VALUES_PER_LINE);
            if (!lineText.empty())
                group.words.push_back(lineText);
            continue;
        }
        for (int field = line + 1; field <= line + VALUES_PER_LINE; ++field)
        {
            if (reader.isBlank(field))
                continue;
            if (spec.content == GroupContent::Words)
                group.words.emplace_back(reader.text(field));
            else
                group.ids.push_back(reader.requiredInteger(field, spec.valueName));
        }
    }
    return groups;
}

void writeDesignGroups(FieldWriter& writer, int firstField, const std::vector<DesignGroup>& groups,
                       std::span<const GroupKeyword> keywords)
{
    int line = firstField;
    for (const DesignGroup& group : groups)
    {
        const std::optional<std::size_t> index = indexOfKeyword(keywords, group.keyword);
        if (!index)
            continue; // a card is checked before it is written
        const GroupContent content = keywords[*index].content;
        const std::size_t count = content == GroupContent::Integers || content == GroupContent::Components
                                      ? group.ids.size()
                                      : group.words.size();
        const std::size_t perLine = static_cast<std::size_t>(valuesPerLine(content));
        std::size_t written = 0;
        bool first = true;
        while (first || written < count)
        {
            writer.text(line, first ? std::string_view(group.keyword) : std::string_view{});
            first = false;
            if (content == GroupContent::Text)
            {
                if (written < count)
                    writer.spanText(line + 1, VALUES_PER_LINE, group.words[written++]);
            }
            else
            {
                for (std::size_t slot = 0; slot < perLine && written < count; ++slot, ++written)
                {
                    const int field = line + 1 + static_cast<int>(slot);
                    if (content == GroupContent::Words)
                        writer.text(field, group.words[written]);
                    else
                        writer.integer(field, group.ids[written]);
                }
            }
            line += FIELDS_PER_LINE;
        }
    }
}

std::optional<CardProblem> checkDesignGroups(const std::vector<DesignGroup>& groups,
                                             std::span<const GroupKeyword> keywords)
{
    std::optional<std::size_t> previous;
    for (const DesignGroup& group : groups)
    {
        const std::optional<std::size_t> index = indexOfKeyword(keywords, group.keyword);
        if (!index)
            return CardProblem{"card.bad-value",
                               "\"" + CardProblem::cardNamed(group.keyword) +
                                   "\" is not a keyword of this card: " + keywordPhrase(keywords),
                               "groupKeyword"};
        if (previous && *index <= *previous)
            return CardProblem{"card.bad-value",
                               CardProblem::cardNamed(group.keyword) +
                                   " is out of order or given twice: the keywords go in the order " +
                                   keywordPhrase(keywords),
                               "groupKeyword"};
        previous = *index;
        const GroupKeyword& spec = keywords[*index];
        switch (spec.content)
        {
        case GroupContent::Integers:
        case GroupContent::Components:
            for (const base::EntityId id : group.ids)
            {
                if (const auto problem = checkRequiredId(id, spec.valueName))
                    return problem;
            }
            if (spec.content == GroupContent::Components && group.ids.size() % 2 != 0)
                return CardProblem{"card.bad-value",
                                   CardProblem::cardNamed(group.keyword) + " needs a grid point and a component each",
                                   "groupKeyword"};
            break;
        case GroupContent::Words:
            for (const std::string& word : group.words)
            {
                if (word.empty() || !std::all_of(word.begin(), word.end(), isWordCharacter))
                    return CardProblem{
                        "card.bad-value",
                        std::string(CardProblem::FIELD) + " must be a name with no blank, comma, dollar sign or quote",
                        spec.valueName};
            }
            break;
        case GroupContent::Text:
            break;
        }
    }
    return std::nullopt;
}

const DesignGroup* findDesignGroup(const std::vector<DesignGroup>& groups, std::string_view keyword) noexcept
{
    for (const DesignGroup& group : groups)
    {
        if (group.keyword == keyword)
            return &group;
    }
    return nullptr;
}

DesignGroup* findDesignGroup(std::vector<DesignGroup>& groups, std::string_view keyword) noexcept
{
    for (DesignGroup& group : groups)
    {
        if (group.keyword == keyword)
            return &group;
    }
    return nullptr;
}

void setDesignGroup(std::vector<DesignGroup>& groups, std::span<const GroupKeyword> keywords, DesignGroup group)
{
    const std::optional<std::size_t> position = indexOfKeyword(keywords, group.keyword);
    if (!position)
        return;
    std::erase_if(groups, [&](const DesignGroup& existing) { return existing.keyword == group.keyword; });
    if (group.ids.empty() && group.words.empty())
        return;
    groups.push_back(std::move(group));
    std::stable_sort(groups.begin(), groups.end(),
                     [&](const DesignGroup& left, const DesignGroup& right)
                     {
                         const auto leftIndex = indexOfKeyword(keywords, left.keyword);
                         const auto rightIndex = indexOfKeyword(keywords, right.keyword);
                         return leftIndex.value_or(keywords.size()) < rightIndex.value_or(keywords.size());
                     });
}

void visitGroupIds(ReferenceVisitor& visitor, DesignGroup& group, std::string_view fieldName, ReferenceTarget target)
{
    for (base::EntityId& id : group.ids)
        visitId(visitor, fieldName, target, id);
}

} // namespace oor::model
