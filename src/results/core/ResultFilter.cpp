#include "oor/results/core/ResultFilter.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace oor::results
{

namespace
{

// The ID range up to which a bitmap answers "is this ID wanted" (16 MB); a wider range uses a binary search.
constexpr std::int64_t BITMAP_LIMIT = std::int64_t{1} << 27;

template <typename T>
void appendList(std::string& text, const char* label, const std::optional<std::vector<T>>& values)
{
    if (!values)
        return;
    if (!text.empty())
        text += ", ";
    text += label;
    text += " [";
    constexpr std::size_t SHOWN = 8;
    for (std::size_t index = 0; index < values->size() && index < SHOWN; ++index)
    {
        if (index > 0)
            text += ", ";
        if constexpr (std::is_same_v<T, std::string>)
            text += (*values)[index];
        else
            text += std::to_string((*values)[index]);
    }
    if (values->size() > SHOWN)
        text += ", ... " + std::to_string(values->size()) + " in all";
    text += "]";
}

// Python's slice.indices(): the positions a slice keeps of a sequence of `length` items.
std::vector<std::int64_t> slicePositions(const IndexSlice& slice, std::int64_t length)
{
    std::vector<std::int64_t> positions;
    const std::int64_t step = slice.step == 0 ? 1 : slice.step;
    const auto clamp =
        [&](std::optional<std::int64_t> value, std::int64_t lowest, std::int64_t highest, std::int64_t fallback)
    {
        if (!value)
            return fallback;
        std::int64_t number = *value;
        if (number < 0)
            number += length;
        return std::clamp(number, lowest, highest);
    };
    if (step > 0)
    {
        const std::int64_t first = clamp(slice.start, 0, length, 0);
        const std::int64_t last = clamp(slice.stop, 0, length, length);
        for (std::int64_t position = first; position < last; position += step)
            positions.push_back(position);
    }
    else
    {
        const std::int64_t first = clamp(slice.start, -1, length - 1, length - 1);
        const std::int64_t last = clamp(slice.stop, -1, length - 1, -1);
        for (std::int64_t position = first; position > last; position += step)
            positions.push_back(position);
    }
    return positions;
}

} // namespace

bool sameStepValue(double left, double right) noexcept
{
    if (left == right)
        return true;
    if (std::isnan(left) || std::isnan(right))
        return false;
    return std::fabs(left - right) <= 1.0e-6 * std::max(std::fabs(left), std::fabs(right));
}

std::string ResultFilter::describe() const
{
    std::string text;
    appendList(text, "subcases", subcases);
    appendList(text, "tables", tables);
    appendList(text, "ids", ids);
    appendList(text, "element ids", elementIds);
    appendList(text, "layers", layers);
    appendList(text, "design cycles", designCycles);
    appendList(text, "variants", variants);
    appendList(text, "components", components);
    appendList(text, "steps", steps);
    if (stepRange)
    {
        if (!text.empty())
            text += ", ";
        text += "step range [" + std::to_string(stepRange->lower) + ", " + std::to_string(stepRange->upper) + "]";
        if (stepRange->kind)
            text += " of " + std::string(stepKindName(*stepRange->kind));
    }
    appendList(text, "step indices", stepIndices);
    if (stepSlice)
    {
        if (!text.empty())
            text += ", ";
        text += "a slice of the steps";
    }
    return text.empty() ? "nothing" : text;
}

std::vector<std::uint32_t> selectStepPositions(const ResultFilter& filter, std::span<const double> steps, StepKind kind)
{
    const auto count = static_cast<std::int64_t>(steps.size());
    std::vector<bool> keep(steps.size(), true);
    if (filter.steps)
    {
        for (std::size_t position = 0; position < steps.size(); ++position)
        {
            const double step = steps[position];
            keep[position] = keep[position] && std::any_of(filter.steps->begin(), filter.steps->end(),
                                                           [&](double wanted) { return sameStepValue(wanted, step); });
        }
    }
    if (filter.stepRange)
    {
        const StepRange& range = *filter.stepRange;
        const bool kindMatches = !range.kind || *range.kind == kind;
        const double slack = 1.0e-6 * std::max(std::fabs(range.lower), std::fabs(range.upper));
        for (std::size_t position = 0; position < steps.size(); ++position)
        {
            const double step = steps[position];
            keep[position] = keep[position] && kindMatches && !std::isnan(step) && step >= range.lower - slack &&
                             step <= range.upper + slack;
        }
    }
    if (filter.stepIndices)
    {
        std::vector<bool> wanted(steps.size(), false);
        for (std::int64_t index : *filter.stepIndices)
        {
            if (index < 0)
                index += count;
            if (index >= 0 && index < count)
                wanted[static_cast<std::size_t>(index)] = true;
        }
        for (std::size_t position = 0; position < steps.size(); ++position)
            keep[position] = keep[position] && wanted[position];
    }
    if (filter.stepSlice)
    {
        std::vector<bool> wanted(steps.size(), false);
        for (const std::int64_t position : slicePositions(*filter.stepSlice, count))
            wanted[static_cast<std::size_t>(position)] = true;
        for (std::size_t position = 0; position < steps.size(); ++position)
            keep[position] = keep[position] && wanted[position];
    }
    std::vector<std::uint32_t> positions;
    for (std::size_t position = 0; position < keep.size(); ++position)
        if (keep[position])
            positions.push_back(static_cast<std::uint32_t>(position));
    return positions;
}

std::vector<std::uint32_t> selectIdPositions(std::span<const std::int32_t> wanted, std::span<const std::int32_t> ids)
{
    std::vector<std::uint32_t> positions;
    if (wanted.empty() || ids.empty())
        return positions;
    std::vector<std::int32_t> sorted(wanted.begin(), wanted.end());
    // the queries give their IDs sorted already: they are then only checked
    if (!std::is_sorted(sorted.begin(), sorted.end()))
        std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    const std::int64_t lowest = sorted.front();
    const std::int64_t span = static_cast<std::int64_t>(sorted.back()) - lowest + 1;
    // PERFORMANCE: the rows of a table mostly come by increasing ID; they are then merged with the
    // IDs wanted, without a bitmap of their span (a few thousand elements numbered over millions made one of a megabyte
    // for every table of every subcase)
    if (std::is_sorted(ids.begin(), ids.end()))
    {
        std::size_t next = 0;
        for (std::size_t position = 0; position < ids.size(); ++position)
        {
            while (next < sorted.size() && sorted[next] < ids[position])
                ++next;
            if (next == sorted.size())
                break;
            if (sorted[next] == ids[position])
                positions.push_back(static_cast<std::uint32_t>(position));
        }
    }
    else if (span <= BITMAP_LIMIT)
    {
        std::vector<bool> present(static_cast<std::size_t>(span), false);
        for (const std::int32_t id : sorted)
            present[static_cast<std::size_t>(id - lowest)] = true;
        for (std::size_t position = 0; position < ids.size(); ++position)
        {
            const std::int64_t offset = static_cast<std::int64_t>(ids[position]) - lowest;
            if (offset >= 0 && offset < span && present[static_cast<std::size_t>(offset)])
                positions.push_back(static_cast<std::uint32_t>(position));
        }
    }
    else
    {
        for (std::size_t position = 0; position < ids.size(); ++position)
            if (std::binary_search(sorted.begin(), sorted.end(), ids[position]))
                positions.push_back(static_cast<std::uint32_t>(position));
    }
    return positions;
}

std::vector<std::uint32_t> selectComponentPositions(std::span<const std::string> wanted,
                                                    std::span<const std::string> names)
{
    std::vector<std::uint32_t> positions;
    for (const std::string& name : wanted)
    {
        for (std::size_t position = 0; position < names.size(); ++position)
        {
            if (base::equalsIgnoreCase(names[position], name))
            {
                const auto value = static_cast<std::uint32_t>(position);
                if (std::find(positions.begin(), positions.end(), value) == positions.end())
                    positions.push_back(value);
                break;
            }
        }
    }
    return positions;
}

} // namespace oor::results
