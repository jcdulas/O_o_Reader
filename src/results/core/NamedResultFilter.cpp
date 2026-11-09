#include "oor/results/core/NamedResultFilter.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <string>

namespace oor::results
{

bool namedKept(const ResultFilter& filter, std::string_view kind, std::string_view tableName, std::int32_t subcase,
               std::int32_t designCycle)
{
    if (filter.subcases &&
        std::find(filter.subcases->begin(), filter.subcases->end(), subcase) == filter.subcases->end())
        return false;
    if (filter.designCycles &&
        std::find(filter.designCycles->begin(), filter.designCycles->end(), designCycle) == filter.designCycles->end())
        return false;
    if (filter.variants && std::none_of(filter.variants->begin(), filter.variants->end(), [](const std::string& name)
                                        { return name.empty() || base::equalsIgnoreCase(name, "plain"); }))
        return false;
    if (filter.tables)
        return std::any_of(filter.tables->begin(), filter.tables->end(), [&](const std::string& name)
                           { return base::equalsIgnoreCase(name, kind) || base::equalsIgnoreCase(name, tableName); });
    return true;
}

} // namespace oor::results
