#pragma once

#include "oor/analysis/connections/ContactBodies.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace oor::analysis
{

// One face of a contact surface: the face of a shell, an outer face of a solid, or a segment of a BLSEG line, with the
// positions of its corners in the basic system.
struct ContactFace
{
    std::optional<model::CardLocation> element; // none for a segment of a BLSEG line
    base::EntityId eid = 0;                    // 0 for a segment
    std::uint8_t cornerCount = 0;               // 2, 3 or 4
    // The corners, in the order of the face: a shell's in the order of its card, a solid face's in the order that
    // turns its normal outward (ElementTopology::orientedFacesOf); 0 past cornerCount.
    std::array<base::EntityId, 4> nodes{};
    std::array<base::Vec3, 4> corners{};
    // The mid-side grid points of the face of a quadratic element (CTRIA6, CQUAD8, CTETRA 10, CPENTA 15, CHEXA 20,
    // CPYRAM 13, ...): entry k is on the edge from corner k to corner k + 1 (the last corner to the first), 0 for an
    // edge with none; all 0 for a linear face and a segment. The face is then the quadratic surface through its corners
    // and these points (the solver glues the mid-side points and ties to its quadratic segments).
    std::array<base::EntityId, 4> midSides{};
    std::array<base::Vec3, 4> midSidePoints{};
    // How far the mid-side points stand off the straight edges: the largest distance of one from the middle of its edge
    // (0 for a linear face). The flat face through the corners is within about this distance of the curved one.
    double bulge = 0.0;
    // The unit normal of the face as its corners turn (zero for a segment); outward for the face of a solid
    // (`isOutward`), of either sign for a shell.
    base::Vec3 normal;
    bool isOutward = false;
    // A shell: half its thickness and the distance from its grid points to its mid surface along `normal`: its offset
    // ZOFFS, plus, for a laminate, the middle of its plies (PCOMP Z0: a stack that starts at Z0 = 0 stands on the side
    // of the normal). The mid surface is the face moved by `offset`, the skins are `halfThickness` on each side of it.
    // Both 0 for a solid face and a segment, and for a shell whose thickness is not known.
    bool isShell = false;
    double halfThickness = 0.0;
    double offset = 0.0;

    // True when the face has at least one mid-side grid point.
    [[nodiscard]] bool isQuadratic() const noexcept
    {
        return midSides[0] != 0 || midSides[1] != 0 || midSides[2] != 0 || midSides[3] != 0;
    }
    // The grid points of the face that are there: its corners, then its mid-side points (no 0).
    void appendGridPoints(std::vector<base::EntityId>& out) const;
    // The face as `primary_face` gives it: its corners, then on a quadratic face the mid-side point of each edge in
    // order (0 for an edge with none, so that the position says the edge).
    void appendLayout(std::vector<std::int32_t>& out) const;
};

// The direction of a surface at one of its grid points: the mean of the normals of its faces there, and whether they
// are all outward (a grid point of the skin of solids).
struct SurfaceDirection
{
    base::Vec3 normal;
    bool isOutward = false;
};

// The shells at one grid point of a surface, for the point as it searches another surface: the mean of their offsets
// (each ZOFFS along its normal) and of their half thicknesses, weighted by their areas. Zero at a grid point of solids.
struct NodeSkin
{
    base::Vec3 offset;
    double halfThickness = 0.0;
    // The unit normal of the shells at the point, the normals of their corners there weighted as Nelson Max weighs
    // them (the cross product of the two sides of the corner over the product of their squared lengths): the normal
    // along which the solver places the skins of a point. Zero at a point of no shell.
    base::Vec3 vertexNormal;
};

// Which skins of a shell surface take part in a contact and whether its thickness counts: the digit B of the packed
// codes COPTS (the secondary surface) and COPTM (the primary surface): 
// 1 both skins with the thickness (the default; 0, a blank field, too), 2 the bottom skin, 3 the bottom face without
// the thickness, 4 the top skin, 5 the top face without the thickness, 6 both faces without the thickness. The top is
// the side the normal of the shell points to.
struct ShellSkins
{
    enum class Side : std::uint8_t
    {
        Both,
        Top,
        Bottom
    };

    bool withThickness = true;
    Side side = Side::Both;

    [[nodiscard]] static ShellSkins fromContactOption(std::int32_t packedCode) noexcept;
};

// The distances within which a grid point meets a face, on its gap (the signed distance between the skins, negative
// where they overlap): from -inside to outside. With `throughSkins`, a point that overlaps a shell meets it however
// deep, down to its mid surface (the components' rule: a stiffener whose edge lies on the mid surface of a panel meets
// the panel), and a point inside a solid meets it to `inside`.
struct ContactZone
{
    double outside = 0.0;
    double inside = 0.0;
    bool throughSkins = false;
    // The the solver rule: a grid point of shells meets the face of a solid by the point of its skin, (ZOFFS +- T/2)
    // along its vertex normal on the side nearer the face (the top side when both are as near), which is projected on
    // the face.
    bool surfacePoints = false;
};

// Where a point meets a surface: the face, the closest point of it, the gap and the distance.
struct SurfaceHit
{
    std::uint32_t face = 0; // into faces()
    base::Vec3 point;       // on the face as its grid points place it (without ZOFFS and thickness)
    double gap = 0.0;       // between the skins, positive outside the primary body
    double distance = 0.0;  // max(gap, 0): 0 where the skins touch or overlap
    // From the grid point to the point of its skin that meets the face (ContactZone::surfacePoints); zero when the grid
    // point itself meets it.
    base::Vec3 surfaceOffset;
};

// The surface of one contact body: its outer faces and its grid points, with a grid of cells over the faces so that the
// faces near a point are found without looking at the others.
//
// The outer faces of a body are the faces of its shells and the faces of its solids that no other solid of the body
// has (a face two solids of the body share is inside it). The grid points of the surface are the corners and the
// mid-side points of these faces (the grid points of a BCGRID body; the points of a BLSEG line, whose faces are its
// segments). A face of a quadratic element is its quadratic surface: a point that projects inside it is measured to
// that surface, any other to the flat face through its corners. A grid point or a face corner that has no position (a
// scalar point, a grid point that does not exist) is left out with the faces that use it. The thickness of a shell is
// the element's (its corner thicknesses, else its property's, or the total of a laminate: ElementGeometry); its offset
// the ZOFFS of its card. Immutable once built; safe to read from several threads.
class ContactSurface
{
public:
    ContactSurface(const model::Model& model, const ContactBody& body,
                   std::span<const std::array<double, 3>> positions);

    [[nodiscard]] const std::vector<ContactFace>& faces() const noexcept { return m_faces; }
    // The grid points of the surface, sorted, with their positions (parallel).
    [[nodiscard]] const std::vector<base::EntityId>& nodes() const noexcept { return m_nodes; }
    [[nodiscard]] const std::vector<base::Vec3>& points() const noexcept { return m_points; }
    // The direction of the surface at each grid point (parallel to nodes()); a zero normal at a point of no face.
    [[nodiscard]] const std::vector<SurfaceDirection>& directions() const noexcept { return m_directions; }
    // The shells at each grid point (parallel to nodes()).
    [[nodiscard]] const std::vector<NodeSkin>& skins() const noexcept { return m_skins; }
    // The median length of the sides of the faces (the segments of a line); 0 for a surface with no face.
    [[nodiscard]] double medianEdgeLength() const noexcept { return m_medianEdgeLength; }
    // The smallest side of a face that is not 0: of every face of the elements of the body (a face two of its solids
    // share too), and of its outer faces only. Infinite for a surface with no face.
    [[nodiscard]] double smallestEdgeLength() const noexcept { return m_smallestEdge; }
    [[nodiscard]] double smallestOuterEdgeLength() const noexcept { return m_smallestOuterEdge; }
    // The thinnest shell of the body; infinite when it has none.
    [[nodiscard]] double thinnestShell() const noexcept { return m_thinnestShell; }

    // The face that `point`, the grid point at `index` of `from`, meets within `zone`: the one of the smallest absolute
    // gap. `fromSkins` and `skins` say which skins of the two surfaces take part. Nothing when no face is that close.
    // Cost: the faces of the cells the zone covers.
    //
    // Faces at the same gap (a point on an edge or a corner of the surface: the corner of a cube resting on another is
    // at distance 0 from its top face and from two of its sides) are told apart by the direction of `from` at the
    // point: two outward normals are best opposed (the faces of two solids that touch face each other), else the planes
    // are best parallel; then by the lowest element ID, then by the first face.
    [[nodiscard]] std::optional<SurfaceHit> meet(const ContactSurface& from, std::size_t index, const ContactZone& zone,
                                                 const ShellSkins& fromSkins, const ShellSkins& skins) const;

private:
    struct CellEntry
    {
        std::int64_t x = 0;
        std::int64_t y = 0;
        std::int64_t z = 0;
        std::uint32_t face = 0;
    };

    void index();
    [[nodiscard]] std::int64_t cellOf(double value, std::size_t axis) const noexcept;

    std::vector<ContactFace> m_faces;
    std::vector<base::EntityId> m_nodes;
    std::vector<base::Vec3> m_points;
    std::vector<SurfaceDirection> m_directions;
    std::vector<NodeSkin> m_skins;
    double m_medianEdgeLength = 0.0;
    double m_smallestEdge = std::numeric_limits<double>::infinity();
    double m_smallestOuterEdge = std::numeric_limits<double>::infinity();
    double m_thinnestShell = std::numeric_limits<double>::infinity();
    double m_largestReach = 0.0; // the largest |offset| + halfThickness of a shell face
    base::Vec3 m_origin;
    double m_cell = 1.0;
    std::vector<CellEntry> m_cells; // sorted by (x, y, z, face)
};

// One grid point of a secondary surface that meets a primary surface.
struct ContactNodePair
{
    base::EntityId nid = 0;
    std::uint32_t face = 0; // the face of the primary surface it meets
    base::Vec3 point;       // its projection on the primary face, basic system
    double gap = 0.0;
    double distance = 0.0;
    base::Vec3 surfaceOffset; // SurfaceHit::surfaceOffset
};

// The surfaces of contact bodies, built once for each body a search uses, and the search between two of them.
class ContactSurfaces
{
public:
    // Builds the surfaces of `bodies` (each once), spread over the pool.
    [[nodiscard]] static ContactSurfaces build(const model::Model& model, std::vector<const ContactBody*> bodies,
                                               base::ThreadPool* pool = nullptr);

    // The surface of a body given to build().
    [[nodiscard]] const ContactSurface& surfaceOf(const ContactBody& body) const;

    // The grid points of `secondary` that meet `primary` within `zone`, each with the face it meets: by increasing grid
    // point. A grid point of both surfaces (bodies that share a node) meets the faces it is a corner of.
    [[nodiscard]] static std::vector<ContactNodePair> nodePairs(const ContactSurface& secondary,
                                                                const ContactSurface& primary, const ContactZone& zone,
                                                                const ShellSkins& secondarySkins,
                                                                const ShellSkins& primarySkins);

private:
    std::vector<const ContactBody*> m_bodies; // sorted by address
    std::vector<ContactSurface> m_surfaces;   // parallel
};

// The local coordinates of a point of a face (glued contact): xi and eta from -1 to 1 on a
// quadrilateral (corner 1 at (-1, -1), corner 3 at (1, 1); the bilinear surface through the corners), the area
// coordinates L2 and L3 on a triangle, the position along a segment from -1 to 1 (eta 0); on the quadratic surface of a
// face with mid-side points (the serendipity functions of CQUAD8 and CTRIA6).
struct FaceCoordinates
{
    double xi = 0.0;
    double eta = 0.0;
};

[[nodiscard]] FaceCoordinates faceCoordinatesOf(const ContactFace& face, const base::Vec3& point) noexcept;

} // namespace oor::analysis
