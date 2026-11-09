#include "oor/results/core/ResultTable.hpp"

#include "oor/base/errors/Errors.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <numeric>
#include <set>
#include <span>

namespace oor::results
{

namespace
{

std::vector<std::uint32_t> allPositions(std::size_t count)
{
    std::vector<std::uint32_t> positions(count);
    std::iota(positions.begin(), positions.end(), 0U);
    return positions;
}

template <typename T>
std::vector<T> gather(std::span<const T> values, const std::vector<std::uint32_t>& positions)
{
    std::vector<T> result;
    if (values.empty())
        return result;
    result.reserve(positions.size());
    for (const std::uint32_t position : positions)
        result.push_back(values[position]);
    return result;
}

// The description of a decoded block: the parts of the meta that the data now tells.
ResultTableMeta metaFromData(ResultTableMeta meta, const ResultData& data)
{
    meta.stepsKnown = true;
    meta.stepCountKnown = true;
    meta.stepCount = data.stepCount;
    meta.steps = data.steps;
    meta.eigenvalues = data.eigenvalues;
    meta.eigenvaluesImag = data.eigenvaluesImag;
    meta.cycles = data.cycles;
    meta.rowCountKnown = true;
    meta.rowCount = data.rowCount;
    meta.components = data.components;
    return meta;
}

} // namespace

std::string_view storedFrameName(StoredFrame frame) noexcept
{
    switch (frame)
    {
    case StoredFrame::Unknown:
        return "unknown";
    case StoredFrame::NodeOutputSystem:
        return "nodeOutputSystem";
    case StoredFrame::Basic:
        return "basic";
    case StoredFrame::Element:
        return "element";
    case StoredFrame::Material:
        return "material";
    case StoredFrame::Ply:
        return "ply";
    case StoredFrame::Coordinate:
        return "coordinateSystem";
    case StoredFrame::Cylindrical:
        return "cylindrical";
    case StoredFrame::Spherical:
        return "spherical";
    case StoredFrame::Mixed:
        return "mixed";
    }
    return "unknown";
}

namespace
{

// The positions (increasing) of the rows of `data` that the filter keeps: by grid point for a nodal table, by element
// and layer for an element table.
std::vector<std::uint32_t> selectRows(const ResultData& data, const ResultFilter& filter)
{
    if (!data.isElementData())
    {
        std::vector<std::uint32_t> rows =
            filter.ids ? selectIdPositions(*filter.ids, data.ids) : allPositions(data.rowCount);
        if (filter.elementIds && !data.elementIds.empty())
        {
            // rows that are a grid point of an element (grid point forces): the elements narrow them too
            std::vector<std::uint32_t> kept;
            kept.reserve(rows.size());
            const std::vector<std::uint32_t> byElement = selectIdPositions(*filter.elementIds, data.elementIds);
            std::set_intersection(rows.begin(), rows.end(), byElement.begin(), byElement.end(),
                                  std::back_inserter(kept));
            rows = std::move(kept);
        }
        return rows;
    }
    std::vector<std::uint32_t> rows =
        filter.elementIds ? selectIdPositions(*filter.elementIds, data.ids) : allPositions(data.rowCount);
    if (filter.layers)
    {
        std::vector<std::uint32_t> kept;
        kept.reserve(rows.size());
        for (const std::uint32_t row : rows)
            if (std::find(filter.layers->begin(), filter.layers->end(), data.layers[row]) != filter.layers->end())
                kept.push_back(row);
        rows = std::move(kept);
    }
    return rows;
}

} // namespace

namespace
{

// True when `positions` are every position of `count` in order.
bool everyPosition(const std::vector<std::uint32_t>& positions, std::size_t count)
{
    if (positions.size() != count)
        return false;
    for (std::size_t position = 0; position < count; ++position)
        if (positions[position] != position)
            return false;
    return true;
}

// The subset of `data`; `shared` (the same data, shared) is given back as it is when the filter keeps all of it.
std::shared_ptr<const ResultData> subsetOf(const ResultData& data, const ResultFilter& filter, StepKind stepKind,
                                           const std::shared_ptr<const ResultData>* shared)
{
    const std::vector<std::uint32_t> stepPositions =
        filter.hasStepSelection() ? selectStepPositions(filter, data.steps, stepKind) : allPositions(data.stepCount);
    const std::vector<std::uint32_t> rowPositions = selectRows(data, filter);
    const std::vector<std::uint32_t> componentPositions =
        filter.components ? selectComponentPositions(*filter.components, data.components)
                          : allPositions(data.componentCount);
    if (stepPositions.empty() || rowPositions.empty() || componentPositions.empty())
    {
        const char* noRow = data.isElementData() ? "no row (none of the elements and layers is in the table)"
                                                 : "no row (none of the IDs is in the table)";
        const std::string what = stepPositions.empty() ? "no step" : (rowPositions.empty() ? noRow : "no component");
        throw base::EmptySelectionError("the selection (" + filter.describe() + ") keeps " + what + " of the table");
    }
    // PERFORMANCE: a filter that keeps every row of a table made in memory (the elements of a
    // block are all those of the table) copied it for every subcase; the data, never written once made, is shared
    if (shared != nullptr && everyPosition(stepPositions, data.stepCount) &&
        everyPosition(rowPositions, data.rowCount) && everyPosition(componentPositions, data.componentCount))
        return *shared;
    auto result = std::make_shared<ResultData>();
    result->type = data.type;
    result->stepCount = stepPositions.size();
    result->rowCount = rowPositions.size();
    result->componentCount = componentPositions.size();
    result->allocate();
    result->steps = gather<double>(data.steps, stepPositions);
    result->eigenvalues = gather<double>(data.eigenvalues, stepPositions);
    result->eigenvaluesImag = gather<double>(data.eigenvaluesImag, stepPositions);
    result->cycles = gather<double>(data.cycles, stepPositions);
    result->ids = gather<std::int32_t>(data.ids, rowPositions);
    result->pointTypes = gather<std::int32_t>(data.pointTypes, rowPositions);
    result->nodeIds = gather<std::int32_t>(data.nodeIds, rowPositions);
    result->elementIds = gather<std::int32_t>(data.elementIds, rowPositions);
    result->layers = gather<std::int32_t>(data.layers, rowPositions);
    result->positions = gather<double>(data.positions, rowPositions);
    for (const ResultText& text : data.texts)
        result->texts.push_back(text.gather(rowPositions));
    for (const std::uint32_t position : componentPositions)
        result->components.push_back(data.components[position]);

    const std::size_t size = valueTypeSize(data.type);
    const bool wholeRows = componentPositions.size() == data.componentCount &&
                           std::is_sorted(componentPositions.begin(), componentPositions.end());
    const std::byte* source = data.bytes();
    std::byte* target = result->mutableBytes();
    for (const std::uint32_t step : stepPositions)
    {
        const std::byte* stepBase =
            source + static_cast<std::size_t>(step) * data.rowCount * data.componentCount * size;
        for (const std::uint32_t row : rowPositions)
        {
            const std::byte* rowBase = stepBase + static_cast<std::size_t>(row) * data.componentCount * size;
            if (wholeRows)
            {
                std::memcpy(target, rowBase, data.componentCount * size);
                target += data.componentCount * size;
            }
            else
            {
                for (const std::uint32_t component : componentPositions)
                {
                    std::memcpy(target, rowBase + static_cast<std::size_t>(component) * size, size);
                    target += size;
                }
            }
        }
    }
    return result;
}

} // namespace

std::shared_ptr<const ResultData> subsetResultData(const ResultData& data, const ResultFilter& filter,
                                                   StepKind stepKind)
{
    return subsetOf(data, filter, stepKind, nullptr);
}

std::shared_ptr<const ResultData> subsetResultData(const std::shared_ptr<const ResultData>& data,
                                                   const ResultFilter& filter, StepKind stepKind)
{
    return subsetOf(*data, filter, stepKind, &data);
}

ResultTable::ResultTable(ResultTableMeta meta, std::shared_ptr<const ResultTableSource> source)
    : m_meta(std::move(meta)), m_source(std::move(source))
{
}

ResultTable::ResultTable(ResultTableMeta meta, std::shared_ptr<const ResultData> data)
    : m_meta(metaFromData(std::move(meta), *data)), m_data(std::move(data))
{
}

void ResultTable::setPending(ResultFilter filter)
{
    m_pending.clear();
    if (filter.hasRowSelection() || filter.hasStepSelection())
        m_pending.push_back(std::move(filter));
}

bool ResultTable::isDecoded() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_data != nullptr;
}

std::shared_ptr<const ResultData> ResultTable::data(const DecodeContext& context) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (m_data)
        return m_data;
    LoadRequest request;
    request.components = m_meta.components;
    if (!m_pending.empty())
        request.filter = m_pending.front();
    std::shared_ptr<const ResultData> loaded = m_source->load(request, context);
    for (std::size_t link = 1; link < m_pending.size(); ++link)
        loaded = subsetResultData(loaded, m_pending[link], m_meta.stepKind);
    m_data = std::move(loaded);
    return m_data;
}

std::size_t ResultTable::stepCount(const DecodeContext& context) const
{
    return m_meta.stepCountKnown ? m_meta.stepCount : data(context)->stepCount;
}

std::size_t ResultTable::rowCount(const DecodeContext& context) const
{
    return m_meta.rowCountKnown ? m_meta.rowCount : data(context)->rowCount;
}

std::vector<double> ResultTable::steps(const DecodeContext& context) const
{
    return m_meta.stepsKnown ? m_meta.steps : data(context)->steps;
}

void ResultTable::setAttachment(std::weak_ptr<ResultAttachment> attachment)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_attachment = std::move(attachment);
}

std::shared_ptr<ResultAttachment> ResultTable::attachment() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_attachment.lock();
}

std::shared_ptr<ResultTable> ResultTable::select(const ResultFilter& filter) const
{
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        if (m_data)
        {
            const std::shared_ptr<const ResultData> subset = subsetResultData(m_data, filter, m_meta.stepKind);
            auto copy = std::make_shared<ResultTable>(m_meta, subset);
            copy->m_attachment = m_attachment;
            return copy;
        }
    }
    ResultTableMeta meta = m_meta;
    std::shared_ptr<const ResultTableSource> source = m_source;
    if (filter.components)
    {
        const std::vector<std::uint32_t> positions = selectComponentPositions(*filter.components, m_meta.components);
        if (positions.empty())
            throw base::EmptySelectionError("the selection (" + filter.describe() +
                                            ") keeps no component of the table");
        meta.components.clear();
        for (const std::uint32_t position : positions)
            meta.components.push_back(m_meta.components[position]);
    }
    // what the table applies while it decodes: the grid points of a nodal table, the elements and layers of an element
    // table (the other IDs of the filter do not concern it), and the steps of a SORT2 table
    ResultFilter link;
    switch (resultKindInfo(m_meta.kind).rows)
    {
    case RowKind::Element:
        link.elementIds = filter.elementIds;
        link.layers = filter.layers;
        break;
    case RowKind::GridPointElement:
        link.ids = filter.ids;
        link.elementIds = filter.elementIds;
        break;
    case RowKind::GridPoint:
    case RowKind::Mode:
        link.ids = filter.ids;
        break;
    }
    if (filter.hasStepSelection())
    {
        if (m_source->stepsInData())
        {
            link.steps = filter.steps;
            link.stepRange = filter.stepRange;
            link.stepIndices = filter.stepIndices;
            link.stepSlice = filter.stepSlice;
            meta.stepCountKnown = false;
            meta.stepsKnown = false;
        }
        else
        {
            const std::vector<std::uint32_t> positions = selectStepPositions(filter, m_meta.steps, m_meta.stepKind);
            if (positions.empty())
                throw base::EmptySelectionError("the selection (" + filter.describe() + ") keeps no step of the table");
            source = m_source->withSteps(positions);
            meta.steps = gather<double>(m_meta.steps, positions);
            meta.eigenvalues = gather<double>(m_meta.eigenvalues, positions);
            meta.eigenvaluesImag = gather<double>(m_meta.eigenvaluesImag, positions);
            meta.cycles = gather<double>(m_meta.cycles, positions);
            meta.stepCount = positions.size();
        }
    }
    std::vector<ResultFilter> pending = m_pending;
    if (link.hasRowSelection() || link.hasStepSelection())
    {
        if (link.hasRowSelection())
            meta.rowCountKnown = false;
        if (!pending.empty() && !link.hasStepSelection() && !pending.back().hasStepSelection())
        {
            // two selections of rows meet: what both keep
            const auto meet = [&](std::optional<std::vector<std::int32_t>>& earlier,
                                  const std::optional<std::vector<std::int32_t>>& later, const char* what)
            {
                if (!later)
                    return;
                if (!earlier)
                {
                    earlier = later;
                    return;
                }
                std::vector<std::int32_t> common;
                std::vector<std::int32_t> left = *earlier;
                std::vector<std::int32_t> right = *later;
                std::sort(left.begin(), left.end());
                std::sort(right.begin(), right.end());
                std::set_intersection(left.begin(), left.end(), right.begin(), right.end(), std::back_inserter(common));
                if (common.empty())
                    throw base::EmptySelectionError("the selection (" + filter.describe() + ") keeps no row: its " +
                                                    what + " and the earlier ones have none in common");
                earlier = std::move(common);
            };
            meet(pending.back().ids, link.ids, "IDs");
            meet(pending.back().elementIds, link.elementIds, "element IDs");
            meet(pending.back().layers, link.layers, "layers");
        }
        else
        {
            pending.push_back(link);
        }
    }
    auto result = std::make_shared<ResultTable>(std::move(meta), std::move(source));
    result->m_pending = std::move(pending);
    result->m_attachment = m_attachment;
    return result;
}

} // namespace oor::results
