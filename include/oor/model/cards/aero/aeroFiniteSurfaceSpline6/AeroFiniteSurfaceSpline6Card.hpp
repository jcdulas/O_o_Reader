#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// METHOD of a SPLINE6: the degrees of freedom of the independent points that connect to the virtual surface.
enum class AeroFiniteSurfaceSpline6Method : std::uint8_t
{
    Translations,            // FPS3: the three translations only
    TranslationsAndRotations // FPS6: all six degrees of freedom (the default)
};

inline constexpr std::array<std::string_view, 2> AERO_FINITE_SURFACE_SPLINE6_METHOD_WORDS{"FiniteSurfaceSpline3",
                                                                                          "FiniteSurfaceSpline6"};

// VSTYPE of a SPLINE6: what the elements of the virtual surface (the AELIST VSLIST) are.
enum class AeroFiniteSurfaceSpline6SurfaceType : std::uint8_t
{
    Aero, // AERO: boxes or elements of the aerodynamic mesh (the default)
    Struc // STRUC: shell elements of the structure
};

inline constexpr std::array<std::string_view, 2> AERO_FINITE_SURFACE_SPLINE6_SURFACE_TYPE_WORDS{"AeroMesh",
                                                                                                "Structure"};

// METHVS of a SPLINE6: the degrees of freedom of the virtual surface that connect to the dependent points.
enum class AeroFiniteSurfaceSpline6SurfaceMethod : std::uint8_t
{
    TranslationsAndRotations, // VS6: all six degrees of freedom (the default)
    Translations              // VS3: the three translations only
};

inline constexpr std::array<std::string_view, 2> AERO_FINITE_SURFACE_SPLINE6_SURFACE_METHOD_WORDS{"VibrationSimple6",
                                                                                                  "VibrationSimple3"};

// METHCON of a SPLINE6: how the points of the two meshes are tied to the virtual surface.
enum class AeroFiniteSurfaceSpline6Connection : std::uint8_t
{
    NodalProximity, // NODEPROX: the nearest points of the other mesh (the default)
    CircularBias    // CIRCBIAS: points that surround the one to connect, found through the elements of the surface
};

inline constexpr std::array<std::string_view, 2> AERO_FINITE_SURFACE_SPLINE6_CONNECTION_WORDS{"NodeProximity",
                                                                                              "CircularBias"};

// One SPLINE6 card: a finite surface spline between two meshes in 6 or 3 degrees of freedom. A virtual surface mesh
// sits between them, and virtual RBE3 elements that the solver generates tie each mesh to it.
//
// Card layout: SPLINE6  EID     CAERO   AELIST         SETG    DZ    METHOD   USAGE
//                      VSTYPE  VSLIST  I2VNUM  D2VNUM METHVS  DZR   METHCON  NGRID
//                      ELTOL   NCYCLE  AUGWEI
// Field 4 (the blank cell after AELIST) is not used and must be blank; the card has 19 fields. CAERO is the aerodynamic
// panel (ReferenceTarget::AeroElement); it may be blank when an AEGRID mesh gives the aerodynamic points, and it is
// needed only to name one of the CAERO4 or CAERO5 the points come from (a CAERO2 body is not supported). AELIST
// (ReferenceTarget::AeroList) lists the boxes or the aerodynamic grid points to interpolate; SETG is the SET1 or SET3
// of structural grid points. VSLIST is the AELIST (ReferenceTarget::AeroList) of the shell elements of the virtual
// surface (the boxes of a CAERO when VSTYPE is AERO, structural CQUAD4, CQUADR, CTRIA3 or CTRIAR elements when it is
// STRUC). It may be blank only when CAERO is given and VSTYPE is AERO; with an AEGRID mesh (no CAERO) or a structural
// surface it must name the AELIST that defines the surface.
//
// DZ and DZR are the linear and rotational attachment flexibilities (zero or more, 0.0 each; zero is rigid). I2VNUM and
// D2VNUM are the least number of structural (independent) and aerodynamic (dependent) points tied to each virtual
// point, 3 each. The four last fields (NGRID, ELTOL, NCYCLE, AUGWEI) tune the circular bias connection and serve only
// when METHCON is CIRCBIAS: NGRID positive (1), ELTOL the tolerance of the projection as a
// percentage of the element size (any real, 100.0), NCYCLE positive (3), AUGWEI zero or more (0.0). They are kept
// whatever METHCON is, because a value the nodal proximity method does not use harms nothing, and refusing it would
// turn a deck that the solver may read into text.
struct AeroFiniteSurfaceSpline6Card
{
    base::EntityId id = 0;
    base::EntityId aeroPanelId = 0; // 0 is blank
    base::EntityId aeroBoxListId = 0;
    base::EntityId nodeSetId = 0;
    double linearFlexibility = 0.0;
    AeroFiniteSurfaceSpline6Method method = AeroFiniteSurfaceSpline6Method::TranslationsAndRotations;
    SplineUsage usage = SplineUsage::Both;
    AeroFiniteSurfaceSpline6SurfaceType surfaceKind = AeroFiniteSurfaceSpline6SurfaceType::Aero;
    base::EntityId surfaceElemListId = 0; // 0 is blank
    std::int32_t independentPointCount = 3;
    std::int32_t dependentPointCount = 3;
    AeroFiniteSurfaceSpline6SurfaceMethod surfaceMethod =
        AeroFiniteSurfaceSpline6SurfaceMethod::TranslationsAndRotations;
    double rotationFlexibility = 0.0;
    AeroFiniteSurfaceSpline6Connection connectionMethod = AeroFiniteSurfaceSpline6Connection::NodalProximity;
    std::int32_t nodeCount = 1;
    double projectionTolerance = 100.0;
    std::int32_t cycleCount = 3;
    double augmentationWeight = 0.0;
};

} // namespace oor::model
