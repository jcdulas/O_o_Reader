#include "oor/query/named/common/ContactArguments.hpp"

#include "oor/query/QueryErrors.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

namespace oor::query
{

std::vector<ParameterInfo> contactParameters()
{
    return {parameter("contact", ParameterKind::Text, "\"merge\"",
                      "contact between bodies: \"merge\" (bodies in contact are one component), \"connection\" (a "
                      "connection with its node pairs) or \"ignore\""),
            contactToleranceParameter(),
            parameter(
                "contactRule", ParameterKind::Text, "\"components\"",
                "the node pairs the contact acts through: \"components\" (the components' rule) or \"solver\" (the "
                "pairs the solver ties for a glued pair, contactNodePairs(rule=\"solver\"); contactTolerance plays no "
                "part)"),
            parameter("connections", ParameterKind::IdList, "None",
                      "elements declared connections whatever their type: part of no component, "
                      "the components made of the other elements by the same rule")};
}

ParameterInfo contactToleranceParameter()
{
    return parameter("contactTolerance", ParameterKind::Real, "None",
                     "the distance below which a grid point meets a contact surface when the pair gives none (None: 5 "
                     "% of the median edge length of the secondary surface)");
}

ParameterInfo contactRuleParameter()
{
    return parameter(
        "rule", ParameterKind::Text, "\"components\"",
        "\"components\" (the pairs of the components) or \"solver\" (the pairs the solver ties for a glued pair: its "
        "ERROR, BIAS, search order and the grid points it leaves out)");
}

analysis::ContactRule contactRuleOf(const QueryArguments& arguments, std::string_view query)
{
    const std::string name = arguments.textOr("rule", "components");
    const auto found = analysis::contactRuleOf(name);
    if (!found)
        throw QueryError(std::string(query) + ": rule is \"components\" or \"solver\", not \"" + name + "\"");
    return *found;
}

analysis::ContactOptions contactOptionsOf(const QueryArguments& arguments, std::string_view query)
{
    analysis::ContactOptions options;
    const std::string mode = arguments.textOr("contact", "merge");
    const auto found = analysis::contactModeOf(mode);
    if (!found)
        throw QueryError(std::string(query) + ": contact is \"merge\", \"connection\" or \"ignore\", not \"" + mode +
                         "\"");
    options.mode = *found;
    const std::string rule = arguments.textOr("contactRule", "components");
    const auto foundRule = analysis::contactRuleOf(rule);
    if (!foundRule)
        throw QueryError(std::string(query) + ": contactRule is \"components\" or \"solver\", not \"" + rule + "\"");
    options.rule = *foundRule;
    if (arguments.has("contactTolerance"))
    {
        const double tolerance = arguments.realOr("contactTolerance", 0.0);
        if (!std::isfinite(tolerance) || tolerance <= 0.0)
            throw QueryError(std::string(query) + ": contactTolerance is a positive distance, not " +
                             std::to_string(tolerance));
        options.tolerance = tolerance;
    }
    if (const auto ids = arguments.ids("connections"))
    {
        for (const std::int64_t id : *ids)
        {
            const auto converted = base::toEntityId(id);
            if (!converted || *converted <= 0)
                throw QueryError(std::string(query) + ": connections are element IDs, not " + std::to_string(id));
            options.connections.push_back(*converted);
        }
        std::sort(options.connections.begin(), options.connections.end());
        options.connections.erase(std::unique(options.connections.begin(), options.connections.end()),
                                  options.connections.end());
    }
    return options;
}

std::string contactKey(const analysis::ContactOptions& contact)
{
    std::uint64_t tolerance = 0;
    if (contact.tolerance)
        std::memcpy(&tolerance, &*contact.tolerance, sizeof(tolerance));
    std::string key = std::to_string(static_cast<int>(contact.mode)) + "|" +
                      (contact.tolerance ? std::to_string(tolerance) : "-") + "|" +
                      std::to_string(static_cast<int>(contact.rule));
    if (!contact.connections.empty())
    {
        // the declared connections by their bytes: equal keys exactly for equal lists
        key += "|" + std::to_string(contact.connections.size()) + "|";
        key.append(reinterpret_cast<const char*>(contact.connections.data()),
                   contact.connections.size() * sizeof(base::EntityId));
    }
    return key;
}

} // namespace oor::query
