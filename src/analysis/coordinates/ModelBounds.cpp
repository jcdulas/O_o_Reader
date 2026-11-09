#include "oor/analysis/coordinates/ModelBounds.hpp"

#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <algorithm>
#include <limits>

namespace oor::analysis
{

std::optional<BoundingBox> boundingBox(const model::Model& model, base::ThreadPool* pool)
{
    const model::NodeTable* grids = model.findTable<model::NodeTable>();
    if (grids == nullptr || grids->rows().aliveCount() == 0)
        return std::nullopt;
    const BasicPositionCache& cache = BasicPositionCache::of(model, pool);
    const std::span<const std::array<double, 3>> positions = cache.positions();
    constexpr double INFINITY_VALUE = std::numeric_limits<double>::infinity();
    std::array<double, 3> low = {INFINITY_VALUE, INFINITY_VALUE, INFINITY_VALUE};
    std::array<double, 3> high = {-INFINITY_VALUE, -INFINITY_VALUE, -INFINITY_VALUE};
    const model::RowBook& rows = grids->rows();
    for (std::uint32_t row = 0; row < positions.size(); ++row)
    {
        if (!rows.isAlive(row))
            continue;
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            low[axis] = std::min(low[axis], positions[row][axis]);
            high[axis] = std::max(high[axis], positions[row][axis]);
        }
    }
    return BoundingBox{base::Vec3{low[0], low[1], low[2]}, base::Vec3{high[0], high[1], high[2]}};
}

} // namespace oor::analysis
