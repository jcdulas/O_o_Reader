#include "oor/model/cards/superelements/SuperelementGroups.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int PAIRS_PER_LINE = 3;
constexpr int NODES_PER_LINE = 2;
constexpr int LAST_TEXT_OFFSET = FIELDS_PER_LINE - 1; // the text runs over the seven fields after the keyword

int itemsPerLine(SuperelementGroupContent content) noexcept
{
    switch (content)
    {
    case SuperelementGroupContent::Pairs:
    case SuperelementGroupContent::Labels:
        return PAIRS_PER_LINE;
    case SuperelementGroupContent::Nodes:
        return NODES_PER_LINE;
    case SuperelementGroupContent::Text:
        return 1;
    }
    return 1;
}

// The fields of a line after the keyword that its items do not use: they must be blank.
bool hasLeftovers(const FieldReader& reader, int line, SuperelementGroupContent content)
{
    int firstUnused = line + 1;
    switch (content)
    {
    case SuperelementGroupContent::Pairs:
    case SuperelementGroupContent::Labels:
        firstUnused = line + 1 + 2 * PAIRS_PER_LINE;
        break;
    case SuperelementGroupContent::Nodes:
        firstUnused = line + 1 + 3 * NODES_PER_LINE;
        break;
    case SuperelementGroupContent::Text:
        return false;
    }
    for (int field = firstUnused; field < line + FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

bool lineIsBlank(const FieldReader& reader, int line)
{
    for (int field = line; field < line + FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
            return false;
    }
    return true;
}

// Reads the items of one line of a group.
void readLine(FieldReader& reader, int line, const SuperelementGroupKeyword& keyword, SuperelementGroup& group)
{
    const std::string name = std::string(keyword.valueName);
    if (keyword.content == SuperelementGroupContent::Text)
    {
        const std::string text = reader.spanText(line + 1, line + LAST_TEXT_OFFSET);
        if (!text.empty())
            group.lines.push_back(text);
        return;
    }
    const bool nodes = keyword.content == SuperelementGroupContent::Nodes;
    const int count = itemsPerLine(keyword.content);
    const int width = nodes ? 3 : 2;
    for (int slot = 0; slot < count && !reader.failed(); ++slot)
    {
        const int first = line + 1 + slot * width;
        bool blank = true;
        for (int field = first; field < first + width; ++field)
            blank = blank && reader.isBlank(field);
        if (blank)
            continue; // the padding of a short line
        SuperelementGroupItem item;
        item.superelementId = reader.requiredInteger(first, "superelementId");
        if (keyword.content == SuperelementGroupContent::Labels)
        {
            item.label = std::string(reader.text(first + 1));
            if (item.label.empty())
                reader.error("card.missing-field", name + " is required", keyword.valueName);
        }
        else
        {
            item.id = reader.requiredInteger(first + 1, keyword.valueName);
            if (nodes)
                item.component = reader.requiredInteger(first + 2, "nodeComponents");
        }
        group.items.push_back(std::move(item));
    }
    if (!reader.failed() && hasLeftovers(reader, line, keyword.content))
        reader.error("card.bad-value",
                     "a line of the group " + std::string(group.keyword) + " has fields after its items",
                     keyword.valueName);
}

} // namespace

const SuperelementGroupKeyword* findSuperelementGroupKeyword(std::span<const SuperelementGroupKeyword> keywords,
                                                             std::string_view keyword) noexcept
{
    for (const SuperelementGroupKeyword& candidate : keywords)
    {
        if (candidate.keyword == keyword)
            return &candidate;
    }
    return nullptr;
}

std::vector<SuperelementGroup> readSuperelementGroups(FieldReader& reader, int firstField,
                                                      std::span<const SuperelementGroupKeyword> keywords)
{
    std::vector<SuperelementGroup> groups;
    const int last = reader.lastUsedField();
    for (int line = firstField; line <= last && !reader.failed(); line += FIELDS_PER_LINE)
    {
        const std::string word(reader.text(line));
        if (!word.empty())
        {
            const SuperelementGroupKeyword* entry = findSuperelementGroupKeyword(keywords, word);
            if (entry == nullptr)
            {
                reader.error("card.bad-value", "\"" + word + "\" is not a keyword of this card", "keyword");
                break;
            }
            if (groups.empty() || groups.back().keyword != entry->keyword)
            {
                SuperelementGroup group;
                group.keyword = std::string(entry->keyword);
                groups.push_back(std::move(group));
            }
        }
        else if (lineIsBlank(reader, line))
        {
            continue;
        }
        else if (groups.empty())
        {
            reader.error("card.bad-value", "values come before the first keyword of a group", "keyword");
            break;
        }
        readLine(reader, line, *findSuperelementGroupKeyword(keywords, groups.back().keyword), groups.back());
    }
    return groups;
}

void writeSuperelementGroups(FieldWriter& writer, int firstField, const std::vector<SuperelementGroup>& groups,
                             std::span<const SuperelementGroupKeyword> keywords)
{
    int line = firstField;
    for (const SuperelementGroup& group : groups)
    {
        const SuperelementGroupKeyword* keyword = findSuperelementGroupKeyword(keywords, group.keyword);
        if (keyword == nullptr)
            continue; // the check of the card reports it
        writer.text(line, group.keyword);
        if (keyword->content == SuperelementGroupContent::Text)
        {
            for (std::size_t index = 0; index < group.lines.size(); ++index)
            {
                if (index > 0)
                    line += FIELDS_PER_LINE;
                writer.spanText(line + 1, FIELDS_PER_LINE - 1, group.lines[index]);
            }
            line += FIELDS_PER_LINE;
            continue;
        }
        const int perLine = itemsPerLine(keyword->content);
        const int width = keyword->content == SuperelementGroupContent::Nodes ? 3 : 2;
        int slot = 0;
        for (const SuperelementGroupItem& item : group.items)
        {
            if (slot == perLine)
            {
                slot = 0;
                line += FIELDS_PER_LINE;
            }
            const int first = line + 1 + slot * width;
            writer.integer(first, item.superelementId);
            if (keyword->content == SuperelementGroupContent::Labels)
            {
                writer.text(first + 1, item.label);
            }
            else
            {
                writer.integer(first + 1, item.id);
                if (width == 3)
                    writer.integer(first + 2, item.component);
            }
            ++slot;
        }
        line += FIELDS_PER_LINE;
    }
}

std::optional<CardProblem> checkSuperelementGroups(const std::vector<SuperelementGroup>& groups,
                                                   std::span<const SuperelementGroupKeyword> keywords)
{
    for (const SuperelementGroup& group : groups)
    {
        const SuperelementGroupKeyword* keyword = findSuperelementGroupKeyword(keywords, group.keyword);
        if (keyword == nullptr)
            return CardProblem{"card.bad-value",
                               "\"" + CardProblem::cardNamed(group.keyword) + "\" is not a keyword of this card",
                               "keyword"};
        if (keyword->content == SuperelementGroupContent::Text)
        {
            if (group.lines.empty())
                return CardProblem{"card.missing-field",
                                   "the group " + CardProblem::cardNamed(group.keyword) + " has no text", "keyword"};
            for (const std::string& text : group.lines)
            {
                if (const auto problem = superelementLabelProblem(text, keyword->valueName))
                    return problem;
            }
            continue;
        }
        if (group.items.empty())
            return CardProblem{"card.missing-field",
                               "the group " + CardProblem::cardNamed(group.keyword) + " has no item", "keyword"};
        for (const SuperelementGroupItem& item : group.items)
        {
            if (const auto problem = checkSuperelementId(item.superelementId, "superelementId", true))
                return problem;
            if (keyword->content == SuperelementGroupContent::Labels)
            {
                if (const auto problem = checkNameWord(item.label, keyword->valueName))
                    return problem;
                continue;
            }
            if (const auto problem = checkRequiredId(item.id, keyword->valueName))
                return problem;
            // The component of a DNODE is one translation of the grid point: 1, 2 or 3.
            if (keyword->content == SuperelementGroupContent::Nodes && (item.component < 1 || item.component > 3))
                return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2 or 3",
                                   "nodeComponents"};
        }
    }
    return std::nullopt;
}

void visitSuperelementGroups(ReferenceVisitor& visitor, std::vector<SuperelementGroup>& groups,
                             std::span<const SuperelementGroupKeyword> keywords)
{
    for (SuperelementGroup& group : groups)
    {
        const SuperelementGroupKeyword* keyword = findSuperelementGroupKeyword(keywords, group.keyword);
        if (keyword == nullptr || keyword->content == SuperelementGroupContent::Text)
            continue;
        for (SuperelementGroupItem& item : group.items)
        {
            visitSuperelement(visitor, "superelementId", item.superelementId);
            if (keyword->content == SuperelementGroupContent::Labels)
                continue;
            SuperelementScope scope(visitor, item.superelementId);
            visitId(scope, keyword->valueName, keyword->target, item.id);
        }
    }
}

} // namespace oor::model
