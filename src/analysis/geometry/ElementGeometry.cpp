#include "oor/analysis/geometry/ElementGeometry.hpp"

#include "ElementPoints.hpp"
#include "ElementProvider.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <algorithm>
#include <string>

namespace oor::analysis
{

namespace detail
{

const ProviderRegistry& ProviderRegistry::instance()
{
    static const ProviderRegistry REGISTRY = []
    {
        ProviderRegistry registry;
        registerTriangleProviders(registry);
        registerQuadProviders(registry);
        registerTetraProviders(registry);
        registerPentaProviders(registry);
        registerHexaProviders(registry);
        registerPyramidProviders(registry);
        registerLineProviders(registry);
        registerPointProviders(registry);
        registerRigidProviders(registry);
        registerSurfaceProviders(registry);
        registerAcousticProviders(registry);
        registerInterfaceProviders(registry);
        registerCrackSeamSlotProviders(registry);
        registerSol700ScalarProviders(registry);
        registerDummyProviders(registry);
        registerRigidSplineProviders(registry);
        registerPlotProviders(registry);
        return registry;
    }();
    return REGISTRY;
}

} // namespace detail

namespace
{

constexpr std::size_t ROWS_PER_CHUNK = 4096;
constexpr std::size_t PARALLEL_ROW_COUNT = 8192;

void allocate(ElementArrays& arrays, std::size_t count, std::uint8_t quantities)
{
    if ((quantities & bit(Quantity::Center)) != 0)
        arrays.centers.assign(count, undefinedVector());
    if ((quantities & bit(Quantity::Normal)) != 0)
        arrays.normals.assign(count, undefinedVector());
    if ((quantities & bit(Quantity::Area)) != 0)
        arrays.areas.assign(count, NOT_DEFINED);
    if ((quantities & bit(Quantity::Volume)) != 0)
        arrays.volumes.assign(count, NOT_DEFINED);
    if ((quantities & bit(Quantity::Length)) != 0)
        arrays.lengths.assign(count, NOT_DEFINED);
    if ((quantities & bit(Quantity::Thickness)) != 0)
        arrays.thicknesses.assign(count, NOT_DEFINED);
    if ((quantities & bit(Quantity::Frame)) != 0)
        arrays.frames.assign(count, undefinedFrame());
    if ((quantities & bit(Quantity::MaterialFrame)) != 0)
        arrays.materialFrames.assign(count, undefinedFrame());
}

void store(ElementArrays& arrays, std::size_t index, const ElementValues& values, std::uint8_t quantities)
{
    if ((quantities & bit(Quantity::Center)) != 0)
        arrays.centers[index] = values.center;
    if ((quantities & bit(Quantity::Normal)) != 0)
        arrays.normals[index] = values.normal;
    if ((quantities & bit(Quantity::Area)) != 0)
        arrays.areas[index] = values.area;
    if ((quantities & bit(Quantity::Volume)) != 0)
        arrays.volumes[index] = values.volume;
    if ((quantities & bit(Quantity::Length)) != 0)
        arrays.lengths[index] = values.length;
    if ((quantities & bit(Quantity::Thickness)) != 0)
        arrays.thicknesses[index] = values.thickness;
    if ((quantities & bit(Quantity::Frame)) != 0)
        arrays.frames[index] = values.frame;
    if ((quantities & bit(Quantity::MaterialFrame)) != 0)
        arrays.materialFrames[index] = values.materialFrame;
}

// The values of one element: its nodes are found, their positions gathered, and the provider of its card type
// evaluates.
void evaluateRow(const detail::ElementProvider& provider, const model::CardTable& table, BasicPositionCache& cache,
                 const detail::NodeLookup& lookup, const detail::GeometryContext& context, detail::RowScratch& scratch,
                 std::uint32_t row, std::uint8_t quantities, ElementValues& values)
{
    detail::gatherElementPoints(provider, table, cache, lookup, scratch, row);
    const detail::Evaluation evaluation{context, table, row, quantities, scratch.ids, scratch.nodeRows, scratch.points};
    provider.evaluate(evaluation, values);
}

} // namespace

bool ElementGeometry::supports(std::uint16_t typeIndex) noexcept
{
    return detail::ProviderRegistry::instance().find(typeIndex) != nullptr;
}

ElementArrays ElementGeometry::compute(const model::Model& model, std::uint16_t typeIndex,
                                       std::span<const std::uint32_t> rows, std::uint8_t quantities,
                                       base::ThreadPool* pool)
{
    ElementArrays arrays;
    allocate(arrays, rows.size(), quantities);
    const detail::ElementProvider* provider = detail::ProviderRegistry::instance().find(typeIndex);
    const model::CardTable* table = model.findTable(typeIndex);
    if (provider == nullptr || table == nullptr || rows.empty())
        return arrays;

    BasicPositionCache& cache = BasicPositionCache::of(model, pool);
    const detail::NodeLookup lookup(model, rows.size() * 3);
    const auto work = [&](std::size_t begin, std::size_t end)
    {
        detail::GeometryContext context(model, cache);
        detail::RowScratch scratch;
        for (std::size_t index = begin; index < end; ++index)
        {
            ElementValues values;
            evaluateRow(*provider, *table, cache, lookup, context, scratch, rows[index], quantities, values);
            store(arrays, index, values, quantities);
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

ElementValues ElementGeometry::of(const model::Model& model, model::CardLocation location, std::uint8_t quantities)
{
    // One element, as the object-by-object loops ask for it: no arrays, and the scratch lists of the thread are reused.
    ElementValues values;
    const detail::ElementProvider* provider = detail::ProviderRegistry::instance().find(location.table);
    const model::CardTable* table = model.findTable(location.table);
    if (provider == nullptr || table == nullptr)
        return values;
    BasicPositionCache& cache = BasicPositionCache::of(model);
    const detail::NodeLookup lookup(model, 3);
    detail::GeometryContext context(model, cache);
    thread_local detail::RowScratch scratch;
    evaluateRow(*provider, *table, cache, lookup, context, scratch, location.row, quantities, values);
    return values;
}

} // namespace oor::analysis
