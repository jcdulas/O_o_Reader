#include "oor/results/core/NamedResultBuilder.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/results/core/NamedResultKinds.hpp"

#include <algorithm>
#include <cstring>
#include <optional>

namespace oor::results
{

namespace
{

// The extents a shape text asks for: "(n, 6, 6)" is the row count then 6 and 6; a number is fixed, `n` is the row
// count, any other word is free.
struct ShapeRule
{
    bool scalar = true;                            // the shape "" (a number alone)
    std::vector<std::optional<std::size_t>> fixed; // a fixed extent, or none for `n` and the free words
    bool firstIsRows = false;
};

ShapeRule parseShape(std::string_view text)
{
    ShapeRule rule;
    if (text.empty())
        return rule;
    rule.scalar = false;
    std::string_view inner = text;
    inner.remove_prefix(1);
    inner.remove_suffix(1);
    std::size_t at = 0;
    bool first = true;
    while (at <= inner.size())
    {
        std::size_t end = inner.find(',', at);
        if (end == std::string_view::npos)
            end = inner.size();
        std::string_view token = inner.substr(at, end - at);
        while (!token.empty() && token.front() == ' ')
            token.remove_prefix(1);
        if (!token.empty() && token.front() >= '0' && token.front() <= '9')
            rule.fixed.emplace_back(static_cast<std::size_t>(std::stoul(std::string(token))));
        else
        {
            rule.fixed.emplace_back(std::nullopt);
            if (first && token == "n")
                rule.firstIsRows = true;
        }
        first = false;
        at = end + 1;
    }
    return rule;
}

[[nodiscard]] bool isFloatArray(ArrayType type) noexcept
{
    return type == ArrayType::Float32 || type == ArrayType::Float64;
}

[[nodiscard]] bool typeFits(std::string_view wanted, ArrayType type) noexcept
{
    if (wanted == "int32")
        return type == ArrayType::Int32;
    if (wanted == "int64")
        return type == ArrayType::Int64;
    if (wanted == "float")
        return isFloatArray(type);
    if (wanted == "float or complex")
        return isFloatArray(type) || type == ArrayType::Complex64 || type == ArrayType::Complex128;
    return false;
}

[[nodiscard]] std::size_t countOf(const std::vector<std::size_t>& shape) noexcept
{
    std::size_t count = 1;
    for (const std::size_t extent : shape)
        count *= extent;
    return count;
}

// The labels of a matrix: all four or none, as long as the rows and the columns, and the terms inside the shape.
void checkMatrixLabels(std::span<const NamedArrayInput> arrays, std::vector<std::string>& problems)
{
    const auto find = [&](std::string_view name) -> const NamedArrayInput*
    {
        const auto found = std::find_if(arrays.begin(), arrays.end(),
                                        [&](const NamedArrayInput& array) { return array.name == name; });
        return found == arrays.end() ? nullptr : &*found;
    };
    const NamedArrayInput* shape = find("shape");
    std::optional<std::int64_t> rows;
    std::optional<std::int64_t> columns;
    if (shape != nullptr && shape->type == ArrayType::Int64 && shape->bytes.size() == 2 * sizeof(std::int64_t))
    {
        std::int64_t values[2];
        std::memcpy(values, shape->bytes.data(), sizeof(values));
        rows = values[0];
        columns = values[1];
        const NamedArrayInput* row = find("row");
        const NamedArrayInput* column = find("column");
        if (row != nullptr && column != nullptr && row->type == ArrayType::Int32 && column->type == ArrayType::Int32)
        {
            const auto outside = [](const NamedArrayInput& array, std::int64_t limit)
            {
                const std::size_t count = array.bytes.size() / sizeof(std::int32_t);
                for (std::size_t index = 0; index < count; ++index)
                {
                    std::int32_t value = 0;
                    std::memcpy(&value, array.bytes.data() + index * sizeof(value), sizeof(value));
                    if (value < 0 || value >= limit)
                        return true;
                }
                return false;
            };
            if (outside(*row, *rows))
                problems.push_back("a row index of the terms is outside the shape");
            if (outside(*column, *columns))
                problems.push_back("a column index of the terms is outside the shape");
        }
    }
    const char* const labelNames[] = {"rowIds", "rowComponents", "columnIds", "columnComponents"};
    std::size_t given = 0;
    for (const char* name : labelNames)
        given += find(name) != nullptr ? 1 : 0;
    if (given != 0 && given != 4)
        problems.push_back(
            "the labels of a matrix are given all four (rowIds, rowComponents, columnIds, "
            "columnComponents) or not at all");
    for (std::size_t index = 0; index < 4; ++index)
    {
        const NamedArrayInput* array = find(labelNames[index]);
        const std::optional<std::int64_t>& wanted = index < 2 ? rows : columns;
        if (array != nullptr && wanted && array->shape.size() == 1 &&
            static_cast<std::int64_t>(array->shape[0]) != *wanted)
            problems.push_back(std::string("the array '") + labelNames[index] + "' has " +
                               std::to_string(array->shape[0]) + " entries where the matrix has " +
                               std::to_string(*wanted) + (index < 2 ? " rows" : " columns"));
    }
    // The harmonic numbers of a DMIAX matrix go with the labels and have the same lengths.
    const char* const harmonicNames[] = {"rowHarmonics", "columnHarmonics"};
    for (std::size_t index = 0; index < 2; ++index)
    {
        const NamedArrayInput* array = find(harmonicNames[index]);
        if (array == nullptr)
            continue;
        if (given == 0)
            problems.push_back(std::string("the array '") + harmonicNames[index] +
                               "' needs the labels rowIds, rowComponents, columnIds and columnComponents");
        const std::optional<std::int64_t>& wanted = index == 0 ? rows : columns;
        if (wanted && array->shape.size() == 1 && static_cast<std::int64_t>(array->shape[0]) != *wanted)
            problems.push_back(std::string("the array '") + harmonicNames[index] + "' has " +
                               std::to_string(array->shape[0]) + " entries where the matrix has " +
                               std::to_string(*wanted) + (index == 0 ? " rows" : " columns"));
    }
}

} // namespace

std::vector<std::string> checkNamedResult(std::string_view kind, std::span<const NamedArrayInput> arrays,
                                          std::span<const NamedTextInput> texts)
{
    std::vector<std::string> problems;
    const NamedKindInfo* info = findNamedKind(kind);
    if (info == nullptr)
    {
        problems.push_back("'" + std::string(kind) + "' is not a kind of named result");
        return problems;
    }
    std::optional<std::size_t> rows;
    std::optional<ArrayType> floatType;
    const auto noteRows = [&](const std::string& what, std::size_t count)
    {
        if (!rows)
            rows = count;
        else if (*rows != count)
            problems.push_back(what + " has " + std::to_string(count) + " rows where the other arrays have " +
                               std::to_string(*rows));
    };
    for (const NamedArrayInfo& expected : info->arrays)
    {
        const std::string name(expected.name);
        if (expected.text)
        {
            const auto found =
                std::find_if(texts.begin(), texts.end(), [&](const NamedTextInput& text) { return text.name == name; });
            if (found == texts.end())
            {
                if (!expected.optional)
                    problems.push_back("the text '" + name + "' of " + std::string(kind) + " is missing");
            }
            else
            {
                noteRows("the text '" + name + "'", found->values.size());
            }
            continue;
        }
        const auto found = std::find_if(arrays.begin(), arrays.end(),
                                        [&](const NamedArrayInput& array) { return array.name == name; });
        if (found == arrays.end())
        {
            if (!expected.optional)
                problems.push_back("the array '" + name + "' of " + std::string(kind) + " is missing");
            continue;
        }
        if (!typeFits(expected.type, found->type))
            problems.push_back("the array '" + name + "' is " + std::string(arrayTypeName(found->type)) + " where " +
                               std::string(kind) + " has " + std::string(expected.type));
        if (expected.type == "float" && isFloatArray(found->type))
        {
            if (floatType && *floatType != found->type)
                problems.push_back("the numbers of " + std::string(kind) +
                                   " are all single precision or all double precision, not both");
            floatType = found->type;
        }
        const ShapeRule rule = parseShape(expected.shape);
        if (rule.scalar ? !found->shape.empty() : found->shape.size() != rule.fixed.size())
        {
            problems.push_back("the array '" + name + "' has " + std::to_string(found->shape.size()) +
                               " dimensions where the shape of " + std::string(kind) + " is " +
                               std::string(expected.shape));
            continue;
        }
        for (std::size_t axis = 0; axis < rule.fixed.size(); ++axis)
            if (rule.fixed[axis] && found->shape[axis] != *rule.fixed[axis])
                problems.push_back("the array '" + name + "' has " + std::to_string(found->shape[axis]) +
                                   " on its axis " + std::to_string(axis) + " where " + std::string(kind) + " has " +
                                   std::to_string(*rule.fixed[axis]));
        if (found->bytes.size() != countOf(found->shape) * arrayTypeSize(found->type))
            problems.push_back("the array '" + name + "' holds " + std::to_string(found->bytes.size()) +
                               " bytes where its shape needs " +
                               std::to_string(countOf(found->shape) * arrayTypeSize(found->type)));
        if (rule.firstIsRows)
            noteRows("the array '" + name + "'", found->shape.front());
    }
    if (kind == "matrix")
        checkMatrixLabels(arrays, problems);
    for (const NamedArrayInput& array : arrays)
    {
        const bool known = std::any_of(info->arrays.begin(), info->arrays.end(), [&](const NamedArrayInfo& expected)
                                       { return !expected.text && expected.name == array.name; });
        if (!known)
            problems.push_back("'" + array.name + "' is not an array of " + std::string(kind));
    }
    for (const NamedTextInput& text : texts)
    {
        const bool known = std::any_of(info->arrays.begin(), info->arrays.end(), [&](const NamedArrayInfo& expected)
                                       { return expected.text && expected.name == text.name; });
        if (!known)
            problems.push_back("'" + text.name + "' is not a text of " + std::string(kind));
    }
    return problems;
}

std::shared_ptr<NamedResult> makeNamedResult(std::string_view kind, const NamedResultOrigin& origin,
                                             std::span<const NamedArrayInput> arrays,
                                             std::span<const NamedTextInput> texts)
{
    const std::vector<std::string> problems = checkNamedResult(kind, arrays, texts);
    if (!problems.empty())
    {
        std::string message = "cannot make a " + std::string(kind) + " result:";
        for (const std::string& problem : problems)
            message += "\n  " + problem;
        throw base::UsageError(message);
    }
    const NamedKindInfo* info = findNamedKind(kind);
    auto data = std::make_shared<NamedResultData>();
    // the arrays in the order the kind lists them
    for (const NamedArrayInfo& expected : info->arrays)
    {
        if (expected.text)
            continue;
        const auto found = std::find_if(arrays.begin(), arrays.end(),
                                        [&](const NamedArrayInput& array) { return array.name == expected.name; });
        if (found == arrays.end())
            continue; // an optional array that was not given
        ResultArray array;
        array.name = found->name;
        array.description = std::string(expected.description);
        array.type = found->type;
        array.shape = found->shape;
        array.allocate();
        if (!found->bytes.empty())
            std::memcpy(array.mutableBytes(), found->bytes.data(), found->bytes.size());
        data->arrays.push_back(std::move(array));
    }
    for (const NamedArrayInfo& expected : info->arrays)
    {
        if (!expected.text)
            continue;
        const auto found = std::find_if(texts.begin(), texts.end(),
                                        [&](const NamedTextInput& text) { return text.name == expected.name; });
        data->texts.emplace_back(found->name, found->values);
    }
    NamedResultMeta meta;
    meta.kind = std::string(kind);
    meta.name = origin.name.empty() ? std::string(kind) : origin.name;
    meta.designCycle = origin.designCycle;
    meta.subcase = origin.subcase;
    meta.analysisCode = origin.analysisCode;
    meta.status = ResultStatus::NotValidated;
    const bool single = std::any_of(data->arrays.begin(), data->arrays.end(),
                                    [](const ResultArray& array) { return array.type == ArrayType::Float32; });
    meta.wordSize = single ? 4 : 8;
    // a result that says where it comes from (a matrix assembled from the cards of a model) keeps that
    const bool saysOrigin = std::any_of(origin.attributes.begin(), origin.attributes.end(),
                                        [](const auto& item) { return item.first == "origin"; });
    if (!saysOrigin)
        meta.attributes.emplace_back("origin", "memory");
    for (const auto& item : origin.attributes)
        meta.attributes.push_back(item);
    return std::make_shared<NamedResult>(std::move(meta), std::shared_ptr<const NamedResultData>(std::move(data)));
}

} // namespace oor::results
