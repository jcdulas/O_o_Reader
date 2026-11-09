#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// METH of a SPLINE1: how the surface is fitted through the structural grid points.
enum class AeroSurfaceSpline1Method : std::uint8_t
{
    InfinitePlate, // IPS: the Harder-Desmarais infinite plate spline (the default)
    ThinPlate,     // TPS
    FinitePlate    // FPS: the only method that looks at NELEM and MELEM
};

inline constexpr std::array<std::string_view, 3> AERO_SURFACE_SPLINE1_METHOD_WORDS{
    "InfinitePlateSpline", "ThinPlateSpline", "FiniteSurfaceSpline"};

// One SPLINE1 card: a surface spline that carries the displacements of a set of structural grid points to a regular
// array of aerodynamic boxes, and the aerodynamic forces back to the grid points.
//
// Card layout: SPLINE1  EID    CAERO  BOX1  BOX2  SETG  DZ  METH  USAGE
//                      NELEM  MELEM
// CAERO is the aerodynamic element (a CAERO1 to CAERO5, ReferenceTarget::AeroElement) whose plane the spline lies in.
// The boxes BOX1 to BOX2 are the ones it moves, numbered as the CAERO numbers them (ReferenceTarget::AeroBox: a number
// that no CAERO owns is not an error here, because with the Mach box method the numbers count the (x, y) pairs of an
// AEFACT instead). The field table of the format reference says BOX2 >= BOX1 (the text of its description loses the
// sign), so one box is a legal spline. SETG is the SET1 or SET3 that lists the structural grid points.
//
// DZ is the linear attachment flexibility, a length squared: zero makes the surface pass through every deflected grid
// point, a value far above the area of the spline gives a least-squares plane, and in between the surface is smoothed.
// USAGE keeps the spline out of the force transformation (DISP) or out of the displacement transformation (FORCE); a
// blank is BOTH. NELEM and MELEM are the numbers of finite elements along the local x and y axes of the spline, which
// only FPS uses; the continuation line holding them is optional. They are kept whatever METH is (a value the method
// ignores harms nothing, and refusing it would turn a deck that the solver reads into text), and each is always
// positive.
struct AeroSurfaceSpline1Card
{
    static constexpr std::int32_t DEFAULT_ELEMENTS = 10;

    base::EntityId id = 0;
    base::EntityId aeroPanelId = 0;
    base::EntityId firstBox = 0;
    base::EntityId lastBox = 0;
    base::EntityId nodeSetId = 0;
    double linearFlexibility = 0.0;
    AeroSurfaceSpline1Method method = AeroSurfaceSpline1Method::InfinitePlate;
    SplineUsage usage = SplineUsage::Both;
    std::int32_t elemCountX = DEFAULT_ELEMENTS;
    std::int32_t elemCountY = DEFAULT_ELEMENTS;
};

} // namespace oor::model
