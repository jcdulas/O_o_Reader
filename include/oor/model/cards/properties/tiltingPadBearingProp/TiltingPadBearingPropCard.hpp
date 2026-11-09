#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// The values of one pad of the bearing, in the order of its line: ARC (arc length, degrees), OFFSET (offset factor),
// PRELOAD (preload factor), PVANG (pivot angle, degrees), IP (polar moment), KP (pivot stiffness, 0 for a rigid pivot),
// MP (mass) and IT (bending inertia). A blank one is None.
inline constexpr std::size_t TILTING_PAD_BEARING_PAD_VALUES = 8;
using TiltingPadBearingPropPad = std::array<std::optional<double>, TILTING_PAD_BEARING_PAD_VALUES>;
inline constexpr std::array<const char*, TILTING_PAD_BEARING_PAD_VALUES> TILTING_PAD_BEARING_PAD_NAMES{
    "padArc",          "pivotOffset",    "preload", "pivotAngle",
    "padPolarInertia", "pivotStiffness", "padMass", "padBendingInertia"};

// The values of one speed and load case, in the order of its line: RPM (speed of the journal), FX and FY (the force in
// the negative x and y directions, pounds), PGNU (whirl ratio of the shaft), RELAX (relaxation of the position
// iteration), OFLOW (oil flow to the bearing) and DEREL (relaxation of the pivot deformation iteration). A blank one is
// None.
inline constexpr std::size_t TILTING_PAD_BEARING_CASE_VALUES = 7;
using TiltingPadBearingPropCase = std::array<std::optional<double>, TILTING_PAD_BEARING_CASE_VALUES>;
inline constexpr std::array<const char*, TILTING_PAD_BEARING_CASE_VALUES> TILTING_PAD_BEARING_CASE_NAMES{
    "speed", "loadX", "loadY", "whirlRatio", "positionRelaxation", "oilFlow", "deformationRelaxation"};

// The most pads or cases a card is read with. The counts NPADS and NCASE decide how many lines follow; a count that no
// deck needs would only make the reader allocate lines that do not exist.
inline constexpr std::int32_t TILTING_PAD_BEARING_MAXIMUM_ROWS = 10000;

// The defaults of the fields that have one on the format.
inline constexpr double TILTING_PAD_BEARING_DEFAULT_PIVOT_ROTATION_STIFFNESS = 0.0;
inline constexpr double TILTING_PAD_BEARING_DEFAULT_INITIAL_PIVOT_DEFLECTION = 0.0;
inline constexpr double TILTING_PAD_BEARING_DEFAULT_HOT_OIL_CARRY_OVER = 100.0;
inline constexpr double TILTING_PAD_BEARING_DEFAULT_COLD_OIL_CARRY_OVER = 0.0;
inline constexpr double TILTING_PAD_BEARING_DEFAULT_MAX_ITERATIONS = 100.0;

// One THPAD card: the parameters of a tilting pad journal bearing for an external bearing user service, which an
// ELEMUDS with the UNAME THPAD links to a PBUSH2D. All units are English.
//
// Card layout, one line of eight fields each (the fields are numbered 1 to 8 after the card name, then 9 to 16, ...):
//   line 1   THPAD  RID  TLIMIT
//   line 2   TITLE1                       text over fields 9 to 16 (64 characters)
//   line 3   TITLE2                       text over fields 17 to 24
//   line 4   RADIUS  CLEAR  OUTR  LENGTH  E  ALPHP  ALPHJ  ALPHS
//   line 5   NPADS  NEL  IECC
//   NPADS lines, one pad each:   ARCi  OFFSETi  PRELOADi  PVANGi  IPi  KPi  MPi  ITi
//   then     KFILM  KPAD  TBACK  TJF  PSUMP  TIN  KCAV  CCAV
//            DENSITY  SPEC  TA  MUA  TB  MUB  ESUMP  TMANU
//            ERROR  XG  YG  .  .  NAX  FACTOR  XFACT
//            ITJ  IBC  ITUR  IDIM  .  ICOND  ITB  ITG
//            .  .  ICAV  ICROSS  IDEF  IFLEX  IUN  ISUMP
//            KTHETA  DEL0  HOTOVER  COLDOVER
//            NCASE  MAXC  IALPH
//   NCASE lines, one case each:  RPMi  FXi  FYi  PGNUi  RELAXi  OFLOWi  DERELi
// A dot is a blank cell of the grid, which the card refuses to find filled.
//
// The format is not consistent about the order of E and ALPHP on the fourth line; the card takes the modulus fifth and
// the expansion coefficients after it, the order in which the fields are described: RADIUS, CLEAR, OUTR, LENGTH, E,
// ALPHP, ALPHJ, ALPHS.
//
// RID is the number the PBUSH2D and the ELEMUDS of the bearing have: the card is a companion of the PBUSH2D,
// with an ID space of its own. NPADS and NCASE are the number of pad and case lines, so the card holds no count: it is
// the length of `pads` and `cases`, each at least one. The format types MAXC as a real although it counts iterations,
// and gives IALPH, the stiffness smoothing factor, no integer type either: both are kept as reals (a whole number reads
// as a real too).
//
// Nothing but KTHETA (0.0), DEL0 (0.0), HOTOVER (100.0), COLDOVER (0.0) and MAXC (100) has a default on the format, so
// any other blank field stays blank (None) and is written blank. The flags take the values their descriptions list:
// IECC 0 or 1; ITJ 0, 1 or 2; IBC 0, 1 (not used) or 2; ITUR, IDIM, ICOND, ITB, ITG, ICAV, ICROSS, IUN and ISUMP 0 or
// 1; IDEF 0, 1, 2 or 4; IFLEX 0, 1 or 2.
//
// The values are the inputs of the external bearing service: neither this library nor the solver computes with them.
// They are kept, each under a name that says what it is, so that a deck is written back as it was read.
struct TiltingPadBearingPropCard
{
    base::EntityId id = 0; // unique among the THPAD cards; the PBUSH2D and ELEMUDS of the bearing
    std::optional<double> timeLimit;
    std::string title1; // may be empty
    std::string title2;

    std::optional<double> journalRadius;
    std::optional<double> radialClearance;
    std::optional<double> padOuterRadius;
    std::optional<double> bearingLength;
    std::optional<double> padModulus;
    std::optional<double> padExpansion;
    std::optional<double> journalExpansion;
    std::optional<double> housingExpansion;

    std::optional<std::int32_t> elementCount;
    std::optional<std::int32_t> eccentricityInput;
    std::vector<TiltingPadBearingPropPad> pads; // NPADS of them

    std::optional<double> oilConductivity;
    std::optional<double> padConductivity;
    std::optional<double> padBackTemperature;
    std::optional<double> journalTemperature;
    std::optional<double> sumpPressure;
    std::optional<double> inletTemperature;
    std::optional<double> cavitationConductivity;
    std::optional<double> cavitationSpecificHeat;

    std::optional<double> oilDensity;
    std::optional<double> oilSpecificHeat;
    std::optional<double> viscosityTemperatureA;
    std::optional<double> viscosityA;
    std::optional<double> viscosityTemperatureB;
    std::optional<double> viscosityB;
    std::optional<double> sumpMixingFactor;
    std::optional<double> manufacturingTemperature;

    std::optional<double> convergenceTolerance;
    std::optional<double> initialPositionX;
    std::optional<double> initialPositionY;
    std::optional<double> axialNodeCount;
    std::optional<double> pressureRelaxation;
    std::optional<double> temperatureRelaxation;

    std::optional<std::int32_t> journalThermalModel;
    std::optional<std::int32_t> inletThermalBoundary;
    std::optional<std::int32_t> turbulence;
    std::optional<std::int32_t> dimensionalOutput;
    std::optional<std::int32_t> padConduction;
    std::optional<std::int32_t> padBackThermal;
    std::optional<std::int32_t> journalThermalGradient;

    std::optional<std::int32_t> cavitation;
    std::optional<std::int32_t> crossCoupling;
    std::optional<std::int32_t> padDeformation;
    std::optional<std::int32_t> pivotFlexibility;
    std::optional<std::int32_t> unitSystem;
    std::optional<std::int32_t> floodedSump;

    double pivotRotationStiffness = TILTING_PAD_BEARING_DEFAULT_PIVOT_ROTATION_STIFFNESS;
    double initialPivotDeflection = TILTING_PAD_BEARING_DEFAULT_INITIAL_PIVOT_DEFLECTION;
    double hotOilCarryOver = TILTING_PAD_BEARING_DEFAULT_HOT_OIL_CARRY_OVER;
    double coldOilCarryOver = TILTING_PAD_BEARING_DEFAULT_COLD_OIL_CARRY_OVER;

    double maxIterations = TILTING_PAD_BEARING_DEFAULT_MAX_ITERATIONS;
    std::optional<double> stiffnessSmoothing;
    std::vector<TiltingPadBearingPropCase> cases; // NCASE of them

    [[nodiscard]] bool operator==(const TiltingPadBearingPropCard&) const noexcept = default;
};

} // namespace oor::model
