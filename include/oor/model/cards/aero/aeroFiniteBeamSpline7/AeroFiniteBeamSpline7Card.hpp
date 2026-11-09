#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// METHOD of a SPLINE7: the number of degrees of freedom the finite beam maps.
enum class AeroFiniteBeamSpline7Method : std::uint8_t
{
    Translations,            // FBS3: the three translations only
    TranslationsAndRotations // FBS6: all six degrees of freedom (the default)
};

inline constexpr std::array<std::string_view, 2> AERO_FINITE_BEAM_SPLINE7_METHOD_WORDS{"FiniteBeamSpline3",
                                                                                       "FiniteBeamSpline6"};

// One SPLINE7 card: a finite beam spline that maps the motion of structural grid points to the boxes (or aerodynamic
// grid points) of an AELIST through a beam it builds, in all six degrees of freedom or in the three translations.
//
// Card layout: SPLINE7  EID  CAERO  AELIST        SETG    DZ   DTOR  CID
//                                         USAGE  METHOD  DZR  IA2   EPSBM
// Fields 4, 9, 10 and 11 (the blank cells) are not used and must be blank; the card has 16 fields. CAERO is the
// aerodynamic panel (ReferenceTarget::AeroElement) that is interpolated and may be blank, unless the points come from a
// CAERO4 or a CAERO5 (a CAERO2 body is not supported, and when the list spans several CAERO4 or CAERO5
// any one of them is valid). The card cannot tell which case it is in, so CAERO is optional here (0 is blank). AELIST
// (ReferenceTarget::AeroList) lists the boxes, the aerodynamic grid points of an AEGRID mesh or, with the Mach box
// method, the (x, y) pairs of an AEFACT. SETG is the SET1 or SET3 of structural grid points and CID the rectangular
// system whose y axis is the axis of the beam and whose planes are the planes of bending (0, the basic system, by
// default).
//
// The flexibilities and ratios, all with a static default: DZ and DZR (the linear and rotational attachment
// flexibilities, zero or more, 0.0 each, zero meaning rigid attachment), DTOR (bending to torsional stiffness,
// positive, 1.0), IA2 (bending to extensional stiffness, positive, 1.0) and EPSBM (the shortest beam as a fraction of
// the total length, positive, 0.01: a smaller value allows shorter beams and so more of them). USAGE is FORCE, DISP or
// BOTH (the default) and METHOD FBS3 or FBS6 (the default).
struct AeroFiniteBeamSpline7Card
{
    base::EntityId id = 0;
    base::EntityId aeroPanelId = 0;
    base::EntityId aeroBoxListId = 0;
    base::EntityId nodeSetId = 0;
    double linearFlexibility = 0.0;
    double bendingTorsionRatio = 1.0;
    base::EntityId frameId = 0;
    SplineUsage usage = SplineUsage::Both;
    AeroFiniteBeamSpline7Method method = AeroFiniteBeamSpline7Method::TranslationsAndRotations;
    double rotationFlexibility = 0.0;
    double bendingExtensionRatio = 1.0;
    double minBeamLengthRatio = 0.01;
};

} // namespace oor::model
