// The arguments of a named query read from JSON (see argumentsFromJson in Queries.hpp).

#include "oor/api/Queries.hpp"
#include "oor/base/errors/Errors.hpp"

#include <cmath>

namespace oor::api
{

namespace
{

using base::JsonValue;
using query::ParameterKind;
using query::QueryValue;

std::int64_t wholeNumber(const JsonValue& value, const std::string& what)
{
    const double number = value.asNumber(what);
    if (std::floor(number) != number)
        throw base::UsageError(what + " is a whole number, not " + std::to_string(number));
    return static_cast<std::int64_t>(number);
}

// One ID or an array of IDs.
std::vector<std::int64_t> idsOf(const JsonValue& value, const std::string& what)
{
    if (value.isNumber())
        return {wholeNumber(value, what)};
    std::vector<std::int64_t> ids;
    for (const JsonValue& item : value.asArray(what))
        ids.push_back(wholeNumber(item, what + " (an ID)"));
    return ids;
}

std::vector<double> numbersOf(const JsonValue& value, const std::string& what)
{
    if (value.isNumber())
        return {value.asNumber(what)};
    std::vector<double> numbers;
    for (const JsonValue& item : value.asArray(what))
        numbers.push_back(item.asNumber(what + " (a number)"));
    return numbers;
}

std::vector<std::string> textsOf(const JsonValue& value, const std::string& what)
{
    if (value.isText())
        return {value.asText(what)};
    std::vector<std::string> texts;
    for (const JsonValue& item : value.asArray(what))
        texts.push_back(item.asText(what + " (a text)"));
    return texts;
}

std::vector<std::pair<std::string, double>> namedNumbersOf(const JsonValue& value, const std::string& what)
{
    std::vector<std::pair<std::string, double>> entries;
    if (value.isNumber())
    {
        entries.emplace_back("all", value.asNumber(what));
        return entries;
    }
    for (const auto& [key, item] : value.asObject(what))
    {
        if (item.isArray())
        {
            // a list of numbers under one key: "key:0", "key:1", ...
            std::size_t position = 0;
            for (const JsonValue& part : item.asArray(what))
                entries.emplace_back(key + ":" + std::to_string(position++), part.asNumber(what));
        }
        else
        {
            entries.emplace_back(key, item.asNumber(what + "." + key));
        }
    }
    return entries;
}

QueryValue convert(const JsonValue& value, ParameterKind kind, const std::string& name)
{
    const std::string what = "the argument '" + name + "'";
    if (value.isNull())
        return std::monostate{};
    switch (kind)
    {
    case ParameterKind::Bool:
        return value.asBool(what);
    case ParameterKind::Integer:
        return wholeNumber(value, what);
    case ParameterKind::Real:
        return value.asNumber(what);
    case ParameterKind::Text:
        return value.asText(what);
    case ParameterKind::IdList:
        return idsOf(value, what);
    case ParameterKind::IdListOrText:
        if (value.isText())
            return value.asText(what);
        return idsOf(value, what);
    case ParameterKind::TextList:
        return textsOf(value, what);
    case ParameterKind::Point:
    {
        std::vector<double> point = numbersOf(value, what);
        if (point.size() != 3)
            throw base::UsageError(what + " is a point: three numbers");
        return point;
    }
    case ParameterKind::Reals:
        return numbersOf(value, what);
    case ParameterKind::RealsOrText:
        if (value.isText())
            return value.asText(what);
        return numbersOf(value, what);
    case ParameterKind::Steps:
    {
        query::StepSelection selection;
        if (value.isObject())
        {
            selection.isSlice = true;
            if (const JsonValue* start = value.member("start"); start != nullptr && !start->isNull())
                selection.start = wholeNumber(*start, what + ".start");
            if (const JsonValue* stop = value.member("stop"); stop != nullptr && !stop->isNull())
                selection.stop = wholeNumber(*stop, what + ".stop");
            if (const JsonValue* step = value.member("step"); step != nullptr && !step->isNull())
                selection.step = wholeNumber(*step, what + ".step");
            return selection;
        }
        selection.values = numbersOf(value, what);
        return selection;
    }
    case ParameterKind::Pairs:
    {
        std::vector<std::int64_t> flat;
        for (const JsonValue& pair : value.asArray(what))
        {
            const std::vector<std::int64_t> ids = idsOf(pair, what);
            if (ids.size() != 2)
                throw base::UsageError(what + " takes pairs of IDs: [n1, n2]");
            flat.push_back(ids[0]);
            flat.push_back(ids[1]);
        }
        return flat;
    }
    case ParameterKind::Thresholds:
    case ParameterKind::Values:
        return namedNumbersOf(value, what);
    case ParameterKind::IdSets:
    {
        std::vector<query::NamedIdSet> sets;
        if (value.isObject())
        {
            for (const auto& [key, ids] : value.asObject(what))
            {
                query::NamedIdSet set;
                set.name = key;
                set.ids = idsOf(ids, what + "." + key);
                sets.push_back(std::move(set));
            }
            return sets;
        }
        for (const std::int64_t id : idsOf(value, what))
        {
            query::NamedIdSet set;
            set.name = std::to_string(id);
            set.setId = id;
            sets.push_back(std::move(set));
        }
        return sets;
    }
    case ParameterKind::Groups:
    {
        std::vector<query::NamedIdSet> groups;
        for (const auto& [key, group] : value.asObject(what))
        {
            const auto& members = group.asObject(what + "." + key);
            if (members.size() != 1 || (members.front().first != "components" && members.front().first != "elements"))
                throw base::UsageError(what + ": the group '" + key +
                                       "' is {\"components\": [IDs]} or {\"elements\": [IDs]}");
            query::NamedIdSet set;
            set.name = key;
            set.kind = members.front().first;
            set.ids = idsOf(members.front().second, what + "." + key);
            groups.push_back(std::move(set));
        }
        return groups;
    }
    case ParameterKind::Combinations:
    {
        std::vector<query::NamedCombination> combinations;
        for (const auto& [key, factors] : value.asObject(what))
        {
            query::NamedCombination combination;
            combination.name = key;
            for (const auto& [subcase, factor] : factors.asObject(what + "." + key))
            {
                char* end = nullptr;
                const long number = std::strtol(subcase.c_str(), &end, 10);
                if (subcase.empty() || *end != '\0')
                    throw base::UsageError(what + ": the combination '" + key +
                                           "' takes subcase numbers as keys, not '" + subcase + "'");
                combination.factors.emplace_back(static_cast<std::int32_t>(number),
                                                 factor.asNumber(what + "." + key + "." + subcase));
            }
            if (combination.factors.empty())
                throw base::UsageError(what + ": the combination '" + key + "' has no subcase");
            combinations.push_back(std::move(combination));
        }
        return combinations;
    }
    case ParameterKind::Cuts:
    {
        std::vector<query::NamedCut> cuts;
        for (const auto& [key, cut] : value.asObject(what))
        {
            query::NamedCut made;
            made.name = key;
            const std::string where = what + "." + key;
            if (const JsonValue* plane = cut.member("plane"))
                made.plane = numbersOf(*plane, where + ".plane");
            if (const JsonValue* nodes = cut.member("nodes"))
                made.nodes = idsOf(*nodes, where + ".nodes");
            if (const JsonValue* side = cut.member("sideElements"))
                made.sideElements = idsOf(*side, where + ".sideElements");
            if (const JsonValue* point = cut.member("point"))
                made.point = numbersOf(*point, where + ".point");
            if (const JsonValue* frame = cut.member("frame"))
                made.frame = numbersOf(*frame, where + ".frame");
            cuts.push_back(std::move(made));
        }
        return cuts;
    }
    case ParameterKind::Model:
        throw base::UsageError(what + " takes another model, which cannot be given as JSON");
    }
    return std::monostate{};
}

} // namespace

query::QueryArguments argumentsFromJson(const std::string& queryName, const std::string& json)
{
    query::QueryArguments arguments;
    if (json.empty())
        return arguments;
    const JsonValue document = JsonValue::parse(json);
    if (document.isNull())
        return arguments;
    for (const auto& [name, value] : document.asObject("the arguments of a query"))
        arguments.set(name, convert(value, parameterKind(queryName, name), name));
    return arguments;
}

} // namespace oor::api
