#include "oor/query/named/common/CaseRequests.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace oor::query
{

std::vector<OutputRequest> effectiveRequests(const model::CaseControl& caseControl, const model::Subcase& subcase)
{
    return effectiveRequests(caseControl.lineage(subcase.key));
}

std::vector<OutputRequest> effectiveRequests(std::span<const model::Subcase* const> lineage)
{
    if (lineage.empty())
        return {};
    std::vector<std::string> names;
    for (const model::Subcase* section : lineage)
    {
        for (const model::CaseEntry& entry : section->entries)
        {
            if (entry.kind == model::EntryKind::Output &&
                std::find(names.begin(), names.end(), entry.name) == names.end())
                names.push_back(entry.name);
        }
    }
    std::vector<OutputRequest> requests;
    for (const std::string& name : names)
    {
        for (const model::Subcase* section : lineage)
        {
            const std::vector<const model::CaseEntry*> own = model::CaseControl::ownOutputs(*section, name);
            if (own.empty())
                continue;
            for (const model::CaseEntry* entry : own)
                requests.push_back(OutputRequest{entry, section != lineage.front()});
            break;
        }
    }
    return requests;
}

std::string requestText(const model::CaseEntry& entry)
{
    std::string text = entry.name;
    if (!entry.options.empty())
    {
        text += "(";
        for (std::size_t index = 0; index < entry.options.size(); ++index)
        {
            text += (index == 0 ? "" : ",") + entry.options[index];
        }
        text += ")";
    }
    if (!entry.value.isNone())
        text += " = " + entry.value.format();
    return text;
}

std::vector<std::string> fullDescribers(const model::CaseEntry& entry)
{
    return entry.options;
}

} // namespace oor::query
