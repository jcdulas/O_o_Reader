#include "oor/analysis/frames/NodeDisplacementFrames.hpp"

#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <unordered_map>

namespace oor::analysis
{

namespace
{

constexpr std::size_t ROWS_PER_CHUNK = 8192;
constexpr std::size_t PARALLEL_ROW_COUNT = 20000;

} // namespace

std::vector<base::Mat3> NodeDisplacementFrames::rotations(const model::Model& model,
                                                          std::span<const std::uint32_t> rows, base::ThreadPool* pool)
{
    std::vector<base::Mat3> result(rows.size());
    const model::NodeTable* grids = model.findTable<model::NodeTable>();
    if (grids == nullptr || rows.empty())
        return result;
    BasicPositionCache& cache = BasicPositionCache::of(model, pool);

    // The systems the grid points use, resolved one at a time (the cache keeps what it resolves, and is shared); the
    // grid points then only read them.
    std::unordered_map<base::EntityId, CoordinateFrame> frames;
    {
        const std::lock_guard<std::mutex> lock(cache.mutex());
        base::EntityId last = 0;
        for (const std::uint32_t row : rows)
        {
            const base::EntityId cd = grids->displacementFrameId(row);
            if (cd <= 0 || cd == last)
                continue;
            last = cd;
            if (frames.contains(cd))
                continue;
            if (!cache.transform().hasSystem(cd))
                throw CoordinateSystemError("grid " + std::to_string(grids->idAtRow(row)) +
                                            " has the displacement system " + std::to_string(cd) +
                                            ", which does not exist");
            frames.emplace(cd, cache.transform().frame(cd));
        }
    }

    const auto work = [&](std::size_t begin, std::size_t end)
    {
        base::EntityId lastCd = 0;
        const CoordinateFrame* lastFrame = nullptr;
        for (std::size_t index = begin; index < end; ++index)
        {
            const std::uint32_t row = rows[index];
            const base::EntityId cd = grids->displacementFrameId(row);
            if (cd <= 0)
                continue; // the identity
            if (cd != lastCd)
            {
                lastCd = cd;
                lastFrame = &frames.at(cd);
            }
            result[index] = lastFrame->directionsAt(cache.position(row));
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
    return result;
}

} // namespace oor::analysis
