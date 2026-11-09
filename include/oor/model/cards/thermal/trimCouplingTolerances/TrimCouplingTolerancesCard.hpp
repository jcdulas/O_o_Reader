#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The interface coupling type a TRMCPL gives the tolerances of (the CTYPE field). The first five are the interface
// kinds of an ACPEMCP (glued, sliding, open, impervious, air gap); TRMSUR and INTERF are the two that a 1D analytical
// trim component (ACPMCP1) needs. The enumerators are numbered in the order of TRIM_TOLERANCES_TYPE_WORDS.
enum class TrimCouplingTolerancesType : std::uint8_t
{
    Sglued,
    Sslide,
    Sopen,
    Simper,
    Sairgap,
    Trmsur, // the distance between the top and the bottom PLTSURF surfaces of a 1D trim
    Interf  // the distance between the structure or the cavity and the top surface of a 1D trim
};

inline constexpr std::array<std::string_view, 7> TRIM_TOLERANCES_TYPE_WORDS{
    "SolidGlued", "SolidSlide", "SolidOpen", "SimplePerforation", "SolidAirGap", "TrimSurface", "Interface"};

// The defaults the format gives: the relative tolerance, and the absolute tolerance of the first projection (the others
// default to the one before them).
inline constexpr double TRIM_TOLERANCES_DEFAULT_RELATIVE = 0.1;
inline constexpr double TRIM_TOLERANCES_DEFAULT_GAP = 0.01;

// One TRMCPL card: the tolerances with which the interface coupling matrices of a trim component are computed when the
// meshes at the interface do not match. One card gives them for one interface coupling type (CTYPE) of
// the trim component TID (and of its region RID), so a component has one card for each type it needs; with an ACPEMCP
// it is optional, with an ACPMCP1 the two cards TRMSUR and INTERF are required. For an ACPEMCP type (SGLUED,
// SSLIDE, SOPEN, SIMPER, SAIRGAP) the card matches the ACPEMCP of the same TID and RID, and its CTYPE must name a set
// that card gives (a rule between cards).
//
// Card layout: TRMCPL  TID  CTYPE  PLTOL  GAPTOL1  GAPTOL2  GAPTOL3  GAPTOL4  RID        fields 1 to 8
//
// TID is the number of the trim component, a plain integer shared by the cards of the component (IdSpace::Group). CTYPE
// is required. PLTOL is the relative tolerance for the acceptance in the plane, in the element and in the normal
// direction (greater than zero, 0.1 when blank). GAPTOL1 is the absolute tolerance of the first projection of a surface
// on another (greater than zero, 0.01 when blank) and GAPTOL2 to GAPTOL4 those of the next projections of the grids
// that are still not projected; each defaults to the one before it, so a blank one is stored resolved and always
// written. RID is the region of the component, 0 when blank.
struct TrimCouplingTolerancesCard
{
    base::EntityId id = 0; // the trim component, shared by its cards
    TrimCouplingTolerancesType couplingKind = TrimCouplingTolerancesType::Sglued;
    double relativeTolerance = TRIM_TOLERANCES_DEFAULT_RELATIVE;
    double gapTolerance1 = TRIM_TOLERANCES_DEFAULT_GAP;
    double gapTolerance2 = TRIM_TOLERANCES_DEFAULT_GAP; // resolved: the value of GAPTOL1 when blank
    double gapTolerance3 = TRIM_TOLERANCES_DEFAULT_GAP; // resolved: the value of GAPTOL2 when blank
    double gapTolerance4 = TRIM_TOLERANCES_DEFAULT_GAP; // resolved: the value of GAPTOL3 when blank
    base::EntityId regionId = 0;                        // the region of the component, 0 when blank

    [[nodiscard]] bool operator==(const TrimCouplingTolerancesCard&) const noexcept = default;
};

} // namespace oor::model
