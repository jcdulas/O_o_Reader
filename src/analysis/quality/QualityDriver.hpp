#pragma once

#include "ElementPoints.hpp"

#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/quality/MeshQuality.hpp"
#include "oor/base/threading/ParallelFor.hpp"

#include <span>

// The loop the shell and the solid quality share: for each element the nodes are found and their positions gathered,
// and the function of the shape family evaluates them. Large calls are cut into chunks of rows over the thread pool,
// each chunk writing its own entries of the arrays, so the result is the same for any number of threads.

namespace oor::analysis::detail
{

// `evaluate(points)` takes the positions of the nodes of one element in the order of the card and gives its
// QualityValues.
template <typename Evaluate>
[[nodiscard]] QualityArrays computeQuality(const model::Model& model, std::uint16_t typeIndex,
                                           std::span<const std::uint32_t> rows, base::ThreadPool* pool,
                                           const Evaluate& evaluate)
{
    constexpr std::size_t ROWS_PER_CHUNK = 4096;
    constexpr std::size_t PARALLEL_ROW_COUNT = 8192;
    QualityArrays arrays;
    arrays.assign(rows.size());
    const ElementProvider* provider = ProviderRegistry::instance().find(typeIndex);
    const model::CardTable* table = model.findTable(typeIndex);
    if (provider == nullptr || table == nullptr || rows.empty())
        return arrays;
    BasicPositionCache& cache = BasicPositionCache::of(model, pool);
    const NodeLookup lookup(model, rows.size() * 3);
    const auto work = [&](std::size_t begin, std::size_t end)
    {
        RowScratch scratch;
        for (std::size_t index = begin; index < end; ++index)
        {
            gatherElementPoints(*provider, *table, cache, lookup, scratch, rows[index]);
            arrays.store(index, evaluate(std::span<const base::Vec3>(scratch.points)));
        }
    };
    if (pool != nullptr && pool->workerCount() > 0 && rows.size() >= PARALLEL_ROW_COUNT)
    {
        const std::vector<base::IndexRange> chunks = base::splitIntoChunks(rows.size(), ROWS_PER_CHUNK);
        base::parallelFor(*pool, chunks.size(),
                          [&](std::size_t chunk) { work(chunks[chunk].begin, chunks[chunk].end); });
    }
    else
    {
        work(0, rows.size());
    }
    return arrays;
}

// The quality of one element, as the object-by-object loops ask for it: no arrays, the scratch lists of the thread
// reused.
template <typename Evaluate>
[[nodiscard]] QualityValues qualityOfOne(const model::Model& model, model::CardLocation location,
                                         const Evaluate& evaluate)
{
    const ElementProvider* provider = ProviderRegistry::instance().find(location.table);
    const model::CardTable* table = model.findTable(location.table);
    if (provider == nullptr || table == nullptr)
        return {};
    BasicPositionCache& cache = BasicPositionCache::of(model);
    const NodeLookup lookup(model, 3);
    thread_local RowScratch scratch;
    gatherElementPoints(*provider, *table, cache, lookup, scratch, location.row);
    return evaluate(std::span<const base::Vec3>(scratch.points));
}

} // namespace oor::analysis::detail
