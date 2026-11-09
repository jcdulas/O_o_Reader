#include "oor/analysis/geometry/DisplayMesh.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/adjacency/MeshAdjacency.hpp"
#include "oor/analysis/components/ElementRoles.hpp"
#include "oor/analysis/connections/FastenerPiercings.hpp"
#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/analysis/mass/SectionFormulas.hpp"
#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/beam2/Beam2Table.hpp"
#include "oor/model/cards/elements/massDofProperty/MassDofPropertyTable.hpp"
#include "oor/model/cards/elements/massDofInline/MassDofInlineTable.hpp"
#include "oor/model/cards/elements/pointMassMatrix/PointMassMatrixTable.hpp"
#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaTable.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingTable.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"
#include "oor/model/cards/elements/tube2/Tube2Table.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/properties/barShapeProp/BarShapePropTable.hpp"
#include "oor/model/cards/properties/beamShapeProp/BeamShapePropTable.hpp"
#include "oor/model/cards/properties/massProp/MassPropTable.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <string_view>
#include <utility>

namespace oor::analysis
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The connection kinds, indices in connectionKinds().
constexpr std::uint8_t RIGID = 0;
constexpr std::uint8_t RIGID_INTERPOLATION = 1;
constexpr std::uint8_t SPRING = 2;
constexpr std::uint8_t DAMPER = 3;
constexpr std::uint8_t BUSH = 4;
constexpr std::uint8_t FASTENER = 5;
constexpr std::uint8_t GAP = 6;

// Puts the rows of parallel arrays in the order of their element IDs (stable: the rows of one element keep their
// order); `widths` the values a row of each array holds.
template <typename... Arrays>
void sortRows(std::vector<std::int32_t>& eids, std::pair<Arrays*, std::size_t>... arrays)
{
    if (std::is_sorted(eids.begin(), eids.end()))
        return;
    std::vector<std::size_t> order(eids.size());
    for (std::size_t index = 0; index < order.size(); ++index)
        order[index] = index;
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t left, std::size_t right) { return eids[left] < eids[right]; });
    const auto permute = [&](auto& values, std::size_t width)
    {
        auto sorted = values;
        for (std::size_t index = 0; index < order.size(); ++index)
            for (std::size_t k = 0; k < width; ++k)
                sorted[index * width + k] = values[order[index] * width + k];
        values = std::move(sorted);
    };
    (permute(*arrays.first, arrays.second), ...);
    permute(eids, 1);
}

bool finite(const base::Vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

// The grid points of the model: their IDs by increasing ID, the storage row and the position of each.
struct Grids
{
    std::vector<base::EntityId> ids;
    std::vector<base::Vec3> positions;

    // The row of a grid point (in ID order), -1 when the ID is no grid point.
    [[nodiscard]] std::int64_t row(base::EntityId id) const noexcept
    {
        const auto found = std::lower_bound(ids.begin(), ids.end(), id);
        return found != ids.end() && *found == id ? static_cast<std::int64_t>(found - ids.begin()) : -1;
    }
};

Grids gridsOf(const model::Model& model, base::ThreadPool* pool)
{
    Grids grids;
    const model::NodeTable* table = model.findTable<model::NodeTable>();
    if (table == nullptr)
        return grids;
    const BasicPositionCache& cache = BasicPositionCache::of(model, pool);
    std::vector<std::pair<base::EntityId, std::uint32_t>> rows;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        if (table->rows().isAlive(row))
            rows.emplace_back(table->idAt(row), row);
    std::sort(rows.begin(), rows.end());
    grids.ids.reserve(rows.size());
    grids.positions.reserve(rows.size());
    for (const auto& [id, row] : rows)
    {
        grids.ids.push_back(id);
        grids.positions.push_back(cache.position(row));
    }
    return grids;
}

// The live rows of a table, with the elements asked for only.
std::vector<std::uint32_t> rowsOf(const model::CardTable& table, const std::vector<base::EntityId>* wanted)
{
    std::vector<std::uint32_t> rows;
    for (std::uint32_t row = 0; row < table.rows().rowCount(); ++row)
    {
        if (!table.rows().isAlive(row))
            continue;
        if (wanted != nullptr && !std::binary_search(wanted->begin(), wanted->end(), table.idAt(row)))
            continue;
        rows.push_back(row);
    }
    std::sort(rows.begin(), rows.end(),
              [&](std::uint32_t left, std::uint32_t right) { return table.idAt(left) < table.idAt(right); });
    return rows;
}

void push(std::vector<double>& out, const base::Vec3& point)
{
    out.insert(out.end(), {point.x, point.y, point.z});
}

// The offset ZOFFS of a shell card (0 for one that has none, and for a blank).
double offsetOf(const model::Model& model, std::uint16_t type, std::uint32_t row)
{
    double value = 0.0;
    const auto read = [&]<typename TableT>(const TableT*)
    {
        if (type == model::typeIndexOf<TableT>())
            value = model.findTable<TableT>()->card(row).offset;
    };
    read(static_cast<const model::ShellQuad4Table*>(nullptr));
    read(static_cast<const model::ShellTria3Table*>(nullptr));
    read(static_cast<const model::ShellQuad8Table*>(nullptr));
    read(static_cast<const model::ShellTria6Table*>(nullptr));
    read(static_cast<const model::ShellQuad4DrillingTable*>(nullptr));
    read(static_cast<const model::ShellTria3DrillingTable*>(nullptr));
    return std::isfinite(value) ? value : 0.0;
}

// The triangles of a shell over its slots: (corner or mid-side slot) triples.
using Triangles = std::vector<std::array<std::uint8_t, 3>>;

const Triangles& trianglesOf(std::size_t corners, bool midSides)
{
    // a triangle: corners 0 1 2, mid-sides 3 (0-1), 4 (1-2), 5 (2-0); a quadrilateral: corners 0 1 2 3, mid-sides 4
    // (0-1), 5 (1-2), 6 (2-3), 7 (3-0)
    static const Triangles TRIANGLE = {{0, 1, 2}};
    static const Triangles TRIANGLE6 = {{0, 3, 5}, {3, 1, 4}, {5, 4, 2}, {3, 4, 5}};
    static const Triangles QUAD = {{0, 1, 2}, {0, 2, 3}};
    static const Triangles QUAD8 = {{0, 4, 7}, {4, 1, 5}, {5, 2, 6}, {6, 3, 7}, {4, 5, 6}, {4, 6, 7}};
    if (corners == 3)
        return midSides ? TRIANGLE6 : TRIANGLE;
    return midSides ? QUAD8 : QUAD;
}

std::string_view nameOf(std::uint16_t type)
{
    return model::CardRegistry::instance().entry(type).info.name;
}

bool isAnyName(std::string_view name, std::initializer_list<std::string_view> names)
{
    return std::find(names.begin(), names.end(), name) != names.end();
}

} // namespace

const std::vector<std::string>& connectionKinds()
{
    static const std::vector<std::string> KINDS = {"rigid", "rigidInterpolation", "spring", "damper",
                                                   "bush",  "fastener",           "gap"};
    return KINDS;
}

DisplayMesh buildDisplayMesh(const model::Model& model, const DisplayMeshOptions& options, base::ThreadPool* pool)
{
    DisplayMesh mesh;
    const Grids grids = gridsOf(model, pool);
    mesh.gridPositions.reserve(3 * grids.positions.size());
    for (const base::Vec3& position : grids.positions)
        push(mesh.gridPositions, position);
    std::vector<base::EntityId> wantedIds;
    if (options.elements)
    {
        wantedIds = *options.elements;
        std::sort(wantedIds.begin(), wantedIds.end());
    }
    const std::vector<base::EntityId>* wanted = options.elements ? &wantedIds : nullptr;
    const auto isWanted = [&](base::EntityId id)
    { return wanted == nullptr || std::binary_search(wanted->begin(), wanted->end(), id); };

    const model::CardRegistry& registry = model::CardRegistry::instance();
    std::vector<base::EntityId> slots;
    std::set<base::EntityId> sectionPids;

    // ---- the shells, by card type, the IDs increasing within a type, then all of them by ID
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        if (registry.entry(type).info.category != model::CardCategory::Element)
            continue;
        const ElementShape shape = ElementTopology::shapeOf(type);
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr || dimensionOf(shape) != 2)
            continue;
        const std::size_t corners = shape == ElementShape::Triangle ? 3 : 4;
        const bool alternating = ElementTopology::alternatingCorners(type);
        for (const std::uint32_t row : rowsOf(*table, wanted))
        {
            slots.clear();
            ElementTopology::slotsOf(*table, type, row, slots);
            // the place of each corner and mid-side point on the card
            std::array<std::int32_t, 8> place{};
            std::array<std::int64_t, 8> gridRow{};
            std::array<base::Vec3, 8> point{};
            bool complete = slots.size() >= (alternating ? 2 * corners : corners);
            for (std::size_t corner = 0; complete && corner < corners; ++corner)
                place[corner] = static_cast<std::int32_t>(alternating ? 2 * corner : corner);
            bool midSides = complete && slots.size() >= 2 * corners;
            for (std::size_t side = 0; midSides && side < corners; ++side)
            {
                place[corners + side] = static_cast<std::int32_t>(alternating ? 2 * side + 1 : corners + side);
                midSides = slots[static_cast<std::size_t>(place[corners + side])] != 0;
            }
            const std::size_t used = midSides ? 2 * corners : corners;
            for (std::size_t index = 0; complete && index < used; ++index)
            {
                gridRow[index] = grids.row(slots[static_cast<std::size_t>(place[index])]);
                complete = gridRow[index] >= 0;
                if (complete)
                    point[index] = grids.positions[static_cast<std::size_t>(gridRow[index])];
            }
            if (!complete)
                continue;
            if (options.zoffs)
            {
                const double offset = offsetOf(model, type, row);
                if (offset != 0.0)
                {
                    const base::Vec3 area = corners == 3 ? base::cross(point[1] - point[0], point[2] - point[0])
                                                         : base::cross(point[2] - point[0], point[3] - point[1]);
                    const base::Vec3 normal = base::normalized(area);
                    for (std::size_t index = 0; index < used; ++index)
                        point[index] = point[index] + offset * normal;
                }
            }
            const base::EntityId eid = table->idAt(row);
            for (const auto& triangle : trianglesOf(corners, midSides))
            {
                mesh.triangleEid.push_back(static_cast<std::int32_t>(eid));
                mesh.triangleFace.push_back(-1);
                for (const std::uint8_t vertex : triangle)
                {
                    push(mesh.triangleVertices, point[vertex]);
                    mesh.vertexGridRow.push_back(gridRow[vertex]);
                    mesh.vertexLocalIndex.push_back(place[vertex] + 1);
                }
            }
        }
    }
    // the shells of all the types in the order of their IDs (each type's are in order already)
    sortRows(mesh.triangleEid, std::pair{&mesh.triangleFace, std::size_t{1}},
             std::pair{&mesh.triangleVertices, std::size_t{9}}, std::pair{&mesh.vertexGridRow, std::size_t{3}},
             std::pair{&mesh.vertexLocalIndex, std::size_t{3}});

    // ---- the free faces of the solids
    {
        const FreeFaces faces = MeshAdjacency::freeFaces(model, pool);
        std::vector<std::size_t> order(faces.elements.size());
        for (std::size_t index = 0; index < order.size(); ++index)
            order[index] = index;
        std::stable_sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right)
                         { return faces.elements[left] < faces.elements[right]; });
        std::vector<std::array<base::EntityId, 4>> oriented;
        for (const std::size_t index : order)
        {
            const base::EntityId eid = faces.elements[index];
            if (!isWanted(eid))
                continue;
            const auto location = model.locate(base::IdSpace::Element, eid);
            if (!location)
                continue;
            const model::CardTable& table = *model.findTable(location->table);
            slots.clear();
            ElementTopology::slotsOf(table, location->table, location->row, slots);
            oriented.clear();
            ElementTopology::orientedFacesOf(table, location->table, location->row, oriented);
            const std::array<base::EntityId, 4>& face = faces.nodes[index];
            const std::size_t count = face[3] == 0 ? 3 : 4;
            // the face of the element: the template face with the same corners
            std::array<base::EntityId, 4> key = face;
            std::sort(key.begin(), key.end());
            std::int32_t faceIndex = -1;
            for (std::size_t candidate = 0; candidate < oriented.size() && faceIndex < 0; ++candidate)
            {
                std::array<base::EntityId, 4> other = oriented[candidate];
                std::sort(other.begin(), other.end());
                if (other == key)
                    faceIndex = static_cast<std::int32_t>(candidate + 1);
            }
            std::array<std::int64_t, 4> rows{};
            std::array<std::int32_t, 4> places{};
            bool complete = true;
            for (std::size_t corner = 0; corner < count && complete; ++corner)
            {
                rows[corner] = grids.row(face[corner]);
                complete = rows[corner] >= 0;
                const auto slot = std::find(slots.begin(), slots.end(), face[corner]);
                places[corner] = slot == slots.end() ? 0 : static_cast<std::int32_t>(slot - slots.begin()) + 1;
            }
            if (!complete)
                continue;
            const std::array<std::array<std::uint8_t, 3>, 2> split = {{{0, 1, 2}, {0, 2, 3}}};
            for (std::size_t part = 0; part < (count == 4 ? 2U : 1U); ++part)
            {
                mesh.triangleEid.push_back(static_cast<std::int32_t>(eid));
                mesh.triangleFace.push_back(faceIndex);
                for (const std::uint8_t corner : split[part])
                {
                    push(mesh.triangleVertices, grids.positions[static_cast<std::size_t>(rows[corner])]);
                    mesh.vertexGridRow.push_back(rows[corner]);
                    mesh.vertexLocalIndex.push_back(places[corner]);
                }
            }
        }
    }

    // ---- the 1D elements, the connections and the masses, by card type
    std::shared_ptr<const FastenerPiercings> piercings;
    const auto addSegment = [&](std::vector<double>& ends, std::vector<std::int64_t>& rows, const base::Vec3& first,
                                const base::Vec3& second, std::int64_t firstRow, std::int64_t secondRow)
    {
        push(ends, first);
        push(ends, second);
        rows.push_back(firstRow);
        rows.push_back(secondRow);
    };
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        if (registry.entry(type).info.category != model::CardCategory::Element)
            continue;
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr)
            continue;
        const std::string_view name = nameOf(type);
        const ElementRole role = roleOfType(type);
        const bool line = isAnyName(
            name, {"Bar2", "Beam2", "Rod2", "Rod2InlineSection", "Tube2", "CurvedBeam2", "Beam3"});
        const bool massCard = isAnyName(name, {"PointMassInertia", "PointMassMatrix", "MassDofProperty", "MassDofInline"});
        std::uint8_t kind = RIGID;
        bool connection = role == ElementRole::Rigid;
        if (role == ElementRole::Rigid)
            kind = name == "RigidInterpolation" ? RIGID_INTERPOLATION : RIGID;
        else if (isAnyName(name, {"Bushing", "Bushing1D", "Bushing2D"}))
            connection = true, kind = BUSH;
        else if (name == "Gap2")
            connection = true, kind = GAP;
        else if (isAnyName(name, {"SpringDofProperty", "SpringDofInline", "SpringDofPropertyExplicit", "SpringDofInlineExplicit",
                                  "SpringOffsetExplicit"}))
            connection = true, kind = SPRING;
        else if (isAnyName(name, {"DamperDofProperty", "DamperDofInline", "DamperDofPropertyExplicit", "DamperDofInlineExplicit",
                                  "ViscousDamper2"}))
            connection = true, kind = DAMPER;
        else if (isAnyName(name, {"Fastener", "Weld"}))
            connection = true, kind = FASTENER;
        if (!line && !massCard && !connection)
            continue;
        const std::vector<std::uint32_t> rows = rowsOf(*table, wanted);
        if (rows.empty())
            continue;
        if (line)
        {
            const bool offsets = name == "Bar2" || name == "Beam2";
            ElementArrays geometry;
            if (offsets)
                geometry = ElementGeometry::compute(
                    model, type, rows, bit(Quantity::Center) | bit(Quantity::Length) | bit(Quantity::Frame), pool);
            for (std::size_t index = 0; index < rows.size(); ++index)
            {
                const std::uint32_t row = rows[index];
                slots.clear();
                ElementTopology::slotsOf(*table, type, row, slots);
                if (slots.size() < 2)
                    continue;
                const std::int64_t a = grids.row(slots[0]);
                const std::int64_t b = grids.row(slots[1]);
                if (a < 0 || b < 0)
                    continue;
                const base::EntityId eid = table->idAt(row);
                base::EntityId pid = 0;
                if (name == "Bar2")
                    pid = model.findTable<model::Bar2Table>()->card(row).propId;
                else if (name == "Beam2")
                    pid = model.findTable<model::Beam2Table>()->card(row).propId;
                else if (name == "Rod2")
                    pid = model.findTable<model::Rod2Table>()->card(row).propId;
                else if (name == "Tube2")
                    pid = model.findTable<model::Tube2Table>()->card(row).propId;
                base::Vec3 first = grids.positions[static_cast<std::size_t>(a)];
                base::Vec3 second = grids.positions[static_cast<std::size_t>(b)];
                if (offsets)
                {
                    const base::Vec3 axis = geometry.frames[index].column(0);
                    const double half = geometry.lengths[index] / 2.0;
                    if (finite(axis) && std::isfinite(half) && finite(geometry.centers[index]))
                    {
                        first = geometry.centers[index] - half * axis;
                        second = geometry.centers[index] + half * axis;
                    }
                }
                const auto addLine =
                    [&](const base::Vec3& one, const base::Vec3& two, std::int64_t oneRow, std::int64_t twoRow)
                {
                    addSegment(mesh.segmentEnds, mesh.segmentGridRows, one, two, oneRow, twoRow);
                    mesh.segmentEid.push_back(static_cast<std::int32_t>(eid));
                    mesh.segmentPid.push_back(static_cast<std::int32_t>(pid));
                };
                // Beam3: GA, GB, GO, GC; with GC, the two halves of its axis through it
                if (name == "Beam3" && slots.size() >= 4 && slots[3] != 0 && grids.row(slots[3]) >= 0)
                {
                    const std::int64_t c = grids.row(slots[3]);
                    const base::Vec3 middle = grids.positions[static_cast<std::size_t>(c)];
                    addLine(first, middle, a, c);
                    addLine(middle, second, c, b);
                }
                else
                    addLine(first, second, a, b);
                if (pid != 0)
                    sectionPids.insert(pid);
            }
            continue;
        }
        if (massCard)
        {
            const ElementArrays geometry = ElementGeometry::compute(model, type, rows, bit(Quantity::Center), pool);
            for (std::size_t index = 0; index < rows.size(); ++index)
            {
                const std::uint32_t row = rows[index];
                if (!finite(geometry.centers[index]))
                    continue;
                slots.clear();
                ElementTopology::slotsOf(*table, type, row, slots);
                double value = NOT_A_NUMBER;
                if (name == "PointMassInertia")
                    value = model.findTable<model::PointMassInertiaTable>()->card(row).mass;
                else if (name == "PointMassMatrix")
                    value = model.findTable<model::PointMassMatrixTable>()->card(row).massMatrix[0];
                else if (name == "MassDofInline")
                    value = model.findTable<model::MassDofInlineTable>()->card(row).mass;
                else
                {
                    const base::EntityId pid = model.findTable<model::MassDofPropertyTable>()->card(row).propId;
                    const auto property = model.locate(base::IdSpace::Property, pid);
                    if (property && property->table == model::typeIndexOf<model::MassPropTable>())
                        value = model.findTable<model::MassPropTable>()->card(property->row).mass;
                }
                push(mesh.massPosition, geometry.centers[index]);
                mesh.massEid.push_back(static_cast<std::int32_t>(table->idAt(row)));
                mesh.mass.push_back(value);
                std::int64_t gridRow = -1;
                for (const base::EntityId node : slots)
                    if (node != 0 && (gridRow = grids.row(node)) >= 0)
                        break;
                mesh.massGridRow.push_back(gridRow);
            }
            continue;
        }
        // connections
        for (const std::uint32_t row : rows)
        {
            const base::EntityId eid = table->idAt(row);
            const auto add = [&](const base::Vec3& one, const base::Vec3& two, std::int64_t oneRow, std::int64_t twoRow)
            {
                addSegment(mesh.connectionEnds, mesh.connectionGridRows, one, two, oneRow, twoRow);
                mesh.connectionEid.push_back(static_cast<std::int32_t>(eid));
                mesh.connectionKind.push_back(kind);
            };
            if (kind == FASTENER)
            {
                // GA to GB when the card gives them, else its two piercing points
                if (!piercings)
                    piercings = FastenerPiercings::of(model, pool);
                const PiercedFastener* fastener = piercings->find(model::CardLocation{type, row});
                if (fastener == nullptr)
                    continue;
                const std::vector<base::EntityId>& own = fastener->ownNodes;
                const std::span<const FastenerPiercing> sides = piercings->sidesOf(*fastener);
                if (own.size() >= 2 && grids.row(own[0]) >= 0 && grids.row(own[1]) >= 0)
                {
                    const std::int64_t a = grids.row(own[0]);
                    const std::int64_t b = grids.row(own[1]);
                    add(grids.positions[static_cast<std::size_t>(a)], grids.positions[static_cast<std::size_t>(b)], a,
                        b);
                }
                else if (sides.size() == 2 && sides[0].isFound && sides[1].isFound)
                    add(sides[0].position, sides[1].position, -1, -1);
                continue;
            }
            slots.clear();
            ElementTopology::slotsOf(*table, type, row, slots);
            std::vector<std::pair<base::Vec3, std::int64_t>> points;
            for (const base::EntityId node : slots)
            {
                if (node == 0)
                    continue;
                const std::int64_t gridRow = grids.row(node);
                if (gridRow >= 0)
                    points.emplace_back(grids.positions[static_cast<std::size_t>(gridRow)], gridRow);
            }
            if (points.size() < 2)
                continue;
            if (name == "RigidSpline")
            {
                for (std::size_t index = 0; index + 1 < points.size(); ++index)
                    add(points[index].first, points[index + 1].first, points[index].second, points[index + 1].second);
            }
            else if (name == "RigidTriaPlate" || name == "RigidTriaPlate1")
            {
                for (std::size_t index = 0; index < points.size(); ++index)
                {
                    const auto& next = points[(index + 1) % points.size()];
                    add(points[index].first, next.first, points[index].second, next.second);
                }
            }
            else
            {
                // a star from the first grid point of the card (GN, REFGRID; GA of a two-point element)
                for (std::size_t index = 1; index < points.size(); ++index)
                    if (points[index].second != points[0].second)
                        add(points[0].first, points[index].first, points[0].second, points[index].second);
            }
        }
    }
    // the segments, connections and masses in the order of the element IDs (stable: a CBEAM3's halves stay in order)
    sortRows(mesh.segmentEid, std::pair{&mesh.segmentEnds, std::size_t{6}}, std::pair{&mesh.segmentPid, std::size_t{1}},
             std::pair{&mesh.segmentGridRows, std::size_t{2}});
    sortRows(mesh.connectionEid, std::pair{&mesh.connectionEnds, std::size_t{6}},
             std::pair{&mesh.connectionKind, std::size_t{1}}, std::pair{&mesh.connectionGridRows, std::size_t{2}});
    sortRows(mesh.massEid, std::pair{&mesh.massPosition, std::size_t{3}}, std::pair{&mesh.mass, std::size_t{1}},
             std::pair{&mesh.massGridRow, std::size_t{1}});

    // ---- the outlines of the sections
    if (options.sections)
    {
        const ShearFactorRule rule = ShearFactorRule::Solver;
        for (const base::EntityId pid : sectionPids)
        {
            const auto location = model.locate(base::IdSpace::Property, pid);
            if (!location)
                continue;
            std::string type;
            std::vector<std::pair<double, std::vector<double>>> stations;
            bool beam = false;
            if (location->table == model::typeIndexOf<model::BarShapePropTable>())
            {
                const model::BarShapePropCard card = model.findTable<model::BarShapePropTable>()->card(location->row);
                type = card.kind;
                stations.emplace_back(0.0, card.dims);
            }
            else if (location->table == model::typeIndexOf<model::BeamShapePropTable>())
            {
                const model::BeamShapePropCard card = model.findTable<model::BeamShapePropTable>()->card(location->row);
                type = card.kind;
                beam = true;
                for (const model::BeamShapePropStation& station : card.stations)
                    stations.emplace_back(station.stationRatio, station.dims);
            }
            else
                continue;
            for (std::size_t station = 0; station < stations.size(); ++station)
            {
                const auto& [xxb, dims] = stations[station];
                const SectionOutline outline = SectionFormulas::outline(type, dims);
                if (outline.rings.empty())
                    continue;
                // a PBARL has its neutral axis on the element axis, a PBEAML its
                // shear centre (the finite element formulation)
                const SectionProperties properties =
                    SectionFormulas::standard(type, dims, beam ? rule : ShearFactorRule::None);
                const std::array<double, 2> origin = beam ? properties.drawingShearCentre : properties.drawingCentroid;
                for (std::size_t ring = 0; ring < outline.rings.size(); ++ring)
                {
                    for (const auto& point : outline.rings[ring])
                        mesh.outlinePoints.insert(mesh.outlinePoints.end(),
                                                  {point[0] - origin[0], point[1] - origin[1]});
                    mesh.outlineOffsets.push_back(static_cast<std::int64_t>(mesh.outlinePoints.size() / 2));
                    mesh.outlinePid.push_back(static_cast<std::int32_t>(pid));
                    mesh.outlineStation.push_back(static_cast<std::int32_t>(station));
                    mesh.outlineXxb.push_back(xxb);
                    mesh.outlineHole.push_back(outline.holes[ring]);
                    mesh.outlineType.push_back(type);
                }
            }
        }
    }
    return mesh;
}

void displacePoints(std::span<const double> points, std::span<const std::int64_t> rows,
                    std::span<const double> positions, std::span<const double> motion, std::size_t cases,
                    std::span<double> out)
{
    const std::size_t count = rows.size();
    const std::size_t gridCount = positions.size() / 3;
    for (std::size_t caseIndex = 0; caseIndex < cases; ++caseIndex)
        for (std::size_t index = 0; index < count; ++index)
        {
            double* target = out.data() + 3 * (caseIndex * count + index);
            const std::int64_t row = rows[index];
            if (row < 0)
            {
                target[0] = target[1] = target[2] = NOT_A_NUMBER;
                continue;
            }
            const auto at = static_cast<std::size_t>(row);
            const double* m = motion.data() + 6 * (caseIndex * gridCount + at);
            const double rx = points[3 * index] - positions[3 * at];
            const double ry = points[3 * index + 1] - positions[3 * at + 1];
            const double rz = points[3 * index + 2] - positions[3 * at + 2];
            target[0] = m[0] + m[4] * rz - m[5] * ry;
            target[1] = m[1] + m[5] * rx - m[3] * rz;
            target[2] = m[2] + m[3] * ry - m[4] * rx;
        }
}

} // namespace oor::analysis
