#include "oor/analysis/coordinates/SystemTransforms.hpp"

#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/ParallelFor.hpp"

#include <string>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

constexpr std::size_t ITEMS_PER_CHUNK = 16384;
constexpr std::size_t PARALLEL_ITEM_COUNT = 32768;

// The systems a call names, resolved once each: the item i reads the frame of cids[i] (or of cids[0] for every item).
class Frames
{
public:
    Frames(const model::Model& model, std::span<const base::EntityId> cids, std::size_t itemCount)
        : m_cids(cids), m_one(cids.size() == 1)
    {
        if (cids.size() != 1 && cids.size() != itemCount)
            throw base::UsageError("coordinate transforms: give one system for every item or one per item (" +
                                   std::to_string(cids.size()) + " systems for " + std::to_string(itemCount) +
                                   " items)");
        CoordinateTransform transform(model);
        base::EntityId last = -1;
        for (const base::EntityId cid : cids)
        {
            if (cid == last)
                continue;
            last = cid;
            if (m_frames.contains(cid))
                continue;
            if (cid < 0)
                throw CoordinateSystemError("coordinate system " + std::to_string(cid) + " does not exist");
            const CoordinateFrame& frame = transform.frame(cid);
            m_frames.emplace(cid, frame);
            m_curvilinear = m_curvilinear || frame.kind != CoordinateKind::Rectangular;
        }
        if (m_one)
            m_single = &m_frames.at(cids[0]);
    }

    // True when one of the systems is cylindrical or spherical: its directions depend on the point.
    [[nodiscard]] bool curvilinear() const noexcept { return m_curvilinear; }

    // The frame of the item; `lastCid` and `lastFrame` remember the last one looked up (an item usually has the
    // system of the one before).
    [[nodiscard]] const CoordinateFrame& of(std::size_t item, base::EntityId& lastCid,
                                            const CoordinateFrame*& lastFrame) const
    {
        if (m_one)
            return *m_single;
        const base::EntityId cid = m_cids[item];
        if (lastFrame == nullptr || cid != lastCid)
        {
            lastCid = cid;
            lastFrame = &m_frames.at(cid);
        }
        return *lastFrame;
    }

private:
    std::span<const base::EntityId> m_cids;
    bool m_one;
    bool m_curvilinear = false;
    std::unordered_map<base::EntityId, CoordinateFrame> m_frames;
    const CoordinateFrame* m_single = nullptr;
};

// Runs `body(begin, end)` over the items, on the threads of the pool when there are many.
template <typename Body>
void overItems(std::size_t count, base::ThreadPool* pool, const Body& body)
{
    if (pool != nullptr && pool->workerCount() > 0 && count >= PARALLEL_ITEM_COUNT)
    {
        const std::vector<base::IndexRange> chunks = base::splitIntoChunks(count, ITEMS_PER_CHUNK);
        base::parallelFor(*pool, chunks.size(),
                          [&](std::size_t chunk) { body(chunks[chunk].begin, chunks[chunk].end); });
    }
    else
    {
        body(0, count);
    }
}

void checkPointCount(std::span<const base::Vec3> at, std::size_t itemCount)
{
    if (at.size() != 1 && at.size() != itemCount)
        throw base::UsageError("coordinate transforms: give one point for every vector or one per vector (" +
                               std::to_string(at.size()) + " points for " + std::to_string(itemCount) + " vectors)");
}

} // namespace

std::vector<base::Vec3> SystemTransforms::points(const model::Model& model, std::span<const base::EntityId> cids,
                                                 std::span<const base::Vec3> points, TransformDirection direction,
                                                 base::ThreadPool* pool)
{
    const Frames frames(model, cids, points.size());
    std::vector<base::Vec3> result(points.size());
    overItems(points.size(), pool,
              [&](std::size_t begin, std::size_t end)
              {
                  base::EntityId lastCid = 0;
                  const CoordinateFrame* lastFrame = nullptr;
                  for (std::size_t item = begin; item < end; ++item)
                  {
                      const CoordinateFrame& frame = frames.of(item, lastCid, lastFrame);
                      result[item] = direction == TransformDirection::ToBasic ? frame.toBasic(points[item])
                                                                              : frame.fromBasic(points[item]);
                  }
              });
    return result;
}

std::vector<base::Vec3> SystemTransforms::vectors(const model::Model& model, std::span<const base::EntityId> cids,
                                                  std::span<const base::Vec3> vectors, std::span<const base::Vec3> at,
                                                  TransformDirection direction, base::ThreadPool* pool)
{
    const Frames frames(model, cids, vectors.size());
    if (at.empty())
    {
        if (frames.curvilinear())
            throw base::UsageError(
                "coordinate transforms: the directions of a cylindrical or spherical system "
                "depend on the point: give the points of the vectors (`at`, in the basic system)");
    }
    else
    {
        checkPointCount(at, vectors.size());
    }
    std::vector<base::Vec3> result(vectors.size());
    overItems(vectors.size(), pool,
              [&](std::size_t begin, std::size_t end)
              {
                  base::EntityId lastCid = 0;
                  const CoordinateFrame* lastFrame = nullptr;
                  for (std::size_t item = begin; item < end; ++item)
                  {
                      const CoordinateFrame& frame = frames.of(item, lastCid, lastFrame);
                      const base::Mat3 axes = frame.kind == CoordinateKind::Rectangular
                                                  ? frame.axes
                                                  : frame.directionsAt(at[at.size() == 1 ? 0 : item]);
                      result[item] = direction == TransformDirection::ToBasic ? axes * vectors[item]
                                                                              : axes.transposed() * vectors[item];
                  }
              });
    return result;
}

std::vector<base::Mat3> SystemTransforms::axesAt(const model::Model& model, std::span<const base::EntityId> cids,
                                                 std::span<const base::Vec3> at, base::ThreadPool* pool)
{
    const Frames frames(model, cids, at.size());
    std::vector<base::Mat3> result(at.size());
    overItems(at.size(), pool,
              [&](std::size_t begin, std::size_t end)
              {
                  base::EntityId lastCid = 0;
                  const CoordinateFrame* lastFrame = nullptr;
                  for (std::size_t item = begin; item < end; ++item)
                      result[item] = frames.of(item, lastCid, lastFrame).directionsAt(at[item]);
              });
    return result;
}

} // namespace oor::analysis
