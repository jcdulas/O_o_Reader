#include "oor/query/named/QueryValue.hpp"

#include "oor/query/QueryErrors.hpp"

namespace oor::query
{

std::string_view parameterKindName(ParameterKind kind) noexcept
{
    switch (kind)
    {
    case ParameterKind::Bool:
        return "bool";
    case ParameterKind::Integer:
        return "int";
    case ParameterKind::Real:
        return "float";
    case ParameterKind::Text:
        return "str";
    case ParameterKind::IdList:
        return "ids";
    case ParameterKind::IdListOrText:
        return "ids or str";
    case ParameterKind::TextList:
        return "names";
    case ParameterKind::Point:
        return "point";
    case ParameterKind::IdSets:
        return "sets";
    case ParameterKind::Thresholds:
        return "thresholds";
    case ParameterKind::Model:
        return "model";
    case ParameterKind::Steps:
        return "steps";
    case ParameterKind::Pairs:
        return "pairs";
    case ParameterKind::Reals:
        return "numbers";
    case ParameterKind::Values:
        return "values";
    case ParameterKind::RealsOrText:
        return "numbers or str";
    case ParameterKind::Groups:
        return "groups";
    case ParameterKind::Combinations:
        return "combinations";
    case ParameterKind::Cuts:
        return "cuts";
    }
    return "?";
}

const QueryValue* QueryArguments::find(std::string_view name) const
{
    const auto found = m_values.find(name);
    if (found == m_values.end() || std::holds_alternative<std::monostate>(found->second))
        return nullptr;
    return &found->second;
}

bool QueryArguments::has(std::string_view name) const
{
    return find(name) != nullptr;
}

bool QueryArguments::isText(std::string_view name) const
{
    const QueryValue* value = find(name);
    return value != nullptr && std::holds_alternative<std::string>(*value);
}

namespace
{

[[noreturn]] void wrongKind(std::string_view name, const char* expected)
{
    throw QueryTypeError("the argument '" + std::string(name) + "' must be " + expected);
}

} // namespace

bool QueryArguments::boolOr(std::string_view name, bool fallback) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return fallback;
    if (const bool* flag = std::get_if<bool>(value))
        return *flag;
    wrongKind(name, "True or False");
}

std::int64_t QueryArguments::integerOr(std::string_view name, std::int64_t fallback) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return fallback;
    if (const auto* number = std::get_if<std::int64_t>(value))
        return *number;
    wrongKind(name, "a whole number");
}

double QueryArguments::realOr(std::string_view name, double fallback) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return fallback;
    if (const auto* number = std::get_if<double>(value))
        return *number;
    if (const auto* whole = std::get_if<std::int64_t>(value))
        return static_cast<double>(*whole);
    wrongKind(name, "a number");
}

std::string QueryArguments::textOr(std::string_view name, std::string fallback) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return fallback;
    if (const auto* text = std::get_if<std::string>(value))
        return *text;
    wrongKind(name, "a text");
}

std::optional<std::string> QueryArguments::text(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return std::nullopt;
    if (const auto* text = std::get_if<std::string>(value))
        return *text;
    wrongKind(name, "a text");
}

std::string QueryArguments::formLabel(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return {};
    if (const auto* text = std::get_if<std::string>(value))
        return *text;
    if (const bool* flag = std::get_if<bool>(value))
        return *flag ? "True" : "False";
    return {};
}

std::optional<std::vector<std::int64_t>> QueryArguments::ids(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return std::nullopt;
    if (const auto* list = std::get_if<std::vector<std::int64_t>>(value))
        return *list;
    if (const auto* single = std::get_if<std::int64_t>(value))
        return std::vector<std::int64_t>{*single};
    wrongKind(name, "a list of IDs");
}

std::vector<std::string> QueryArguments::texts(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return {};
    if (const auto* list = std::get_if<std::vector<std::string>>(value))
        return *list;
    if (const auto* single = std::get_if<std::string>(value))
        return {*single};
    wrongKind(name, "a text or a list of texts");
}

std::optional<std::array<double, 3>> QueryArguments::point(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return std::nullopt;
    if (const auto* list = std::get_if<std::vector<double>>(value))
    {
        if (list->size() != 3)
            wrongKind(name, "three numbers");
        return std::array<double, 3>{(*list)[0], (*list)[1], (*list)[2]};
    }
    wrongKind(name, "three numbers");
}

std::vector<NamedIdSet> QueryArguments::idSets(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return {};
    if (const auto* sets = std::get_if<std::vector<NamedIdSet>>(value))
        return *sets;
    wrongKind(name, "a dict {name: IDs} or a list of SET1 IDs");
}

std::vector<NamedIdSet> QueryArguments::groups(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return {};
    if (const auto* sets = std::get_if<std::vector<NamedIdSet>>(value))
        return *sets;
    wrongKind(name, "a dict {name: {\"components\": IDs}} or {name: {\"elements\": IDs}}");
}

std::vector<std::pair<std::string, double>> QueryArguments::thresholds(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return {};
    if (const auto* list = std::get_if<std::vector<std::pair<std::string, double>>>(value))
        return *list;
    wrongKind(name, "a dict {metric: limit}");
}

std::shared_ptr<model::Model> QueryArguments::model(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return nullptr;
    if (const auto* other = std::get_if<std::shared_ptr<model::Model>>(value))
        return *other;
    wrongKind(name, "a model");
}

std::optional<StepSelection> QueryArguments::steps(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return std::nullopt;
    if (const auto* selection = std::get_if<StepSelection>(value))
        return *selection;
    if (const auto* list = std::get_if<std::vector<double>>(value))
    {
        StepSelection selection;
        selection.values = *list;
        return selection;
    }
    if (const auto* number = std::get_if<double>(value))
    {
        StepSelection selection;
        selection.values = {*number};
        return selection;
    }
    if (const auto* whole = std::get_if<std::int64_t>(value))
    {
        StepSelection selection;
        selection.values = {static_cast<double>(*whole)};
        return selection;
    }
    wrongKind(name, "a list of step values or a slice");
}

std::vector<std::pair<std::int64_t, std::int64_t>> QueryArguments::pairs(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return {};
    const auto* list = std::get_if<std::vector<std::int64_t>>(value);
    if (list == nullptr || list->size() % 2 != 0)
        wrongKind(name, "a list of pairs of IDs");
    std::vector<std::pair<std::int64_t, std::int64_t>> result;
    for (std::size_t index = 0; index + 1 < list->size(); index += 2)
        result.emplace_back((*list)[index], (*list)[index + 1]);
    return result;
}

std::vector<double> QueryArguments::reals(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return {};
    if (const auto* list = std::get_if<std::vector<double>>(value))
        return *list;
    if (const auto* number = std::get_if<double>(value))
        return {*number};
    wrongKind(name, "a list of numbers");
}

std::vector<std::pair<std::string, double>> QueryArguments::values(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return {};
    if (const auto* list = std::get_if<std::vector<std::pair<std::string, double>>>(value))
        return *list;
    wrongKind(name, "a dict {key: number}");
}

std::vector<NamedCombination> QueryArguments::combinations(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return {};
    if (const auto* list = std::get_if<std::vector<NamedCombination>>(value))
        return *list;
    wrongKind(name, "a dict {name: {subcase: factor}}");
}

std::vector<NamedCut> QueryArguments::cuts(std::string_view name) const
{
    const QueryValue* value = find(name);
    if (value == nullptr)
        return {};
    if (const auto* list = std::get_if<std::vector<NamedCut>>(value))
        return *list;
    wrongKind(name, "a dict {name: {\"plane\": (point, normal)}} or {name: {\"nodes\": IDs, \"sideElements\": IDs}}");
}

} // namespace oor::query
