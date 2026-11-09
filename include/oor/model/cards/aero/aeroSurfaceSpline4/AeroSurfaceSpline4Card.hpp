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

// METH of a SPLINE4: how the surface is fitted through the structural grid points.
enum class AeroSurfaceSpline4Method : std::uint8_t
{
    InfinitePlate,      // IPS: the Harder-Desmarais infinite plate spline (the default)
    ThinPlate,          // TPS
    FinitePlate,        // FPS: the method that uses NELEM and MELEM
    RadialInterpolation // RIS: the method that uses FTYPE and RCORE
};

inline constexpr std::array<std::string_view, 4> AERO_SURFACE_SPLINE4_METHOD_WORDS{
    "InfinitePlateSpline", "ThinPlateSpline", "FiniteSurfaceSpline", "RigidSurfaceSpline"};

// One SPLINE4 card: a surface spline like SPLINE1, for any aerodynamic geometry: the points it moves are listed in an
// AELIST instead of being a run of box numbers.
//
// Card layout: SPLINE4  EID    CAERO  AELIST       SETG  DZ  METH  USAGE
//                      NELEM  MELEM  FTYPE  RCORE
// Field 4 (the blank cell after AELIST) is not used. The format reference calls the first field SID in the description
// and EID in the layout; the layout name is used. CAERO is the aerodynamic component that gives the interpolation
// surface: a CAERO panel, or the component number (CMPID) of the AEQUAD4 and AETRIA3 of an aerodynamic mesh
// (ReferenceTarget::AeroPanelOrComponent). AELIST (ReferenceTarget::AeroList) lists the boxes, or the aerodynamic grid
// points of an AEGRID mesh, or, with the Mach box method, the (x, y) pairs of an AEFACT. SETG is the SET1 or SET3 of
// structural grid points. DZ is the linear attachment flexibility (zero or more, 0.0 by default) and USAGE is FORCE,
// DISP or BOTH (the default).
//
// The method decides which of the four last fields count: NELEM and MELEM (positive, 10 by default) are the finite
// elements along the local x and y axes of the finite plate spline; FTYPE (WF2 by default) and RCORE are the Wendland
// function and its radius of support for the radial interpolation spline. The continuation line is optional. The card
// keeps NELEM, MELEM and FTYPE whatever the method is (a value a method ignores harms nothing), and RCORE too, but a
// radial interpolation spline needs RCORE: it has no default and nothing could stand for the missing radius. A blank
// RCORE is std::nullopt.
struct AeroSurfaceSpline4Card
{
    static constexpr std::int32_t DEFAULT_ELEMENTS = 10;

    base::EntityId id = 0;
    base::EntityId aeroPanelId = 0;
    base::EntityId aeroBoxListId = 0;
    base::EntityId nodeSetId = 0;
    double linearFlexibility = 0.0;
    AeroSurfaceSpline4Method method = AeroSurfaceSpline4Method::InfinitePlate;
    SplineUsage usage = SplineUsage::Both;
    std::int32_t elemCountX = DEFAULT_ELEMENTS;
    std::int32_t elemCountY = DEFAULT_ELEMENTS;
    SplineRadialFunction functionKind = SplineRadialFunction::Wendland2;
    std::optional<double> coreRadius;
};

} // namespace oor::model
