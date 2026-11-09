#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/aero/SplineRadialFunction.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// METH of a SPLINE5: the beam spline of the classical method, or the radial interpolation spline.
enum class AeroBeamSpline5Method : std::uint8_t
{
    Beam,               // BEAM (the default)
    RadialInterpolation // RIS: the method that uses FTYPE and RCORE
};

inline constexpr std::array<std::string_view, 2> AERO_BEAM_SPLINE5_METHOD_WORDS{"Beam", "RigidSurfaceSpline"};

// One SPLINE5 card: a beam spline like SPLINE2, for any aerodynamic geometry: the points it moves are listed in an
// AELIST instead of being a run of box numbers.
//
// Card layout: SPLINE5  EID   CAERO  AELIST        SETG  DZ    DTOR   CID
//                      DTHX  DTHY          USAGE  METH        FTYPE  RCORE
// Fields 4, 11 and 14 (the blank cells) are not used. The format reference calls the first field SID in the description
// and EID in the layout; the layout name is used. Field 4 must be blank. Fields 11 and 14 are ignored: a deck may carry
// a value there, and a value in a cell the card does not use cannot be a value of the card (it is dropped when the card
// is written back). CAERO is the aerodynamic component (ReferenceTarget::AeroElement) that gives the interpolation
// surface and AELIST (ReferenceTarget::AeroList) lists the boxes, the aerodynamic grid points of an AEGRID mesh or,
// with the Mach box method, the (x, y) pairs of an AEFACT. SETG is the SET1 or SET3 of structural grid points; CID is
// the rectangular coordinate system whose y axis is the axis of the beam (0, the basic system, by default; a body
// ignores it).
//
// The flexibilities are as in SPLINE2: DZ zero or more (0.0 by default), DTOR positive (1.0 by default), and DTHX and
// DTHY for the rotations about the x and y axes of the spline, where a NEGATIVE value removes the attachment of that
// rotation, so the sign is kept. DTHX and DTHY have no default and are often left
// blank: a blank is 0.0, a rigid attachment. The continuation line is required: DTHX and DTHY are always written.
//
// METH is BEAM (default) or RIS. FTYPE (WF2 by default) and RCORE are the Wendland function and the radius of support
// of the radial interpolation spline. The card keeps them whatever the method is (a value a method ignores harms
// nothing), but a radial interpolation spline needs RCORE, which has no default. A blank RCORE is std::nullopt.
struct AeroBeamSpline5Card
{
    base::EntityId id = 0;
    base::EntityId aeroPanelId = 0;
    base::EntityId aeroBoxListId = 0;
    base::EntityId nodeSetId = 0;
    double linearFlexibility = 0.0;
    double bendingTorsionRatio = 1.0;
    base::EntityId frameId = 0;
    double rotationFlexibilityX = 0.0;
    double rotationFlexibilityY = 0.0;
    SplineUsage usage = SplineUsage::Both;
    AeroBeamSpline5Method method = AeroBeamSpline5Method::Beam;
    SplineRadialFunction functionKind = SplineRadialFunction::Wendland2;
    std::optional<double> coreRadius;
};

} // namespace oor::model
