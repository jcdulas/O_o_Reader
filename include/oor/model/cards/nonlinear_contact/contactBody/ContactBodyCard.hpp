#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactMotion.hpp"
#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"
#include "oor/model/cards/nonlinear_contact/contactPatchSurface/ContactPatchSurfaceCard.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// The ADVANCE line of a BCBODY: the advanced options of the later versions of the format.
struct ContactBodyAdvance
{
    double smoothingAngle =
        60.0; // the angle (degrees) above which the smoothing detects a discontinuity (SOL 400, spline option)
    std::int32_t contactOption = 0; // how the surfaces of the body may contact (packed number, as COPTS of a BCTABLE)
    std::int32_t midsideProjection = 0; // > 0: the mid-side grid points are projected onto the splines

    [[nodiscard]] bool operator==(const ContactBodyAdvance&) const noexcept = default;
};

// The RIGID line: the rigid body description.
struct ContactBodyRigid
{
    base::EntityId referenceNode = 0; // the grid point of the reference point of the body, or where a load is applied
    std::int32_t entityCount = 1;     // the number of geometric entities that describe the surface
    std::string name;                 // the name of the rigid body (24 characters at most, blank by default)

    [[nodiscard]] bool operator==(const ContactBodyRigid&) const = default;
};

// The BEZIER entry: the control points of a Bezier surface of a rigid body.
struct ContactBodyBezier
{
    std::int32_t pointCount1 = 0;      // the points in the first direction
    std::int32_t pointCount2 = 0;      // the points in the second direction
    std::int32_t subdivisions1 = 0;    // the subdivisions in the first direction
    std::int32_t subdivisions2 = 0;    // the subdivisions in the second direction
    std::vector<base::EntityId> grids; // NP1 x NP2 grid points

    [[nodiscard]] bool operator==(const ContactBodyBezier&) const = default;
};

// The NURBS2D and NURBS entries: a NURBS curve or surface of a rigid body. The format reference gives the header (NPTU
// NORU NSUB for a curve, NPTU NPTV NORU NORV NSUBU NSUBV NTRIM for a surface) and then the control points, the
// homogeneous coordinates, the knots and, for a surface, the trimming curves, each list on lines of its own; the lines
// are kept as they are, field by field (the 7 fields after the first one of each line).
struct ContactBodyNurbs
{
    std::vector<std::array<ContactValue, 7>> lines; // the first is the line of the keyword

    [[nodiscard]] bool operator==(const ContactBodyNurbs&) const = default;
};

// One BCBODY card: a flexible or rigid contact body in 2D or 3D (SOLs 101, 400 and 700; the BCBODY1 is the preferred
// form of SOL 400). A deformable body names the surface it is made of in BSID and has no more than the first line; a
// rigid one is described by the continuation lines. A BCTABLE, BCHANGE or BCMOVE names the body by its BID.
//
// Card layout: BCBODY  BID  DIM  BEHAV  BSID  ISTYP  FRIC  IDSPL  CONTROL
//             (continuation)  NLOAD  ANGVEL  DCOS1  DCOS2  DCOS3  VELRB1  VELRB2  VELRB3
//             "ADVANCE"  SANGLE  COPTB  (blank)  MIDNOD
//             "RIGID"  CGID  NENT  (rigid body name over three fields)
//             "APPROV"  A  N1  N2  N3  V1  V2  V3
//             "GROW"  GF1  GF2  GF3  TAB-GF1  TAB-GF2  TAB-GF3
//             "HEAT"  CFILM  TSINK  HCT  TBODY  HCV  HNC  ITYPE / BNC  EMISS  HBL  HNL  BNL  HNLE  BNLE / HNCE  BNCE
//             CMB  CMS "PATCH3D"  NPATCH, then one line to a patch:  IDP  G1  G2  G3  G4 "BEZIER"  NP1  NP2  NSUB1
//             NSUB2, then the grid points G1  G2 ... "NURBS2D"  NPTU  NORU  NSUB, then the lists of the control data
//             "nurbs"  NPTU  NPTV  NORU  NORV  NSUBU  NSUBV  NTRIM, then the lists of the control data
// The lines after the first one may be left out. Only one of PATCH3D, BEZIER, NURBS2D and NURBS describes a rigid
// surface (RIGID may go with any of them, once). The SOL 700 form has BID, DIM, BEHAV and BSID only.
struct ContactBodyCard
{
    base::EntityId id = 0;                      // BID: unique among BCBODY, BCBODY1, BCSURF and BCGRID
    std::string dimension = "ThreeDimensional"; // 2D or 3D
    std::string behavior = "Deformable";        // DEFORM, RIGID, SYMM (a symmetry body) or HEAT (a heat-rigid body)
    base::EntityId surfaceId =
        0; // a BSURF or BCPROP (deformable body); for SOL 700 also a BCBOX, BCMATL, BCSEG, BCGRID or BCELIPS
    std::int32_t contactKind =
        0;                 // 0 each body against the other, 2 double-sided contact with optimisation of the constraints
    ContactValue friction; // the friction coefficient, or the ID of a table of it; blank: 0.0
    std::int32_t splineSegmentsId =
        0; // > 0: smoothing of the surface by splines, the BLSEG of the discontinuities (< 0: and automatic edges)
    std::int32_t control = 0; // -1 position control, 0 velocity control, a grid point number for load control

    ContactMotion motion;                              // the line of the loads and velocities
    std::optional<ContactBodyAdvance> advance;         // ADVANCE
    std::optional<ContactBodyRigid> rigid;             // RIGID
    std::optional<ContactApproach> approach;           // APPROV
    std::optional<ContactGrowth> growth;               // GROW
    std::optional<std::vector<ContactParameter>> heat; // HEAT: the named fields of its three lines
    std::optional<std::vector<ContactPatch>> patches;  // PATCH3D
    std::optional<ContactBodyBezier> bezier;           // BEZIER
    std::optional<ContactBodyNurbs> nurbs2d;           // NURBS2D
    std::optional<ContactBodyNurbs> nurbs;             // NURBS
};

} // namespace oor::model
