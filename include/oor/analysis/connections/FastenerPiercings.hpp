#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::analysis
{

// The form of a fastener, by the word of its card: Fastener kind (Elements, Property) or Weld kind (ElemPatch,
// PartPatch, ElemIdPatch, NodeIdPatch, AlignedNodes). fastenerFormName gives that word.
enum class FastenerForm : std::uint8_t
{
    Elem,
    Prop,
    Elpat,
    Partpat,
    Elemid,
    Gridid,
    Align
};

[[nodiscard]] std::string_view fastenerFormName(FastenerForm form) noexcept;

// How far from a shell element a piercing point may be, in sizes of that element (the mean length of its
// corner-to-corner edges): the distance from the point projected (GS, the point XS YS ZS, or a given GA or GB) to the
// surface, and the distance along the fastener axis from one piercing point to the other. Far enough for the gap
// between the sheets of a joint, near enough to refuse a location plainly off the shells. A choice of this library.
inline constexpr double PIERCING_REACH_IN_SIZES = 10.0;

// One side of a CFAST or CWELD: where the fastener pierces the shells of that side, the element it pierces and its
// natural coordinates there, the grid points of the element and the values of its shape functions at the point (they
// sum to 1). Pure geometry: what a solver needs to tie the point to the element.
struct FastenerPiercing
{
    model::CardLocation card; // the CFAST or CWELD
    base::EntityId eid = 0;
    FastenerForm form = FastenerForm::Prop;
    char side = 'A';
    base::EntityId pointGrid = 0; // GA (side A) or GB (side B) when the card gives it: the point is that grid point
    bool isFound = false;
    base::Vec3 position; // the piercing point in the basic system
    // The element pierced: a shell of the model, or none for the face of grid points of a CWELD GRIDID.
    bool hasElement = false;
    model::CardLocation element;
    base::EntityId elementId = 0;
    double xi = 0.0; // natural coordinates of the point on the element (a triangle: xi = L2, eta = L3), within it
    double eta = 0.0;
    std::vector<base::EntityId> nids; // the grid points of the element in its order (blank mid-side points left out)
    std::vector<double> shapeValues;  // one per grid point of nids
    // When the point is not found: the field of the card the problem is about and what was expected and found.
    std::string field;
    std::string problem;
};

// One CFAST or CWELD as a connection: the grid points of its card it joins itself and where its sides are in
// FastenerPiercings::sides().
struct PiercedFastener
{
    model::CardLocation card;
    base::EntityId eid = 0;
    FastenerForm form = FastenerForm::Prop;
    base::EntityId ga = 0; // GA and GB as the card gives them, 0 when blank
    base::EntityId gb = 0;
    // GA and GB when the card gives them (the two ends of an ALIGN), GS for a point-to-patch connection (its other
    // end); GS otherwise only locates the fastener. Sorted.
    std::vector<base::EntityId> ownNodes;
    std::uint32_t firstSide = 0;
    std::uint32_t sideCount = 0; // 2; 1 for a point-to-patch connection; 0 for ALIGN
};

// The piercing points of every CFAST and CWELD of a model, side by side (FastenerPiercing), found from the geometry of
// the shells: CQUAD4, CQUADR, CTRIA3, CTRIAR (linear) and CQUAD8, CTRIA6 (quadratic, mid-side points optional), each
// taken as the isoparametric surface of its grid points, so that a warped quadrilateral or a curved quadratic element
// is pierced where its surface is (offsets such as ZOFFS are not applied: the surface of the grid points).
//
// The surface of each side, from the form of the card:
// - CFAST PROP, CWELD PARTPAT: the shells of the property IDA (PIDA) for side A, IDB (PIDB) for side B, searched
//   through a grid of cells over the elements of the property;
// - CFAST ELEM, CWELD ELPAT: the shell IDA (SHIDA), and when the point does not fall on it, the nearest of the shells
//   that share a grid point with it;
// - CWELD ELEMID: the shell SHIDA (SHIDB) alone;
// - CWELD GRIDID: the face of the grid points GA1 to GA8 (GB1 to GB8), a quadrilateral or a triangle as the letter of
//   SPTYP for the side says, the mid-side points optional;
// - CWELD ALIGN: none; GA and GB are the ends themselves, and the fastener has no row.
//
// The piercing points, from the meaning of the fields:
// 1. A side whose grid point the card gives (GA, GB) is that grid point; it pierces the element of its surface on
//    which it projects (as in 2).
// 2. Otherwise side A is the projection of the location of the fastener (the grid point GS, else the point XS, YS, ZS)
//    on surface A: the point of the surface nearest the location (SurfaceProjection, on each candidate element).
// 3. Otherwise side B is where the fastener axis crosses surface B: the axis is the line through A along the normal of
//    the element A pierces, at A; its crossings with the elements of surface B are found by Newton's method
//    (detail::crossElement), the nearest to A taken. When the card gives GB and neither GA nor a location, side B comes
//    first (rule 1) and A is the crossing of the normal at B with surface A (a choice).
// 4. A point-to-patch CWELD (ELEMID without SHIDB, GRIDID with SPTYP Q or T) has a side A only; its other end is GS.
//
// A point falls on an element when its projection (or crossing) lies inside it or outside by at most the PROJTOL of the
// SWLDPRM card (0.02 when not given) in natural coordinates (beyond the edges of a quadrilateral, below 0 for an area
// coordinate of a triangle), the element it falls on strictly inside being preferred; and when it is at most
// PIERCING_REACH_IN_SIZES element sizes away. Of several elements, the one the card names, else the nearest, else the
// lowest element ID (a point on a shared edge or grid point). The natural coordinates are then brought onto the element
// (detail::clampToElement), the position of a generated point taken there, and the shape functions evaluated there
// (ShellShape: bilinear and linear, or quadratic serendipity with the mid-side points present).
//
// A side whose point is not found keeps isFound false, with the field of the card and a message that states the
// tolerance it missed and the nearest element.
//
// As a connection (PiercedFastener), a fastener joins the element it pierces on side A to the element it pierces on
// side B: it acts on the grid points of those elements (the face of grid points of a GRIDID side) and on its own grid
// points GA and GB (GA and GB alone for ALIGN); GS only locates it, except in a point-to-patch connection, whose other
// end it is.
//
// Computed one fastener after the other, the work spread over the pool; cached with the model until its content
// changes (ComponentCache); immutable once built.
class FastenerPiercings
{
public:
    [[nodiscard]] static std::shared_ptr<const FastenerPiercings> of(const model::Model& model,
                                                                     base::ThreadPool* pool = nullptr);
    // Computes without the cache.
    [[nodiscard]] static FastenerPiercings compute(const model::Model& model, base::ThreadPool* pool = nullptr);

    // By fastener element ID, side A before side B.
    [[nodiscard]] const std::vector<FastenerPiercing>& sides() const noexcept { return m_sides; }
    // Every CFAST and CWELD (ALIGN included), by element ID.
    [[nodiscard]] const std::vector<PiercedFastener>& fasteners() const noexcept { return m_fasteners; }
    // The fastener at `card`, or null when it is not a CFAST or a CWELD.
    [[nodiscard]] const PiercedFastener* find(model::CardLocation card) const noexcept;
    // The rows of sides() of a fastener: side A, then side B.
    [[nodiscard]] std::span<const FastenerPiercing> sidesOf(const PiercedFastener& fastener) const noexcept
    {
        return std::span<const FastenerPiercing>(m_sides).subspan(fastener.firstSide, fastener.sideCount);
    }
    // The grid points a fastener acts on: its own grid points and those of the elements its sides pierce, sorted.
    [[nodiscard]] std::vector<base::EntityId> connectionNodes(const PiercedFastener& fastener) const;

private:
    std::vector<FastenerPiercing> m_sides;
    std::vector<PiercedFastener> m_fasteners;
    std::vector<std::uint32_t> m_byLocation; // positions into m_fasteners, sorted by (table, row)
};

} // namespace oor::analysis
