#include "oor/analysis/mass/MassCalculator.hpp"

#include "MassProvider.hpp"
#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/mass/Laminate.hpp"
#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/analysis/mass/MaterialProperties.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/parameters/ParameterValues.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace oor::analysis
{

namespace
{

constexpr std::size_t ROWS_PER_CHUNK = 2048;
constexpr std::size_t PARALLEL_ROW_COUNT = 4096;
constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The card types whose mass exists but is not computed: the geometry of a ShellConical2 is not known.
bool isNotComputed(std::uint16_t type)
{
    return model::CardRegistry::instance().entry(type).info.name == "ShellConical2";
}

// The geometry the provider asked for, as the values of one element.
ElementValues valuesAt(const ElementArrays& arrays, std::size_t index, std::uint8_t quantities)
{
    ElementValues values;
    if ((quantities & bit(Quantity::Center)) != 0)
        values.center = arrays.centers[index];
    if ((quantities & bit(Quantity::Normal)) != 0)
        values.normal = arrays.normals[index];
    if ((quantities & bit(Quantity::Area)) != 0)
        values.area = arrays.areas[index];
    if ((quantities & bit(Quantity::Volume)) != 0)
        values.volume = arrays.volumes[index];
    if ((quantities & bit(Quantity::Length)) != 0)
        values.length = arrays.lengths[index];
    if ((quantities & bit(Quantity::Thickness)) != 0)
        values.thickness = arrays.thicknesses[index];
    if ((quantities & bit(Quantity::Frame)) != 0)
        values.frame = arrays.frames[index];
    if ((quantities & bit(Quantity::MaterialFrame)) != 0)
        values.materialFrame = arrays.materialFrames[index];
    return values;
}

void fillWithoutMass(ElementMasses& masses, std::size_t count, double mass)
{
    masses.mass.assign(count, mass);
    masses.structural.assign(count, mass);
    masses.nonStructural.assign(count, 0.0);
    masses.center.assign(count, undefinedVector());
    masses.property.assign(count, 0);
    masses.material.assign(count, 0);
}

// The materials of the structural mass of a property and the fraction each one has: one material for most properties,
// the plies for a laminate (by density times thickness).
std::vector<std::pair<base::EntityId, double>> materialShares(const model::Model& model, base::EntityId pid,
                                                              base::EntityId material)
{
    if (pid != 0)
    {
        if (const auto location = model.locate(base::IdSpace::Property, pid);
            location && Laminate::supports(location->table))
        {
            std::vector<std::pair<base::EntityId, double>> shares;
            double total = 0.0;
            for (const LaminatePly& ply : Laminate::stack(model, *location))
            {
                const double mass = MaterialProperties::density(model, ply.materialId).value_or(0.0) * ply.thickness;
                shares.emplace_back(ply.materialId, mass);
                total += mass;
            }
            if (total > 0.0)
            {
                for (auto& share : shares)
                    share.second /= total;
            }
            return shares;
        }
    }
    return {{material, 1.0}};
}

} // namespace

double MassCalculator::weightFactorOf(const model::Model& model)
{
    // PARAM,WTMASS: the last card that holds a number, else the format reference default (1.0).
    return model::realParameter(model, "WeightToMass").value_or(1.0);
}

OffsetMass MassCalculator::offsetMassOf(const model::Model& model)
{
    const std::optional<model::ModelParameter> given = model::modelParameter(model, "OffsetDefinition");
    if (!given)
        return OffsetMass::GridPoints;
    if (given->kind == model::ModelParameterKind::Text)
    {
        const std::string& value = given->textValue;
        if (value == "ElemOffsetsOnly" || value == "NoMassOffsets" || value == "NoDampingOffsets")
            return OffsetMass::GridPoints;
        if (value == "LoadRecoveryOffsets" || value == "NoOffsetDifference" || value == "NoThermalOffsets" ||
            value == "NoTemperatureGradientOffsets" || value == "ElemOffsetZ")
            return OffsetMass::Offsets;
    }
    const std::string shown =
        given->kind == model::ModelParameterKind::Text
            ? given->textValue
            : (given->kind == model::ModelParameterKind::Integer ? std::to_string(given->integerValue) : "a real");
    const std::string parameter = std::string("ModelParams") + " " + std::string("OffsetDefinition");
    throw MassError(parameter + " " + shown +
                    ": the offsets of the elements are ElemOffsetsOnly, LoadRecoveryOffsets, NoOffsetDifference, "
                    "NoThermalOffsets, NoTemperatureGradientOffsets, ElemOffsetZ, NoMassOffsets or NoDampingOffsets");
}

ElementMasses MassCalculator::compute(const model::Model& model, std::uint16_t typeIndex,
                                      std::span<const std::uint32_t> rows, const MassOptions& options,
                                      base::ThreadPool* pool)
{
    ElementMasses masses;
    const std::size_t count = rows.size();
    const model::CardTable* table = model.findTable(typeIndex);
    if (table == nullptr || count == 0)
    {
        fillWithoutMass(masses, count, 0.0);
        return masses;
    }
    const detail::MassProvider* provider = detail::MassRegistry::instance().find(typeIndex);
    if (provider == nullptr)
    {
        fillWithoutMass(masses, count, isNotComputed(typeIndex) ? NOT_A_NUMBER : 0.0);
        return masses;
    }
    fillWithoutMass(masses, count, 0.0);
    const std::uint8_t quantities = provider->quantities();
    const OffsetMass offsets = options.offsets == OffsetMass::Model ? offsetMassOf(model) : options.offsets;
    const ElementArrays arrays = ElementGeometry::compute(model, typeIndex, rows, quantities, pool);
    BasicPositionCache& cache = BasicPositionCache::of(model, pool);

    const std::vector<base::IndexRange> chunks =
        pool != nullptr && pool->workerCount() > 0 && count >= PARALLEL_ROW_COUNT
            ? base::splitIntoChunks(count, ROWS_PER_CHUNK)
            : std::vector<base::IndexRange>{base::IndexRange{0, count}};
    std::vector<std::vector<OwnInertia>> inertias(chunks.size());
    const auto work = [&](std::size_t chunk)
    {
        detail::GeometryContext geometryContext(model, cache);
        detail::MassContext context(geometryContext, offsets == OffsetMass::Offsets);
        for (std::size_t index = chunks[chunk].begin; index < chunks[chunk].end; ++index)
        {
            const ElementValues values = valuesAt(arrays, index, quantities);
            detail::MassValue value;
            provider->evaluate(context, *table, rows[index], values, value);
            const double extra =
                options.nonStructural != nullptr ? options.nonStructural->massOf(typeIndex, rows[index]) : 0.0;
            const double factor = options.weightFactor;
            masses.structural[index] = value.structural * factor;
            masses.nonStructural[index] = (value.nonStructural + extra) * factor;
            masses.mass[index] = masses.structural[index] + masses.nonStructural[index];
            masses.center[index] = value.center;
            masses.property[index] = value.property;
            masses.material[index] = value.material;
            if (value.hasInertia)
            {
                OwnInertia own;
                own.index = static_cast<std::uint32_t>(index);
                for (std::size_t entry = 0; entry < 9; ++entry)
                    own.tensor.values[entry] = value.inertia.values[entry] * factor;
                inertias[chunk].push_back(own);
            }
        }
    };
    if (chunks.size() > 1)
        base::parallelFor(*pool, chunks.size(), work);
    else
        work(0);
    for (const std::vector<OwnInertia>& chunk : inertias)
        masses.inertia.insert(masses.inertia.end(), chunk.begin(), chunk.end());
    return masses;
}

ElementMasses MassCalculator::of(const model::Model& model, model::CardLocation location, const MassOptions& options)
{
    const std::uint32_t rows[1] = {location.row};
    return compute(model, location.table, rows, options);
}

std::vector<RowSet> MassCalculator::allElements(const model::Model& model)
{
    std::vector<RowSet> sets;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr)
            continue;
        if (detail::MassRegistry::instance().find(type) == nullptr && !isNotComputed(type))
            continue;
        RowSet set;
        set.type = type;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                set.rows.push_back(row);
        }
        if (!set.rows.empty())
            sets.push_back(std::move(set));
    }
    return sets;
}

namespace
{

// Calls `visit(index, mass, center, inertia)` for each element of `masses` that has mass or an inertia and a place, and
// counts the others.
template <typename Visit>
void visitBodies(const ElementMasses& masses, const char* name, MassTotals& totals, Visit&& visit)
{
    std::size_t next = 0; // the next own inertia, by index
    for (std::size_t index = 0; index < masses.mass.size(); ++index)
    {
        const base::Mat3* inertia = nullptr;
        while (next < masses.inertia.size() && masses.inertia[next].index < index)
            ++next;
        if (next < masses.inertia.size() && masses.inertia[next].index == index)
            inertia = &masses.inertia[next].tensor;
        const double mass = masses.mass[index];
        ++totals.elements;
        if (mass != mass)
        {
            ++totals.excluded[name];
            continue;
        }
        if (mass == 0.0 && inertia == nullptr)
            continue;
        if (!isDefined(masses.center[index]))
        {
            ++totals.excluded[name];
            continue;
        }
        visit(index, mass, masses.center[index], inertia);
    }
}

// The own inertias are produced by chunk, so by index within a chunk and chunk after chunk: sorted by index.
void sortInertia(ElementMasses& masses)
{
    std::sort(masses.inertia.begin(), masses.inertia.end(),
              [](const OwnInertia& left, const OwnInertia& right) { return left.index < right.index; });
}

} // namespace

MassTotals MassCalculator::total(const model::Model& model, const std::vector<RowSet>& sets, const MassOptions& options,
                                 base::ThreadPool* pool)
{
    MassTotals totals;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (const RowSet& set : sets)
    {
        ElementMasses masses = compute(model, set.type, set.rows, options, pool);
        sortInertia(masses);
        const char* name = registry.entry(set.type).info.name.data();
        visitBodies(masses, name, totals,
                    [&](std::size_t, double mass, const base::Vec3& center, const base::Mat3* inertia)
                    { totals.sums.add(mass, center, inertia); });
    }
    return totals;
}

MassGroups MassCalculator::group(const model::Model& model, const std::vector<RowSet>& sets, MassGrouping grouping,
                                 const MassOptions& options, base::ThreadPool* pool)
{
    MassGroups groups;
    MassTotals ignored;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    std::map<base::EntityId, std::vector<std::pair<base::EntityId, double>>> shareCache;
    for (const RowSet& set : sets)
    {
        ElementMasses masses = compute(model, set.type, set.rows, options, pool);
        sortInertia(masses);
        const std::string name(registry.entry(set.type).info.name);
        visitBodies(masses, name.c_str(), ignored,
                    [&](std::size_t index, double mass, const base::Vec3& center, const base::Mat3* inertia)
                    {
                        switch (grouping)
                        {
                        case MassGrouping::Property:
                            groups.byId[masses.property[index]].add(mass, center, inertia);
                            break;
                        case MassGrouping::ElementType:
                            groups.byName[name].add(mass, center, inertia);
                            break;
                        case MassGrouping::Material:
                        {
                            // The structural mass goes to its material, or to its plies' materials in proportion; the
                            // rest, which no material owns, to the key 0. The own inertia of an element goes with its
                            // structural mass.
                            const double structural = masses.structural[index];
                            const double rest = mass - structural;
                            const base::EntityId pid = masses.property[index];
                            std::vector<std::pair<base::EntityId, double>> single;
                            const std::vector<std::pair<base::EntityId, double>>* shares = &single;
                            if (masses.material[index] != 0 || pid == 0)
                            {
                                single.emplace_back(masses.material[index], 1.0);
                            }
                            else
                            {
                                auto cached = shareCache.find(pid);
                                if (cached == shareCache.end())
                                    cached = shareCache.emplace(pid, materialShares(model, pid, 0)).first;
                                shares = &cached->second;
                            }
                            const auto& list = *shares;
                            bool first = true;
                            for (const auto& [mid, fraction] : list)
                            {
                                const double part = structural * fraction;
                                if (part == 0.0 && !first)
                                    continue;
                                groups.byId[mid].add(part, center, first ? inertia : nullptr);
                                first = false;
                            }
                            if (rest != 0.0)
                                groups.byId[0].add(rest, center);
                            break;
                        }
                        }
                    });
    }
    return groups;
}

} // namespace oor::analysis
