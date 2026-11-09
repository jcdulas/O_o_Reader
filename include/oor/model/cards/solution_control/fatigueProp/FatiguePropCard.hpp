#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The surface finish of the material of an element (the FINISH field of a PFTG), in the order of the format reference
// list.
enum class FatiguePropFinish : std::uint8_t
{
    None,
    Polish,
    Ground,
    Machine,
    Poor,
    Rolled,
    Cast,
    Krough, // the roughness factor is KFINISH
    Ksurfc  // the surface roughness in microns is KFINISH
};
inline constexpr std::array<std::string_view, 9> FATIGUE_PROP_FINISH_WORDS{
    "None", "PolishedSurface", "GroundSurface",      "MachinedSurface", "PoorSurface", "RolledSurface",
    "Cast", "RoughnessFactor", "SurfaceFinishFactor"};

// One PFTG card: the fatigue properties of the elements that an FTGDEF gives it to (its PFTGID or PFTGIDi).
//
// Card layout:
//   PFTG  ID   (blank)  FINISH  KFINISH  KF     SCALE  OFFSET                       fields 1 to 7
//         (blank)  KTREAT  DIAM  T1  T2  SPTFLG                                      fields 8 to 13
// Field 2 and the first field of the continuation line are unused blank cells (SCALE is field 6), so a value there is
// refused.
//
// The defaults are static (FINISH NONE, KF 1.0, SCALE 1.0, OFFSET 0.0, KTREAT 1.0, SPTFLG 0) and stored resolved; the
// writer leaves them blank. KFINISH has no default and is needed when FINISH is KROUGH (a factor between 0 and 1) or
// KSURFC (a roughness, not negative: the format asks for a positive value but allows 0.0 for some materials). DIAM,
// T1 and T2 are for spot welds only; T1 and T2 come as a pair (one given without the other is an error), and the
// thicknesses come from the shells when they are blank.
//
// The ID is unique among the PFTG cards (IdSpace::PerCardType, the category FatigueProperty that an FTGDEF names).
struct FatiguePropCard
{
    base::EntityId id = 0;
    FatiguePropFinish surfaceFinish = FatiguePropFinish::None;
    std::optional<double> finishFactor;
    double fatigueNotchFactor = 1.0;
    double scaleFactor = 1.0;
    double offset = 0.0;
    double treatmentFactor = 1.0;
    std::optional<double> diameter;
    std::optional<double> thickness1;
    std::optional<double> thickness2;
    int spotWeldFlag = 0; // 1: the spot weld diameter is looked up in a table even for a CWELD
};

} // namespace oor::model
