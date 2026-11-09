#include "ComputedColumns.hpp"

#include "oor/analysis/adjacency/MeshAdjacency.hpp"
#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/analysis/quality/MeshQuality.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"
#include "oor/query/providers/ExportedRowOrder.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>

namespace oor::query
{

ComputationCounters& computationCounters() noexcept
{
    static ComputationCounters counters;
    return counters;
}

namespace detail
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

const char* const GEOMETRY_NAMES[] = {"area",    "volume",  "length",  "thickness", "centerX",
                                      "centerY", "centerZ", "normalX", "normalY",   "normalZ"};
const char* const MASS_NAMES[] = {"mass", "structuralMass", "nonstructuralMass"};
const char* const QUALITY_NAMES[] = {"aspectRatio",    "skew",     "warp",          "taper",
                                     "minAngle",       "maxAngle", "jacobianRatio", "jacobianMin",
                                     "minEdge",        "maxEdge",  "collapse",      "edgePointLengthRatio",
                                     "edgePointAngle", "faceWarp"};
const char* const ADJACENCY_NAMES[] = {"edgeNeighbourCount", "nodeNeighbourCount", "faceNeighbourCount"};
const char* const RESOLVED_NODE_NAMES[] = {"nodesRows"};
const char* const RESOLVED_PROPERTY_NAMES[] = {"propIdRow", "propType"};
// The fields of the elements (rigid elements included) that hold one grid point or scalar point (the format reference
// fields GA, GB, G0, G1, S1, SA, REFGRID, ...): each has a computed `<field>Row` column.
// tests/python/test_row_indices.py finds these fields from the reference visitors of the cards and fails when one is
// missing here.
const char* const SINGLE_POINT_FIELDS[] = {"node",
                                           "node1",
                                           "node2",
                                           "node3",
                                           "node4",
                                           "nodeA",
                                           "nodeB",
                                           "nodeC",
                                           "node3D",
                                           "axisymNode",
                                           "gMatId",
                                           "orientationNode",
                                           "independentNode",
                                           "referenceNode",
                                           "spotNode",
                                           "raValue",
                                           "rbValue",
                                           "scalarPoint1",
                                           "scalarPoint2",
                                           "stressRecoveryA",
                                           "bondAllowable",
                                           "compressionAllowable"};
constexpr std::string_view ROW_SUFFIX = "Row";
const char* const POSITION_NAMES[] = {"x", "y", "z", "xyzGlobal"};
const char* const COUNT_NAMES[] = {"elementCount"};

template <std::size_t N>
bool contains(const char* const (&names)[N], std::string_view name)
{
    return std::any_of(std::begin(names), std::end(names), [&](const char* candidate) { return name == candidate; });
}

// The grid point field of a `<field>Row` column name, or an empty view.
std::string_view pointFieldOf(std::string_view column)
{
    if (!column.ends_with(ROW_SUFFIX))
        return {};
    const std::string_view field = column.substr(0, column.size() - ROW_SUFFIX.size());
    return contains(SINGLE_POINT_FIELDS, field) ? field : std::string_view{};
}

// True when the card type stores `field` as one integer per card (an ID), not a list.
bool isScalarIdColumn(std::uint16_t type, std::string_view field)
{
    const StoredColumnCatalogue& catalogue = StoredColumnCatalogue::instance();
    return catalogue.contains(type, field) && catalogue.layoutOf(type, field).type == ColumnType::Int32;
}

// The cards of `rows` that are real (not NO_ROW), by card type: the rows of the table and the position in `rows`.
struct TypeGroup
{
    std::uint16_t type = 0;
    std::vector<std::uint32_t> rows;
    std::vector<std::uint32_t> positions;
};

std::vector<TypeGroup> groupByType(const CardRows& cards)
{
    std::map<std::uint16_t, TypeGroup> groups;
    for (std::size_t index = 0; index < cards.count(); ++index)
    {
        if (cards.rows[index] == NO_ROW)
            continue;
        TypeGroup& group = groups[cards.typeAt(index)];
        group.type = cards.typeAt(index);
        group.rows.push_back(cards.rows[index]);
        group.positions.push_back(static_cast<std::uint32_t>(index));
    }
    std::vector<TypeGroup> result;
    result.reserve(groups.size());
    for (auto& entry : groups)
        result.push_back(std::move(entry.second));
    return result;
}

std::size_t validCount(const std::vector<TypeGroup>& groups)
{
    std::size_t total = 0;
    for (const TypeGroup& group : groups)
        total += group.rows.size();
    return total;
}

// The validity of a column over `count` cards when only the cards of `groups` have a value.
QueryColumn withGroupValidity(QueryColumn column, const std::vector<TypeGroup>& groups, std::size_t count)
{
    if (validCount(groups) == count)
        return column;
    std::vector<std::uint8_t> flags(count, 0);
    for (const TypeGroup& group : groups)
    {
        for (const std::uint32_t position : group.positions)
            flags[position] = 1;
    }
    std::int64_t nullCount = 0;
    ColumnBuffer bits = validityFromFlags(flags, nullCount);
    return column.withValidity(std::move(bits), nullCount);
}

// The values of a type group, computed in one call when that works. When a card cannot be computed (a property or a
// grid point that does not exist: the analysis functions throw, naming the card) the cards are tried one at a time and
// the ones that fail get NaN: a query reports on a model, and one broken card must not hide the others.
using GroupComputation = std::function<std::vector<std::vector<double>>(const TypeGroup&)>;

std::vector<std::vector<double>> tolerant(const TypeGroup& group, std::size_t columns, const GroupComputation& compute)
{
    try
    {
        return compute(group);
    }
    catch (const base::OperationCancelled&)
    {
        throw;
    }
    catch (const base::OorError&)
    {
        std::vector<std::vector<double>> result(columns, std::vector<double>(group.rows.size(), NOT_A_NUMBER));
        for (std::size_t index = 0; index < group.rows.size(); ++index)
        {
            TypeGroup single;
            single.type = group.type;
            single.rows = {group.rows[index]};
            single.positions = {0};
            try
            {
                const std::vector<std::vector<double>> values = compute(single);
                for (std::size_t column = 0; column < columns; ++column)
                    result[column][index] = values[column][0];
            }
            catch (const base::OperationCancelled&)
            {
                throw;
            }
            catch (const base::OorError&)
            {
            }
        }
        return result;
    }
}

// The doubles of one family: one vector per column, for the cards of each type, put in place by position.
using DoubleProducer = std::function<std::vector<std::vector<double>>(const TypeGroup&)>;

std::vector<NamedColumn> assembleDoubles(const std::vector<std::string>& names, const CardRows& cards,
                                         const std::vector<TypeGroup>& groups, const DoubleProducer& producer)
{
    const std::size_t count = cards.count();
    std::vector<std::vector<double>> outputs;
    bool inPlace = groups.size() == 1 && groups[0].rows.size() == count;
    for (std::size_t group = 0; group < groups.size(); ++group)
    {
        std::vector<std::vector<double>> values = producer(groups[group]);
        if (inPlace)
        {
            outputs = std::move(values); // every card is of this type, in order: nothing to put in place
            break;
        }
        if (outputs.empty())
            outputs.assign(names.size(), std::vector<double>(count, NOT_A_NUMBER));
        for (std::size_t column = 0; column < names.size(); ++column)
        {
            for (std::size_t index = 0; index < groups[group].positions.size(); ++index)
                outputs[column][groups[group].positions[index]] = values[column][index];
        }
    }
    if (outputs.empty())
        outputs.assign(names.size(), std::vector<double>(count, NOT_A_NUMBER));
    std::vector<NamedColumn> result;
    for (std::size_t column = 0; column < names.size(); ++column)
    {
        result.push_back(NamedColumn{
            names[column], withGroupValidity(QueryColumn::primitive(std::move(outputs[column])), groups, count)});
    }
    return result;
}

std::vector<std::vector<double>> splitVectors(const std::vector<base::Vec3>& vectors)
{
    std::vector<std::vector<double>> parts(3, std::vector<double>(vectors.size()));
    for (std::size_t index = 0; index < vectors.size(); ++index)
    {
        parts[0][index] = vectors[index].x;
        parts[1][index] = vectors[index].y;
        parts[2][index] = vectors[index].z;
    }
    return parts;
}

// --- geometry
// ---------------------------------------------------------------------------------------------------------

std::vector<NamedColumn> computeGeometry(std::string_view column, model::Model& model, const CardRows& cards,
                                         ExecutionContext& context)
{
    using analysis::Quantity;
    Quantity quantity = Quantity::Area;
    std::vector<std::string> names{std::string(column)};
    if (column == "area")
        quantity = Quantity::Area;
    else if (column == "volume")
        quantity = Quantity::Volume;
    else if (column == "length")
        quantity = Quantity::Length;
    else if (column == "thickness")
        quantity = Quantity::Thickness;
    else if (column.starts_with("center"))
    {
        quantity = Quantity::Center;
        names = {"centerX", "centerY", "centerZ"};
    }
    else
    {
        quantity = Quantity::Normal;
        names = {"normalX", "normalY", "normalZ"};
    }
    const std::vector<TypeGroup> groups = groupByType(cards);
    computationCounters().geometryCalls.fetch_add(1);
    computationCounters().rowsComputed.fetch_add(validCount(groups));
    const GroupComputation compute = [&](const TypeGroup& group) -> std::vector<std::vector<double>>
    {
        context.checkCancelled();
        analysis::ElementArrays arrays =
            analysis::ElementGeometry::compute(model, group.type, group.rows, analysis::bit(quantity), context.pool());
        switch (quantity)
        {
        case Quantity::Center:
            return splitVectors(arrays.centers);
        case Quantity::Normal:
            return splitVectors(arrays.normals);
        case Quantity::Volume:
            return {std::move(arrays.volumes)};
        case Quantity::Length:
            return {std::move(arrays.lengths)};
        case Quantity::Thickness:
            return {std::move(arrays.thicknesses)};
        default:
            return {std::move(arrays.areas)};
        }
    };
    return assembleDoubles(names, cards, groups,
                           [&](const TypeGroup& group) { return tolerant(group, names.size(), compute); });
}

// --- mass
// -------------------------------------------------------------------------------------------------------------

std::vector<NamedColumn> computeMass(model::Model& model, const CardRows& cards, ExecutionContext& context,
                                     const analysis::MassOptions& options)
{
    const std::vector<TypeGroup> groups = groupByType(cards);
    computationCounters().massCalls.fetch_add(1);
    computationCounters().rowsComputed.fetch_add(validCount(groups));
    const std::vector<std::string> names{"mass", "structuralMass", "nonstructuralMass"};
    const GroupComputation compute = [&](const TypeGroup& group) -> std::vector<std::vector<double>>
    {
        context.checkCancelled();
        analysis::ElementMasses masses =
            analysis::MassCalculator::compute(model, group.type, group.rows, options, context.pool());
        return {std::move(masses.mass), std::move(masses.structural), std::move(masses.nonStructural)};
    };
    return assembleDoubles(names, cards, groups,
                           [&](const TypeGroup& group) { return tolerant(group, names.size(), compute); });
}

// --- quality
// ----------------------------------------------------------------------------------------------------------

std::vector<NamedColumn> computeQuality(model::Model& model, const CardRows& cards, ExecutionContext& context)
{
    const std::vector<TypeGroup> groups = groupByType(cards);
    computationCounters().qualityCalls.fetch_add(1);
    computationCounters().rowsComputed.fetch_add(validCount(groups));
    std::vector<std::string> names(std::begin(QUALITY_NAMES), std::end(QUALITY_NAMES));
    const GroupComputation compute = [&](const TypeGroup& group) -> std::vector<std::vector<double>>
    {
        context.checkCancelled();
        analysis::QualityArrays arrays = analysis::MeshQuality::compute(model, group.type, group.rows, context.pool());
        return {std::move(arrays.aspectRatio),    std::move(arrays.skew),        std::move(arrays.warp),
                std::move(arrays.taper),          std::move(arrays.minAngle),    std::move(arrays.maxAngle),
                std::move(arrays.jacobianRatio),  std::move(arrays.jacobianMin), std::move(arrays.minEdge),
                std::move(arrays.maxEdge),        std::move(arrays.collapse),    std::move(arrays.edgePointLengthRatio),
                std::move(arrays.edgePointAngle), std::move(arrays.faceWarp)};
    };
    return assembleDoubles(names, cards, groups,
                           [&](const TypeGroup& group) { return tolerant(group, names.size(), compute); });
}

// --- counts
// -----------------------------------------------------------------------------------------------------------

QueryColumn countsColumn(const std::vector<std::int64_t>& offsets, const std::vector<TypeGroup>& groups,
                         const std::vector<std::uint32_t>& positions, std::size_t count)
{
    std::vector<std::int64_t> counts(count, 0);
    for (std::size_t index = 0; index < positions.size(); ++index)
        counts[positions[index]] = offsets[index + 1] - offsets[index];
    return withGroupValidity(QueryColumn::primitive(std::move(counts)), groups, count);
}

std::vector<NamedColumn> computeAdjacency(std::string_view column, model::Model& model, const CardRows& cards,
                                          ExecutionContext& context)
{
    const std::vector<TypeGroup> groups = groupByType(cards);
    computationCounters().adjacencyCalls.fetch_add(1);
    computationCounters().rowsComputed.fetch_add(validCount(groups));
    analysis::AdjacencyKind kind = analysis::AdjacencyKind::Edge;
    if (column == "nodeNeighbourCount")
        kind = analysis::AdjacencyKind::Node;
    else if (column == "faceNeighbourCount")
        kind = analysis::AdjacencyKind::Face;
    std::vector<model::CardLocation> locations;
    std::vector<std::uint32_t> positions;
    for (std::size_t index = 0; index < cards.count(); ++index)
    {
        if (cards.rows[index] == NO_ROW)
            continue;
        locations.push_back(model::CardLocation{cards.typeAt(index), cards.rows[index]});
        positions.push_back(static_cast<std::uint32_t>(index));
    }
    context.checkCancelled();
    const analysis::AdjacencyCsr csr = analysis::MeshAdjacency::adjacency(model, locations, kind, {}, context.pool());
    std::vector<NamedColumn> result;
    result.push_back(NamedColumn{std::string(column), countsColumn(csr.offsets, groups, positions, cards.count())});
    return result;
}

std::vector<NamedColumn> computeNodeCount(model::Model& model, const CardRows& cards, ExecutionContext& context)
{
    const std::vector<TypeGroup> groups = groupByType(cards);
    computationCounters().adjacencyCalls.fetch_add(1);
    computationCounters().rowsComputed.fetch_add(validCount(groups));
    std::vector<base::EntityId> ids;
    std::vector<std::uint32_t> positions;
    for (std::size_t index = 0; index < cards.count(); ++index)
    {
        if (cards.rows[index] == NO_ROW)
            continue;
        ids.push_back(model.findTable(cards.typeAt(index))->idAt(cards.rows[index]));
        positions.push_back(static_cast<std::uint32_t>(index));
    }
    context.checkCancelled();
    const analysis::AdjacencyCsr csr = analysis::MeshAdjacency::nodeElements(model, ids, {}, context.pool());
    std::vector<NamedColumn> result;
    result.push_back(NamedColumn{"elementCount", countsColumn(csr.offsets, groups, positions, cards.count())});
    return result;
}

// --- positions of the grid points
// -------------------------------------------------------------------------------------

std::vector<NamedColumn> computePositions(model::Model& model, const CardRows& cards, ExecutionContext& context)
{
    const std::uint16_t gridType = model::typeIndexOf<model::NodeTable>();
    // Only a grid point has a position: the cards of the other types of the node ID space (a scalar point) get a null.
    std::vector<TypeGroup> groups = groupByType(cards);
    groups.erase(
        std::remove_if(groups.begin(), groups.end(), [&](const TypeGroup& group) { return group.type != gridType; }),
        groups.end());
    computationCounters().positionCalls.fetch_add(1);
    computationCounters().rowsComputed.fetch_add(validCount(groups));
    context.checkCancelled();
    analysis::BasicPositionCache& cache = analysis::BasicPositionCache::of(model, context.pool());
    const std::span<const std::array<double, 3>> positions = cache.positions();
    const std::size_t count = cards.count();
    std::vector<NamedColumn> result;
    if (cards.wholeTable && cards.single == gridType && positions.size() == count)
    {
        // The whole table: the three columns are cut out of the cache, and the (N, 3) column is the cache itself.
        std::vector<double> x(count);
        std::vector<double> y(count);
        std::vector<double> z(count);
        context.parallel(count, LIGHT_CHUNK,
                         [&](base::IndexRange range)
                         {
                             for (std::size_t row = range.begin; row < range.end; ++row)
                             {
                                 x[row] = positions[row][0];
                                 y[row] = positions[row][1];
                                 z[row] = positions[row][2];
                             }
                         });
        result.push_back(NamedColumn{"x", QueryColumn::primitive(std::move(x))});
        result.push_back(NamedColumn{"y", QueryColumn::primitive(std::move(y))});
        result.push_back(NamedColumn{"z", QueryColumn::primitive(std::move(z))});
        const auto buffer = cache.shareBuffer();
        QueryColumn values = QueryColumn::fromBuffer(ColumnType::Float64, static_cast<std::int64_t>(count) * 3,
                                                     ColumnBuffer::share(buffer));
        result.push_back(
            NamedColumn{"xyzGlobal", QueryColumn::fixedList(std::move(values), 3, static_cast<std::int64_t>(count))});
        return result;
    }
    std::vector<double> x(count, NOT_A_NUMBER);
    std::vector<double> y(count, NOT_A_NUMBER);
    std::vector<double> z(count, NOT_A_NUMBER);
    std::vector<double> xyz(count * 3, NOT_A_NUMBER);
    context.parallel(count, LIGHT_CHUNK,
                     [&](base::IndexRange range)
                     {
                         for (std::size_t index = range.begin; index < range.end; ++index)
                         {
                             const std::uint32_t row = cards.rows[index];
                             if (row == NO_ROW || cards.typeAt(index) != gridType)
                                 continue;
                             x[index] = xyz[3 * index] = positions[row][0];
                             y[index] = xyz[3 * index + 1] = positions[row][1];
                             z[index] = xyz[3 * index + 2] = positions[row][2];
                         }
                     });
    result.push_back(NamedColumn{"x", withGroupValidity(QueryColumn::primitive(std::move(x)), groups, count)});
    result.push_back(NamedColumn{"y", withGroupValidity(QueryColumn::primitive(std::move(y)), groups, count)});
    result.push_back(NamedColumn{"z", withGroupValidity(QueryColumn::primitive(std::move(z)), groups, count)});
    QueryColumn values = QueryColumn::primitive(std::move(xyz));
    result.push_back(NamedColumn{
        "xyzGlobal", withGroupValidity(QueryColumn::fixedList(std::move(values), 3, static_cast<std::int64_t>(count)),
                                       groups, count)});
    return result;
}

// --- resolved rows
// ----------------------------------------------------------------------------------------------------

// The row of the grid point `id` in the exported node table (`model.nodes.table()`, by ID), or -1 when the ID is not a
// grid point (a scalar point, a missing point, a blank field). `tableRows` maps the storage rows of the Node table to
// the rows of that table (ExportedRowOrder).
std::int32_t nodeRowOf(const model::Model& model, std::uint16_t gridType, const std::vector<std::int32_t>& tableRows,
                       std::int64_t id) noexcept
{
    const auto entityId = base::toEntityId(id);
    if (!entityId)
        return -1;
    const auto location = model.locate(base::IdSpace::Node, *entityId);
    return location && location->table == gridType && location->row < tableRows.size() ? tableRows[location->row] : -1;
}

std::vector<NamedColumn> computeResolvedNodes(model::Model& model, const CardRows& cards, ExecutionContext& context)
{
    const std::vector<TypeGroup> groups = groupByType(cards);
    computationCounters().resolvedCalls.fetch_add(1);
    computationCounters().rowsComputed.fetch_add(validCount(groups));
    const std::uint16_t gridType = model::typeIndexOf<model::NodeTable>();
    const std::vector<std::int32_t> gridRows = ExportedRowOrder::tableRows(model, gridType);
    const std::size_t count = cards.count();

    // The `nodes` column of each type (a type without one has no node to resolve).
    std::map<std::uint16_t, QueryColumn> nodeColumns;
    std::int32_t fixedWidth = -1; // -1: not decided, 0: not fixed
    for (const TypeGroup& group : groups)
    {
        if (!StoredColumnCatalogue::instance().contains(group.type, "nodes"))
        {
            fixedWidth = 0;
            continue;
        }
        QueryColumn column = StoredColumnCatalogue::instance().fetch(model, group.type, "nodes");
        if (column.type() != ColumnType::FixedList || !isInteger(column.child()->type()) ||
            (fixedWidth > 0 && column.width() != fixedWidth))
            fixedWidth = 0;
        else if (fixedWidth == -1)
            fixedWidth = column.width();
        nodeColumns.emplace(group.type, std::move(column));
    }
    std::vector<std::uint8_t> present(count, 0);
    for (const TypeGroup& group : groups)
    {
        for (const std::uint32_t position : group.positions)
            present[position] = nodeColumns.count(group.type) != 0 ? 1 : 0;
    }
    const auto listOf = [&](std::size_t index, std::int64_t& first, std::int64_t& length) -> const QueryColumn*
    {
        if (!present[index])
        {
            first = 0;
            length = 0;
            return nullptr;
        }
        const QueryColumn& column = nodeColumns.at(cards.typeAt(index));
        first = column.listBegin(cards.rows[index]);
        length = column.listLength(cards.rows[index]);
        return column.child().get();
    };
    const auto idAt = [](const QueryColumn& values, std::int64_t position) -> std::int64_t
    {
        switch (values.type())
        {
        case ColumnType::Int32:
            return values.as<std::int32_t>()[static_cast<std::size_t>(position)];
        case ColumnType::UInt32:
            return values.as<std::uint32_t>()[static_cast<std::size_t>(position)];
        default:
            return values.as<std::int64_t>()[static_cast<std::size_t>(position)];
        }
    };

    std::vector<NamedColumn> result;
    if (fixedWidth > 0)
    {
        std::vector<std::int32_t> rows(count * static_cast<std::size_t>(fixedWidth), -1);
        context.parallel(count, LIGHT_CHUNK,
                         [&](base::IndexRange range)
                         {
                             for (std::size_t index = range.begin; index < range.end; ++index)
                             {
                                 std::int64_t first = 0;
                                 std::int64_t length = 0;
                                 const QueryColumn* values = listOf(index, first, length);
                                 for (std::int64_t at = 0; at < length && values != nullptr; ++at)
                                     rows[index * static_cast<std::size_t>(fixedWidth) + static_cast<std::size_t>(at)] =
                                         nodeRowOf(model, gridType, gridRows, idAt(*values, first + at));
                             }
                         });
        QueryColumn values = QueryColumn::primitive(std::move(rows));
        QueryColumn column = QueryColumn::fixedList(std::move(values), fixedWidth, static_cast<std::int64_t>(count));
        result.push_back(NamedColumn{"nodesRows", withGroupValidity(std::move(column), groups, count)});
        return result;
    }
    std::vector<std::int64_t> offsets(count + 1, 0);
    for (std::size_t index = 0; index < count; ++index)
    {
        std::int64_t first = 0;
        std::int64_t length = 0;
        (void)listOf(index, first, length);
        offsets[index + 1] = offsets[index] + length;
    }
    std::vector<std::int32_t> rows(static_cast<std::size_t>(offsets[count]), -1);
    context.parallel(count, LIGHT_CHUNK,
                     [&](base::IndexRange range)
                     {
                         for (std::size_t index = range.begin; index < range.end; ++index)
                         {
                             std::int64_t first = 0;
                             std::int64_t length = 0;
                             const QueryColumn* values = listOf(index, first, length);
                             for (std::int64_t at = 0; at < length && values != nullptr; ++at)
                                 rows[static_cast<std::size_t>(offsets[index] + at)] =
                                     nodeRowOf(model, gridType, gridRows, idAt(*values, first + at));
                         }
                     });
    QueryColumn column = QueryColumn::list(ColumnBuffer::adopt(std::move(offsets)),
                                           QueryColumn::primitive(std::move(rows)), static_cast<std::int64_t>(count));
    result.push_back(NamedColumn{"nodesRows", withGroupValidity(std::move(column), groups, count)});
    return result;
}

std::vector<NamedColumn> computeResolvedProperty(model::Model& model, const CardRows& cards, ExecutionContext& context)
{
    const std::vector<TypeGroup> groups = groupByType(cards);
    computationCounters().resolvedCalls.fetch_add(1);
    computationCounters().rowsComputed.fetch_add(validCount(groups));
    const std::size_t count = cards.count();
    std::map<std::uint16_t, QueryColumn> propertyColumns;
    for (const TypeGroup& group : groups)
    {
        if (StoredColumnCatalogue::instance().contains(group.type, "propId"))
            propertyColumns.emplace(group.type, StoredColumnCatalogue::instance().fetch(model, group.type, "propId"));
    }
    std::vector<std::int32_t> rows(count, -1);
    std::vector<std::uint16_t> propertyTypes(count, 0); // 0: none (the table of unknown cards is never a property)
    context.parallel(count, LIGHT_CHUNK,
                     [&](base::IndexRange range)
                     {
                         for (std::size_t index = range.begin; index < range.end; ++index)
                         {
                             if (cards.rows[index] == NO_ROW)
                                 continue;
                             const auto found = propertyColumns.find(cards.typeAt(index));
                             if (found == propertyColumns.end() || found->second.type() != ColumnType::Int32)
                                 continue;
                             const std::int32_t pid = found->second.as<std::int32_t>()[cards.rows[index]];
                             const auto id = base::toEntityId(pid);
                             if (!id)
                                 continue;
                             const auto location = model.locate(base::IdSpace::Property, *id);
                             if (!location)
                                 continue;
                             rows[index] = static_cast<std::int32_t>(location->row);
                             propertyTypes[index] = location->table;
                         }
                     });
    // The names of the property types met, in the order of the registry.
    std::vector<std::uint16_t> met;
    for (const std::uint16_t type : propertyTypes)
    {
        if (type != 0 && std::find(met.begin(), met.end(), type) == met.end())
            met.push_back(type);
    }
    std::sort(met.begin(), met.end());
    std::vector<std::string> dictionary;
    for (const std::uint16_t type : met)
        dictionary.emplace_back(model::CardRegistry::instance().entry(type).info.name);
    // The storage row found above becomes the row of the exported table of the property type (`model.pshell.table()`,
    // sorted by ID), whatever the order of the property cards in the deck.
    std::map<std::uint16_t, std::vector<std::int32_t>> tableRowsOf;
    for (const std::uint16_t type : met)
        tableRowsOf.emplace(type, ExportedRowOrder::tableRows(model, type));
    for (std::size_t index = 0; index < count; ++index)
    {
        if (propertyTypes[index] != 0)
            rows[index] = tableRowsOf.at(propertyTypes[index])[static_cast<std::size_t>(rows[index])];
    }
    std::vector<std::int32_t> indices(count, 0);
    std::vector<std::uint8_t> found(count, 0);
    for (std::size_t index = 0; index < count; ++index)
    {
        if (propertyTypes[index] == 0)
            continue;
        indices[index] =
            static_cast<std::int32_t>(std::lower_bound(met.begin(), met.end(), propertyTypes[index]) - met.begin());
        found[index] = 1;
    }
    std::int64_t nullCount = 0;
    ColumnBuffer bits = validityFromFlags(found, nullCount);
    QueryColumn typeColumn = QueryColumn::dictionary(std::move(indices), dictionary);
    if (nullCount > 0)
        typeColumn = typeColumn.withValidity(std::move(bits), nullCount);
    std::vector<NamedColumn> result;
    result.push_back(
        NamedColumn{"propIdRow", withGroupValidity(QueryColumn::primitive(std::move(rows)), groups, count)});
    result.push_back(NamedColumn{"propType", std::move(typeColumn)});
    return result;
}

// The `<field>Row` column of a single grid point field: the row of the point in the exported node table, null where
// the card type has no such field, the field is blank, or it names a point that is not a grid point of the model (a
// scalar point, a missing one).
std::vector<NamedColumn> computeResolvedPoint(std::string_view column, model::Model& model, const CardRows& cards,
                                              ExecutionContext& context)
{
    const std::vector<TypeGroup> groups = groupByType(cards);
    computationCounters().resolvedCalls.fetch_add(1);
    computationCounters().rowsComputed.fetch_add(validCount(groups));
    const std::string_view field = pointFieldOf(column);
    const std::uint16_t gridType = model::typeIndexOf<model::NodeTable>();
    const std::vector<std::int32_t> gridRows = ExportedRowOrder::tableRows(model, gridType);
    const std::size_t count = cards.count();
    std::map<std::uint16_t, QueryColumn> pointColumns;
    for (const TypeGroup& group : groups)
    {
        if (isScalarIdColumn(group.type, field))
            pointColumns.emplace(group.type, StoredColumnCatalogue::instance().fetch(model, group.type, field));
    }
    std::vector<std::int32_t> rows(count, -1);
    context.parallel(count, LIGHT_CHUNK,
                     [&](base::IndexRange range)
                     {
                         for (std::size_t index = range.begin; index < range.end; ++index)
                         {
                             if (cards.rows[index] == NO_ROW)
                                 continue;
                             const auto found = pointColumns.find(cards.typeAt(index));
                             if (found == pointColumns.end())
                                 continue;
                             const std::int32_t id = found->second.as<std::int32_t>()[cards.rows[index]];
                             if (id > 0)
                                 rows[index] = nodeRowOf(model, gridType, gridRows, id);
                         }
                     });
    std::vector<std::uint8_t> valid(count, 0);
    for (std::size_t index = 0; index < count; ++index)
        valid[index] = rows[index] >= 0 ? 1 : 0;
    std::int64_t nullCount = 0;
    ColumnBuffer bits = validityFromFlags(valid, nullCount);
    QueryColumn values = QueryColumn::primitive(std::move(rows));
    if (nullCount > 0)
        values = values.withValidity(std::move(bits), nullCount);
    std::vector<NamedColumn> result;
    result.push_back(NamedColumn{std::string(column), std::move(values)});
    return result;
}

// True for card types that are all points of the node ID space: grid points and scalar points.
bool allNodes(std::span<const std::uint16_t> types)
{
    if (types.empty())
        return false;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    return std::all_of(types.begin(), types.end(),
                       [&](std::uint16_t type)
                       {
                           const model::CardCategory category = registry.entry(type).info.category;
                           return category == model::CardCategory::Node || category == model::CardCategory::Point;
                       });
}

bool allElements(std::span<const std::uint16_t> types)
{
    if (types.empty())
        return false;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    return std::all_of(types.begin(), types.end(), [&](std::uint16_t type)
                       { return registry.entry(type).info.category == model::CardCategory::Element; });
}

} // namespace

std::optional<Family> familyOf(std::string_view column)
{
    if (contains(GEOMETRY_NAMES, column))
        return Family::Geometry;
    if (contains(MASS_NAMES, column))
        return Family::Mass;
    if (contains(QUALITY_NAMES, column))
        return Family::Quality;
    if (contains(ADJACENCY_NAMES, column))
        return Family::Adjacency;
    if (contains(RESOLVED_NODE_NAMES, column))
        return Family::ResolvedNodes;
    if (contains(RESOLVED_PROPERTY_NAMES, column))
        return Family::ResolvedProperty;
    if (!pointFieldOf(column).empty())
        return Family::ResolvedPoint;
    if (contains(POSITION_NAMES, column))
        return Family::NodePosition;
    if (contains(COUNT_NAMES, column))
        return Family::NodeCount;
    return std::nullopt;
}

std::vector<std::string> computedNamesFor(std::span<const std::uint16_t> types,
                                          const std::function<bool(std::uint16_t, std::string_view)>& hasStored)
{
    std::vector<std::string> names;
    if (allElements(types))
    {
        for (const char* name : GEOMETRY_NAMES)
            names.emplace_back(name);
        for (const char* name : MASS_NAMES)
            names.emplace_back(name);
        for (const char* name : QUALITY_NAMES)
            names.emplace_back(name);
        for (const char* name : ADJACENCY_NAMES)
            names.emplace_back(name);
        const bool nodes =
            std::any_of(types.begin(), types.end(), [&](std::uint16_t type) { return hasStored(type, "nodes"); });
        const bool property =
            std::any_of(types.begin(), types.end(), [&](std::uint16_t type) { return hasStored(type, "propId"); });
        if (nodes)
            names.emplace_back("nodesRows");
        for (const char* field : SINGLE_POINT_FIELDS)
        {
            if (std::any_of(types.begin(), types.end(), [&](std::uint16_t type)
                            { return hasStored(type, field) && isScalarIdColumn(type, field); }))
                names.push_back(std::string(field) + std::string(ROW_SUFFIX));
        }
        if (property)
        {
            names.emplace_back("propIdRow");
            names.emplace_back("propType");
        }
    }
    else if (allNodes(types))
    {
        // the positions of the grid points (the other points of the node ID space have none), the elements of any node
        if (std::find(types.begin(), types.end(), model::typeIndexOf<model::NodeTable>()) != types.end())
        {
            for (const char* name : POSITION_NAMES)
                names.emplace_back(name);
        }
        names.emplace_back("elementCount");
    }
    return names;
}

std::vector<NamedColumn> computeColumns(std::string_view column, model::Model& model, const CardRows& cards,
                                        ExecutionContext& context, const analysis::MassOptions& massOptions)
{
    const std::optional<Family> family = familyOf(column);
    if (!family)
        throw QueryError("'" + std::string(column) + "' is not a computed column");
    switch (*family)
    {
    case Family::Geometry:
        return computeGeometry(column, model, cards, context);
    case Family::Mass:
        return computeMass(model, cards, context, massOptions);
    case Family::Quality:
        return computeQuality(model, cards, context);
    case Family::Adjacency:
        return computeAdjacency(column, model, cards, context);
    case Family::ResolvedNodes:
        return computeResolvedNodes(model, cards, context);
    case Family::ResolvedProperty:
        return computeResolvedProperty(model, cards, context);
    case Family::ResolvedPoint:
        return computeResolvedPoint(column, model, cards, context);
    case Family::NodePosition:
        return computePositions(model, cards, context);
    case Family::NodeCount:
        return computeNodeCount(model, cards, context);
    }
    throw QueryError("unknown computed column");
}

} // namespace detail

} // namespace oor::query
