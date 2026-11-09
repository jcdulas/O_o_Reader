#include "oor/analysis/connections/ContactSurfaces.hpp"

#include "contact/ClosestPoint.hpp"
#include "fasteners/SurfaceProjection.hpp"
#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/frames/StoredFrames.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/analysis/mass/Laminate.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingTable.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <utility>

namespace oor::analysis
{

namespace
{

// A face of a solid with the sorted IDs of its corners: two solids of a body that have the same key share the face.
struct SolidFace
{
    std::array<base::EntityId, 4> key{};
    ContactFace face;
};

// The most cells one face may cover: the cell size grows with the largest face so that one large face among many small
// ones does not fill the index.
constexpr double FACE_SPAN_IN_CELLS = 16.0;

// How well a face suits a point of the other surface whose direction is `direction`, the lower the better: two outward
// normals are best opposed (dot -1), any other two best parallel (|dot| 1); 0 when the point has no direction.
double alignmentOf(const ContactFace& face, const SurfaceDirection& direction) noexcept
{
    const double cosine = base::dot(face.normal, direction.normal);
    return face.isOutward && direction.isOutward ? cosine : -std::abs(cosine);
}

// One candidate face for a point: what tells it from another at the same gap.
struct Candidate
{
    double gap = 0.0; // absolute
    double alignment = 0.0;
    base::EntityId eid = 0;
    std::uint32_t face = 0;
};

bool isBetter(const Candidate& candidate, const Candidate& best) noexcept
{
    const double tie = 1e-12 * std::max(1.0, best.gap);
    if (candidate.gap < best.gap - tie)
        return true;
    if (candidate.gap > best.gap + tie)
        return false;
    constexpr double ALIGNMENT_TIE = 1e-9;
    if (candidate.alignment < best.alignment - ALIGNMENT_TIE)
        return true;
    if (candidate.alignment > best.alignment + ALIGNMENT_TIE)
        return false;
    return candidate.eid != best.eid ? candidate.eid < best.eid : candidate.face < best.face;
}

// A projection on a quadratic face counts when it lands inside the face, to rounding (natural coordinates).
constexpr double INSIDE_TOLERANCE = 1e-9;

// The face as a shell element of the fastener piercing search (ShellShape: the corners first, then the
// mid-side points along the edges 1-2, 2-3, ...; a missing one leaves its edge straight), moved by `shift`.
detail::ShellElement shellElementOf(const ContactFace& face, const base::Vec3& shift) noexcept
{
    detail::ShellElement element;
    element.isVirtual = true;
    const bool quadratic = face.isQuadratic();
    if (face.cornerCount == 3)
        element.kind = quadratic ? detail::ShellShapeKind::Tria6 : detail::ShellShapeKind::Tria3;
    else
        element.kind = quadratic ? detail::ShellShapeKind::Quad8 : detail::ShellShapeKind::Quad4;
    for (std::size_t corner = 0; corner < face.cornerCount; ++corner)
    {
        element.nodes[corner] = face.nodes[corner];
        element.points[corner] = face.corners[corner] + shift;
        element.presentNodes = static_cast<std::uint8_t>(element.presentNodes | (1U << corner));
    }
    for (std::size_t edge = 0; quadratic && edge < face.cornerCount; ++edge)
    {
        if (face.midSides[edge] == 0)
            continue;
        const std::size_t slot = face.cornerCount + edge;
        element.nodes[slot] = face.midSides[edge];
        element.points[slot] = face.midSidePoints[edge] + shift;
        element.presentNodes = static_cast<std::uint8_t>(element.presentNodes | (1U << slot));
    }
    return element;
}

// The closest point of a face (moved by `shift`, whose corners are `corners`) to a point: on the quadratic surface of a
// quadratic face when the point projects inside it, else on the face through its corners (a linear face, a segment, a
// point beyond the edges of a curved face: its neighbour holds it).
detail::ClosestPoint closestPointOf(const ContactFace& face, const std::array<base::Vec3, 4>& corners,
                                    const base::Vec3& shift, const base::Vec3& point) noexcept
{
    if (face.isQuadratic() && face.cornerCount >= 3)
    {
        const detail::SurfaceProjection projection =
            detail::SurfaceProjection::onto(shellElementOf(face, shift), point);
        if (projection.isInside(INSIDE_TOLERANCE))
            return detail::ClosestPoint{projection.point, projection.distance};
    }
    return detail::closestPointOnFace(corners, face.cornerCount, point);
}

// The box of a face: its corners and its mid-side points (a curved face bulges out of the box of its corners).
void boundsOf(const ContactFace& face, base::Vec3& low, base::Vec3& high) noexcept
{
    low = face.corners[0];
    high = face.corners[0];
    const auto extend = [&](const base::Vec3& point)
    {
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            low[axis] = std::min(low[axis], point[axis]);
            high[axis] = std::max(high[axis], point[axis]);
        }
    };
    for (std::size_t corner = 1; corner < face.cornerCount; ++corner)
        extend(face.corners[corner]);
    for (std::size_t edge = 0; edge < face.cornerCount; ++edge)
    {
        if (face.midSides[edge] != 0)
            extend(face.midSidePoints[edge]);
    }
}

// The normal of a face scaled by its area (zero for a segment).
base::Vec3 areaNormalOf(const ContactFace& face) noexcept
{
    if (face.cornerCount == 3)
        return base::cross(face.corners[1] - face.corners[0], face.corners[2] - face.corners[0]) * 0.5;
    if (face.cornerCount == 4)
        return base::cross(face.corners[2] - face.corners[0], face.corners[3] - face.corners[1]) * 0.5;
    return base::Vec3{};
}

// The offset ZOFFS of a shell card (format reference CQUAD4 and the other shells: the distance from the grid points to
// the reference plane of the element along its normal); 0 for a shell that has none.
double offsetOf(const model::Model& model, model::CardLocation location)
{
    if (isType<model::ShellQuad4Table>(location))
        return cardAt<model::ShellQuad4Table>(model, location).offset;
    if (isType<model::ShellTria3Table>(location))
        return cardAt<model::ShellTria3Table>(model, location).offset;
    if (isType<model::ShellQuad8Table>(location))
        return cardAt<model::ShellQuad8Table>(model, location).offset;
    if (isType<model::ShellTria6Table>(location))
        return cardAt<model::ShellTria6Table>(model, location).offset;
    if (isType<model::ShellQuad4DrillingTable>(location))
        return cardAt<model::ShellQuad4DrillingTable>(model, location).offset;
    if (isType<model::ShellTria3DrillingTable>(location))
        return cardAt<model::ShellTria3DrillingTable>(model, location).offset;
    return 0.0;
}

// Where the plies of a laminate stand across its thickness: the distance from the reference plane of the element (the
// plane its offset ZOFFS gives) to the middle of the stack. format reference PCOMP and PCOMPG: Z0 is the distance from
// the reference plane to the bottom surface, -T/2 by default (the laminate centred on its plane); a laminate maintained
// at the outer mould line has Z0 = 0 and stands entirely on the side of the normal. 0 for any other property. The
// library's assumption, which the model glued_contact/glued_pcomp_z0.oos exercises: contact places the skins
// of a laminate on its plies, as its stiffness does. One value per property.
class LaminateCentres
{
public:
    explicit LaminateCentres(const model::Model& model) : m_model(model) {}

    double of(model::CardLocation element)
    {
        const std::optional<base::EntityId> pid = StoredFrames::propertyOf(m_model, element);
        if (!pid)
            return 0.0;
        const auto found = m_centres.find(*pid);
        if (found != m_centres.end())
            return found->second;
        double centre = 0.0;
        if (const auto property = m_model.locate(base::IdSpace::Property, *pid);
            property && Laminate::supports(property->table))
        {
            try
            {
                const std::vector<LaminatePly> plies = Laminate::stack(m_model, *property);
                if (!plies.empty())
                    centre = 0.5 * (plies.front().zBottom + plies.back().zTop);
            }
            catch (const base::OorError&)
            {
                centre = 0.0; // a laminate that cannot be stacked is taken as centred, as its thickness is unknown too
            }
        }
        m_centres.emplace(*pid, centre);
        return centre;
    }

private:
    const model::Model& m_model;
    std::map<base::EntityId, double> m_centres;
};

// The thickness of each shell of `elements` (ElementGeometry: the corner thicknesses of the element, else its
// property's, or the total of a laminate); 0 where it is not known.
std::map<std::pair<std::uint16_t, std::uint32_t>, double> shellThicknesses(
    const model::Model& model, std::span<const model::CardLocation> elements)
{
    std::map<std::uint16_t, std::vector<std::uint32_t>> rowsByType;
    for (const model::CardLocation location : elements)
    {
        if (dimensionOf(ElementTopology::shapeOf(location.table)) == 2)
            rowsByType[location.table].push_back(location.row);
    }
    std::map<std::pair<std::uint16_t, std::uint32_t>, double> result;
    for (const auto& [type, rows] : rowsByType)
    {
        ElementArrays values;
        try
        {
            values = ElementGeometry::compute(model, type, rows, bit(Quantity::Thickness));
        }
        catch (const base::OorError&)
        {
            values.thicknesses.clear(); // an element with a missing grid point: each one alone below
        }
        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            double thickness = index < values.thicknesses.size() ? values.thicknesses[index] : 0.0;
            if (values.thicknesses.empty())
            {
                try
                {
                    thickness =
                        ElementGeometry::of(model, model::CardLocation{type, rows[index]}, bit(Quantity::Thickness))
                            .thickness;
                }
                catch (const base::OorError&)
                {
                    thickness = 0.0;
                }
            }
            result[{type, rows[index]}] = std::isfinite(thickness) && thickness > 0.0 ? thickness : 0.0;
        }
    }
    return result;
}

} // namespace

void ContactFace::appendGridPoints(std::vector<base::EntityId>& out) const
{
    out.insert(out.end(), nodes.begin(), nodes.begin() + cornerCount);
    for (std::size_t edge = 0; edge < cornerCount; ++edge)
    {
        if (midSides[edge] != 0)
            out.push_back(midSides[edge]);
    }
}

void ContactFace::appendLayout(std::vector<std::int32_t>& out) const
{
    out.insert(out.end(), nodes.begin(), nodes.begin() + cornerCount);
    if (isQuadratic())
        out.insert(out.end(), midSides.begin(), midSides.begin() + cornerCount);
}

ShellSkins ShellSkins::fromContactOption(std::int32_t packedCode) noexcept
{
    // COPxxx = A + 10 B + 1000 C, A and B one digit each, C 0, 1, 10 or 11
    const std::int32_t digit = (std::abs(packedCode) % 1000) / 10;
    ShellSkins skins;
    switch (digit)
    {
    case 2:
        skins.side = Side::Bottom;
        break;
    case 3:
        skins.side = Side::Bottom;
        skins.withThickness = false;
        break;
    case 4:
        skins.side = Side::Top;
        break;
    case 5:
        skins.side = Side::Top;
        skins.withThickness = false;
        break;
    case 6:
        skins.withThickness = false;
        break;
    default: // 1, or 0 for a blank field: both skins with the thickness
        break;
    }
    return skins;
}

ContactSurface::ContactSurface(const model::Model& model, const ContactBody& body,
                               std::span<const std::array<double, 3>> positions)
{
    const std::uint16_t gridType = model::typeIndexOf<model::NodeTable>();
    const auto positionOf = [&](base::EntityId nid) -> std::optional<base::Vec3>
    {
        if (nid == 0)
            return std::nullopt;
        const auto location = model.locate(base::IdSpace::Node, nid);
        if (!location || location->table != gridType || location->row >= positions.size())
            return std::nullopt;
        const std::array<double, 3>& value = positions[location->row];
        return base::Vec3{value[0], value[1], value[2]};
    };
    // The corners of a face, with their positions; false when one has none. A mid-side point with no position is left
    // out (its edge is then straight).
    const auto place = [&](ContactFace& face)
    {
        for (std::size_t corner = 0; corner < face.cornerCount; ++corner)
        {
            const auto position = positionOf(face.nodes[corner]);
            if (!position)
                return false;
            face.corners[corner] = *position;
        }
        for (std::size_t edge = 0; edge < face.cornerCount; ++edge)
        {
            const auto position = positionOf(face.midSides[edge]);
            if (!position)
            {
                face.midSides[edge] = 0;
                continue;
            }
            face.midSidePoints[edge] = *position;
            const base::Vec3 middle = (face.corners[edge] + face.corners[(edge + 1) % face.cornerCount]) * 0.5;
            face.bulge = std::max(face.bulge, base::norm(*position - middle));
        }
        return true;
    };
    // The sides of a face that is placed, for the smallest element dimension.
    const auto measureSides = [&](const ContactFace& face, double& smallest)
    {
        const std::size_t sides = face.cornerCount == 2 ? 1 : face.cornerCount;
        for (std::size_t side = 0; side < sides; ++side)
        {
            const double length = base::norm(face.corners[(side + 1) % face.cornerCount] - face.corners[side]);
            if (length > 0.0)
                smallest = std::min(smallest, length);
        }
    };

    const std::map<std::pair<std::uint16_t, std::uint32_t>, double> thicknesses =
        shellThicknesses(model, body.elements);
    LaminateCentres laminateCentres(model);
    std::vector<SolidFace> solidFaces;
    std::vector<std::array<base::EntityId, 4>> scratch;
    std::vector<std::array<base::EntityId, 4>> scratchMidSides;
    for (const model::CardLocation location : body.elements)
    {
        const model::CardTable* table = model.findTable(location.table);
        if (table == nullptr || !table->rows().isAlive(location.row))
            continue;
        const bool isSolid = dimensionOf(ElementTopology::shapeOf(location.table)) == 3;
        scratch.clear();
        scratchMidSides.clear();
        ElementTopology::orientedFacesOf(*table, location.table, location.row, scratch);
        ElementTopology::orientedFaceMidSidesOf(*table, location.table, location.row, scratchMidSides);
        for (std::size_t faceIndex = 0; faceIndex < scratch.size(); ++faceIndex)
        {
            const std::array<base::EntityId, 4>& corners = scratch[faceIndex];
            ContactFace face;
            face.element = location;
            face.eid = table->idAt(location.row);
            for (const base::EntityId node : corners)
            {
                // a blank corner and a repeated one (a collapsed hexahedron) are left out
                if (node != 0 && std::find(face.nodes.begin(), face.nodes.begin() + face.cornerCount, node) ==
                                     face.nodes.begin() + face.cornerCount)
                    face.nodes[face.cornerCount++] = node;
            }
            if (face.cornerCount < 3)
                continue;
            // The mid-side points follow the edges of the face as the template gives them: a face that lost a corner
            // (collapsed, or with a blank one) has other edges, and is taken flat between its corners.
            bool keptAsGiven = faceIndex < scratchMidSides.size();
            for (std::size_t corner = 0; corner < corners.size() && keptAsGiven; ++corner)
                keptAsGiven = corner < face.cornerCount ? corners[corner] == face.nodes[corner] : corners[corner] == 0;
            if (keptAsGiven)
            {
                for (std::size_t edge = 0; edge < face.cornerCount; ++edge)
                {
                    const base::EntityId node = scratchMidSides[faceIndex][edge];
                    const bool isCorner = std::find(face.nodes.begin(), face.nodes.begin() + face.cornerCount, node) !=
                                          face.nodes.begin() + face.cornerCount;
                    face.midSides[edge] = isCorner ? 0 : node;
                }
            }
            face.isOutward = isSolid; // ElementTopology gives the faces of a solid in the outward order
            if (isSolid)
            {
                SolidFace entry;
                entry.key = face.nodes;
                std::sort(entry.key.begin(), entry.key.begin() + face.cornerCount);
                entry.face = face;
                solidFaces.push_back(entry);
                continue;
            }
            if (!place(face))
                continue;
            face.isShell = true;
            const auto thickness = thicknesses.find({location.table, location.row});
            face.halfThickness = thickness != thicknesses.end() ? 0.5 * thickness->second : 0.0;
            face.offset = offsetOf(model, location) + laminateCentres.of(location);
            if (face.halfThickness > 0.0)
                m_thinnestShell = std::min(m_thinnestShell, 2.0 * face.halfThickness);
            m_largestReach = std::max(m_largestReach, std::abs(face.offset) + face.halfThickness);
            measureSides(face, m_smallestEdge);
            m_faces.push_back(face);
        }
    }
    // The outer faces of the solids: a key that appears once. Every face of a solid is an element dimension.
    std::stable_sort(solidFaces.begin(), solidFaces.end(),
                     [](const SolidFace& left, const SolidFace& right) { return left.key < right.key; });
    for (std::size_t first = 0; first < solidFaces.size();)
    {
        std::size_t last = first + 1;
        while (last < solidFaces.size() && solidFaces[last].key == solidFaces[first].key)
            ++last;
        ContactFace& face = solidFaces[first].face;
        if (place(face))
        {
            measureSides(face, m_smallestEdge);
            if (last == first + 1)
                m_faces.push_back(face);
        }
        first = last;
    }
    // A BLSEG line: a segment between every two consecutive points.
    if (body.source == ContactBodySource::SlidelineSegments)
    {
        for (std::size_t point = 1; point < body.grids.size(); ++point)
        {
            ContactFace face;
            face.cornerCount = 2;
            face.nodes = {body.grids[point - 1], body.grids[point], 0, 0};
            if (face.nodes[0] != face.nodes[1] && place(face))
            {
                measureSides(face, m_smallestEdge);
                m_faces.push_back(face);
            }
        }
    }

    // The grid points of the surface: the corners and the mid-side points of its faces.
    std::vector<base::EntityId> nodes(body.grids.begin(), body.grids.end());
    std::vector<double> edges;
    for (const ContactFace& face : m_faces)
    {
        face.appendGridPoints(nodes);
        const std::size_t sides = face.cornerCount == 2 ? 1 : face.cornerCount;
        for (std::size_t side = 0; side < sides; ++side)
            edges.push_back(base::norm(face.corners[(side + 1) % face.cornerCount] - face.corners[side]));
        measureSides(face, m_smallestOuterEdge);
    }
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    for (const base::EntityId node : nodes)
    {
        if (const auto position = positionOf(node))
        {
            m_nodes.push_back(node);
            m_points.push_back(*position);
        }
    }
    // The normals: of each face, and at each grid point the mean of those of its faces, weighted by their areas; the
    // offsets and thicknesses of the shells at each grid point, weighted the same way.
    m_directions.assign(m_nodes.size(), SurfaceDirection{base::Vec3{}, true});
    m_skins.assign(m_nodes.size(), NodeSkin{});
    std::vector<std::uint8_t> touched(m_nodes.size(), 0);
    std::vector<double> shellArea(m_nodes.size(), 0.0);
    std::vector<base::EntityId> facePoints;
    for (ContactFace& face : m_faces)
    {
        const base::Vec3 areaNormal = areaNormalOf(face);
        face.normal = base::normalized(areaNormal);
        const double area = base::norm(areaNormal);
        facePoints.clear();
        face.appendGridPoints(facePoints);
        for (const base::EntityId node : facePoints)
        {
            const auto at = std::lower_bound(m_nodes.begin(), m_nodes.end(), node);
            if (at == m_nodes.end() || *at != node)
                continue;
            const auto index = static_cast<std::size_t>(at - m_nodes.begin());
            m_directions[index].normal += areaNormal;
            m_directions[index].isOutward = m_directions[index].isOutward && face.isOutward;
            touched[index] = 1;
            if (face.isShell)
            {
                m_skins[index].offset += face.normal * (face.offset * area);
                m_skins[index].halfThickness += face.halfThickness * area;
                shellArea[index] += area;
            }
        }
    }
    // The vertex normals of the shells (Nelson Max's weights): each corner of a shell face adds the cross product of
    // its two sides over the product of their squared lengths, turned to the normal of the face.
    for (const ContactFace& face : m_faces)
    {
        if (!face.isShell || face.cornerCount < 3)
            continue;
        for (std::size_t corner = 0; corner < face.cornerCount; ++corner)
        {
            const auto at = std::lower_bound(m_nodes.begin(), m_nodes.end(), face.nodes[corner]);
            if (at == m_nodes.end() || *at != face.nodes[corner])
                continue;
            const base::Vec3 next = face.corners[(corner + 1) % face.cornerCount] - face.corners[corner];
            const base::Vec3 previous =
                face.corners[(corner + face.cornerCount - 1) % face.cornerCount] - face.corners[corner];
            const double lengths = base::dot(next, next) * base::dot(previous, previous);
            if (!(lengths > 0.0))
                continue;
            base::Vec3 weighted = base::cross(next, previous) * (1.0 / lengths);
            if (base::dot(weighted, face.normal) < 0.0)
                weighted = weighted * -1.0;
            m_skins[static_cast<std::size_t>(at - m_nodes.begin())].vertexNormal += weighted;
        }
    }
    for (std::size_t index = 0; index < m_directions.size(); ++index)
    {
        m_skins[index].vertexNormal = base::normalized(m_skins[index].vertexNormal);
        m_directions[index].normal = base::normalized(m_directions[index].normal);
        m_directions[index].isOutward = m_directions[index].isOutward && touched[index] != 0;
        if (shellArea[index] > 0.0)
        {
            m_skins[index].offset = m_skins[index].offset * (1.0 / shellArea[index]);
            m_skins[index].halfThickness /= shellArea[index];
        }
    }
    if (!edges.empty())
    {
        const auto middle = edges.begin() + static_cast<std::ptrdiff_t>(edges.size() / 2);
        std::nth_element(edges.begin(), middle, edges.end());
        m_medianEdgeLength = *middle;
    }
    index();
}

std::int64_t ContactSurface::cellOf(double value, std::size_t axis) const noexcept
{
    return static_cast<std::int64_t>(std::floor((value - m_origin[axis]) / m_cell));
}

void ContactSurface::index()
{
    if (m_faces.empty())
        return;
    constexpr double INFINITE = std::numeric_limits<double>::infinity();
    base::Vec3 low{INFINITE, INFINITE, INFINITE};
    base::Vec3 high{-INFINITE, -INFINITE, -INFINITE};
    double largestFace = 0.0;
    for (const ContactFace& face : m_faces)
    {
        base::Vec3 faceLow;
        base::Vec3 faceHigh;
        boundsOf(face, faceLow, faceHigh);
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            low[axis] = std::min(low[axis], faceLow[axis]);
            high[axis] = std::max(high[axis], faceHigh[axis]);
            largestFace = std::max(largestFace, faceHigh[axis] - faceLow[axis]);
        }
    }
    m_origin = low;
    m_cell = std::max(2.0 * m_medianEdgeLength, largestFace / FACE_SPAN_IN_CELLS);
    if (!(m_cell > 0.0))
        m_cell = std::max({high.x - low.x, high.y - low.y, high.z - low.z, 1.0});
    for (std::uint32_t index = 0; index < m_faces.size(); ++index)
    {
        const ContactFace& face = m_faces[index];
        std::array<std::int64_t, 3> first{};
        std::array<std::int64_t, 3> last{};
        base::Vec3 faceLow;
        base::Vec3 faceHigh;
        boundsOf(face, faceLow, faceHigh);
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            first[axis] = cellOf(faceLow[axis], axis);
            last[axis] = cellOf(faceHigh[axis], axis);
        }
        for (std::int64_t x = first[0]; x <= last[0]; ++x)
            for (std::int64_t y = first[1]; y <= last[1]; ++y)
                for (std::int64_t z = first[2]; z <= last[2]; ++z)
                    m_cells.push_back(CellEntry{x, y, z, index});
    }
    std::sort(m_cells.begin(), m_cells.end(),
              [](const CellEntry& left, const CellEntry& right)
              {
                  if (left.x != right.x)
                      return left.x < right.x;
                  if (left.y != right.y)
                      return left.y < right.y;
                  if (left.z != right.z)
                      return left.z < right.z;
                  return left.face < right.face;
              });
}

namespace
{

// The cosine above which the normals of two shells are parallel (2.6 degrees).
constexpr double COPLANAR_COSINE = 0.999;

// True when `point` (a point of the face, its closest point to some grid point) lies on the boundary of the face:
// within a millionth of the face's size of one of its sides.
bool onBoundary(const std::array<base::Vec3, 4>& corners, std::size_t count, const base::Vec3& point) noexcept
{
    double size = 0.0;
    double nearest = std::numeric_limits<double>::infinity();
    for (std::size_t side = 0; side < count; ++side)
    {
        const base::Vec3& a = corners[side];
        const base::Vec3& b = corners[(side + 1) % count];
        const base::Vec3 along = b - a;
        const double length = base::norm(along);
        size = std::max(size, length);
        const double t = length > 0.0 ? std::clamp(base::dot(point - a, along) / (length * length), 0.0, 1.0) : 0.0;
        nearest = std::min(nearest, base::norm(point - (a + along * t)));
    }
    return nearest <= 1.0e-6 * size;
}

} // namespace

std::optional<SurfaceHit> ContactSurface::meet(const ContactSurface& from, std::size_t index, const ContactZone& zone,
                                               const ShellSkins& fromSkins, const ShellSkins& skins) const
{
    if (m_faces.empty() || !(zone.outside >= 0.0) || !(zone.inside >= 0.0))
        return std::nullopt;
    const base::Vec3& gridPoint = from.points()[index];
    const SurfaceDirection& direction = from.directions()[index];
    const NodeSkin& skin = from.skins()[index];
    // The point on the mid surface of its shells (ZOFFS), and half its thickness when it counts.
    const base::Vec3 point = gridPoint + skin.offset;
    const double ownHalf = fromSkins.withThickness ? skin.halfThickness : 0.0;
    const double reach = std::max(zone.outside, zone.inside) + ownHalf + m_largestReach;
    // A point of shells meets a solid face by the point of its skin (the the solver rule): ZOFFS along its vertex
    // normal.
    const bool onSkin = zone.surfacePoints && skin.halfThickness > 0.0 && base::norm(skin.vertexNormal) > 0.0;
    const double alongNormal = base::dot(skin.offset, skin.vertexNormal);

    std::optional<SurfaceHit> best;
    Candidate bestCandidate;
    const auto consider = [&](std::uint32_t faceIndex)
    {
        const ContactFace& face = m_faces[faceIndex];
        // The face on its mid surface: moved by its offset along its normal.
        std::array<base::Vec3, 4> corners = face.corners;
        const base::Vec3 shift = face.normal * face.offset;
        for (std::size_t corner = 0; corner < face.cornerCount; ++corner)
            corners[corner] += shift;
        detail::ClosestPoint closest = detail::closestPointOnFace(corners, face.cornerCount, point);
        // A curved face stands within its bulge of the flat one through its corners.
        if (closest.distance - face.bulge > reach)
            return;
        if (face.isQuadratic())
            closest = closestPointOf(face, corners, shift, point);
        const base::Vec3 toPoint = point - closest.point;
        const double side = base::dot(toPoint, face.normal); // > 0 on the side the normal points to
        // The thickness of the point counts along the normal of the face (a stiffener standing on a panel touches it
        // with its edge, not with a skin).
        const double pointHalf = ownHalf * std::abs(base::dot(direction.normal, face.normal));
        // Which side of the point's own shells faces the face: the one the face lies on.
        if (fromSkins.side != ShellSkins::Side::Both && base::norm(direction.normal) > 0.0)
        {
            const bool faceAbove = base::dot(closest.point - point, direction.normal) >= 0.0;
            if (faceAbove != (fromSkins.side == ShellSkins::Side::Top))
                return;
        }
        double gap = 0.0;
        double floor = -zone.inside;
        base::Vec3 surfaceOffset;
        if (face.isShell)
        {
            if (skins.side != ShellSkins::Side::Both && (side >= 0.0) != (skins.side == ShellSkins::Side::Top))
                return;
            double faceHalf = skins.withThickness ? face.halfThickness : 0.0;
            double ownPart = pointHalf;
            // Shells side by side, glued edge to edge: the point's shells lie in the plane of the face, the point is at
            // the level of its mid surface, beside it at its boundary. Their skins are level, so neither thickness
            // separates them: the gap is the distance in the plane.
            if ((faceHalf > 0.0 || ownPart > 0.0) &&
                std::abs(base::dot(direction.normal, face.normal)) >= COPLANAR_COSINE &&
                std::abs(side) <= 0.01 * (faceHalf + ownPart) && onBoundary(corners, face.cornerCount, closest.point))
            {
                faceHalf = 0.0;
                ownPart = 0.0;
            }
            gap = closest.distance - faceHalf - ownPart;
            if (zone.throughSkins)
                floor = std::min(floor, -(faceHalf + ownPart));
        }
        else if (onSkin && face.isOutward)
        {
            // the point of the skin nearer the face meets it (the top skin when both are as near)
            double nearest = std::numeric_limits<double>::infinity();
            for (const double sign : {1.0, -1.0})
            {
                if ((sign > 0.0 && fromSkins.side == ShellSkins::Side::Bottom) ||
                    (sign < 0.0 && fromSkins.side == ShellSkins::Side::Top))
                    continue;
                const base::Vec3 offset = skin.vertexNormal * (alongNormal + sign * ownHalf);
                const base::Vec3 onSkinPoint = gridPoint + offset;
                detail::ClosestPoint projected = detail::closestPointOnFace(corners, face.cornerCount, onSkinPoint);
                if (face.isQuadratic())
                    projected = closestPointOf(face, corners, shift, onSkinPoint);
                const double skinSide = base::dot(onSkinPoint - projected.point, face.normal);
                const double signedDistance = skinSide < 0.0 ? -projected.distance : projected.distance;
                if (std::abs(signedDistance) < nearest)
                {
                    nearest = std::abs(signedDistance);
                    closest = projected;
                    gap = signedDistance;
                    surfaceOffset = offset;
                }
            }
            if (!std::isfinite(nearest))
                return;
        }
        else
        {
            // a solid face: inside the body is below it; a segment: the distance
            const double signedDistance = face.isOutward && side < 0.0 ? -closest.distance : closest.distance;
            gap = signedDistance - pointHalf;
            if (zone.throughSkins)
                floor -= pointHalf;
        }
        if (gap > zone.outside || gap < floor)
            return;
        const Candidate candidate{std::abs(gap), alignmentOf(face, direction), face.eid, faceIndex};
        if (!best || isBetter(candidate, bestCandidate))
        {
            best = SurfaceHit{faceIndex, closest.point - shift, gap, std::max(gap, 0.0), surfaceOffset};
            bestCandidate = candidate;
        }
    };
    std::array<std::int64_t, 3> first{};
    std::array<std::int64_t, 3> last{};
    double cellCount = 1.0;
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        first[axis] = cellOf(point[axis] - reach, axis);
        last[axis] = cellOf(point[axis] + reach, axis);
        cellCount *= static_cast<double>(last[axis] - first[axis] + 1);
    }
    if (cellCount > static_cast<double>(m_cells.size()))
    {
        // A zone larger than the surface: every face is near.
        for (std::uint32_t faceIndex = 0; faceIndex < m_faces.size(); ++faceIndex)
            consider(faceIndex);
        return best;
    }
    for (std::int64_t x = first[0]; x <= last[0]; ++x)
    {
        for (std::int64_t y = first[1]; y <= last[1]; ++y)
        {
            for (std::int64_t z = first[2]; z <= last[2]; ++z)
            {
                const CellEntry key{x, y, z, 0};
                auto at = std::lower_bound(m_cells.begin(), m_cells.end(), key,
                                           [](const CellEntry& left, const CellEntry& right)
                                           {
                                               if (left.x != right.x)
                                                   return left.x < right.x;
                                               if (left.y != right.y)
                                                   return left.y < right.y;
                                               if (left.z != right.z)
                                                   return left.z < right.z;
                                               return left.face < right.face;
                                           });
                for (; at != m_cells.end() && at->x == x && at->y == y && at->z == z; ++at)
                    consider(at->face);
            }
        }
    }
    return best;
}

ContactSurfaces ContactSurfaces::build(const model::Model& model, std::vector<const ContactBody*> bodies,
                                       base::ThreadPool* pool)
{
    ContactSurfaces result;
    std::sort(bodies.begin(), bodies.end());
    bodies.erase(std::unique(bodies.begin(), bodies.end()), bodies.end());
    std::span<const std::array<double, 3>> positions;
    try
    {
        positions = BasicPositionCache::of(model, pool).positions();
    }
    catch (const base::OorError&)
    {
        positions = {}; // a broken coordinate system: no grid point can be placed, so no surface has a face
    }
    std::vector<std::unique_ptr<ContactSurface>> built(bodies.size());
    const auto work = [&](std::size_t index)
    { built[index] = std::make_unique<ContactSurface>(model, *bodies[index], positions); };
    if (pool != nullptr && bodies.size() > 1)
        base::parallelFor(*pool, bodies.size(), work);
    else
    {
        for (std::size_t index = 0; index < bodies.size(); ++index)
            work(index);
    }
    result.m_bodies = std::move(bodies);
    result.m_surfaces.reserve(built.size());
    for (std::unique_ptr<ContactSurface>& surface : built)
        result.m_surfaces.push_back(std::move(*surface));
    return result;
}

const ContactSurface& ContactSurfaces::surfaceOf(const ContactBody& body) const
{
    const auto found = std::lower_bound(m_bodies.begin(), m_bodies.end(), &body);
    if (found == m_bodies.end() || *found != &body)
        throw base::UsageError("ContactSurfaces::surfaceOf: expected a body given to build(), found body " +
                               std::to_string(body.id) + " that was not");
    return m_surfaces[static_cast<std::size_t>(found - m_bodies.begin())];
}

std::vector<ContactNodePair> ContactSurfaces::nodePairs(const ContactSurface& secondary, const ContactSurface& primary,
                                                        const ContactZone& zone, const ShellSkins& secondarySkins,
                                                        const ShellSkins& primarySkins)
{
    std::vector<ContactNodePair> pairs;
    for (std::size_t index = 0; index < secondary.nodes().size(); ++index)
    {
        if (const auto hit = primary.meet(secondary, index, zone, secondarySkins, primarySkins))
            pairs.push_back(ContactNodePair{secondary.nodes()[index], hit->face, hit->point, hit->gap, hit->distance,
                                            hit->surfaceOffset});
    }
    return pairs;
}

FaceCoordinates faceCoordinatesOf(const ContactFace& face, const base::Vec3& point) noexcept
{
    if (face.cornerCount == 2)
    {
        const base::Vec3 along = face.corners[1] - face.corners[0];
        const double squared = base::dot(along, along);
        const double fraction = squared > 0.0 ? base::dot(point - face.corners[0], along) / squared : 0.0;
        return FaceCoordinates{2.0 * fraction - 1.0, 0.0};
    }
    // The shape functions of a shell element (ShellShape): the plane of a triangle, the bilinear surface of a
    // quadrilateral, the quadratic surface of a face with mid-side points.
    const detail::SurfaceProjection projection =
        detail::SurfaceProjection::onto(shellElementOf(face, base::Vec3{}), point);
    return FaceCoordinates{projection.xi, projection.eta};
}

} // namespace oor::analysis
