#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

namespace oor::model
{

// ---------------------------------------------------------------------------------------------------------------------
// The words of the fields. Each enumeration is numbered in the order of its array of words (ExplicitSupport.hpp reads
// and writes them by position), the first word being the default of the field where the format reference gives one.
// ---------------------------------------------------------------------------------------------------------------------

// The type of analysis: stress-life (S-N, the default) or strain-life (E-N).
enum class FatigueParamsType : std::uint8_t
{
    Sn,
    En
};
inline constexpr std::array<std::string_view, 2> FATIGUE_PARAMS_TYPE_WORDS{"StressLifeMethod", "StrainLifeMethod"};

// The flag of the result line: the stresses or the strains of the finite element analysis feed the fatigue calculation.
enum class FatigueParamsResultKind : std::uint8_t
{
    Stress,
    Strain
};
inline constexpr std::array<std::string_view, 2> FATIGUE_PARAMS_RESULT_WORDS{"Stress", "Strain"};

// The stress or strain combination (the COMB field): one scalar made of the
// components at each point of each time step, as the damage models are uniaxial.
enum class FatigueParamsCombination : std::uint8_t
{
    AbsMaxPr, // the absolute maximum principal value (the default)
    SgVon,    // the signed von Mises stress
    SgMaxShr, // the signed maximum shear
    Critical, // critical plane analysis
    Standard, // STNDRD: the critical plane of a spot weld (spot welds only)
    CompX,
    CompY,
    CompZ,
    CompXy,
    CompYz,
    CompZx
};
inline constexpr std::array<std::string_view, 11> FATIGUE_PARAMS_COMBINATION_WORDS{
    "AbsoluteMaxPrincipal", "SignedVonMises",    "SignedMaxShear",   "CriticalPlane",
    "StandardCorrection",   "ComponentXStress",  "ComponentYStress", "ComponentZStress",
    "ComponentXyStress",    "ComponentYzStress", "ComponentZxStress"};

// The mean stress correction (the CORR field).
enum class FatigueParamsCorrection : std::uint8_t
{
    None,
    Goodman,
    Gerber,
    Gdmant, // tension only Goodman
    Grbert, // tension only Gerber
    Fkm,
    Simple, // the modified FKM method of the spot welds
    Interp,
    Swt,
    Morrow,
    Walker,
    Mmpds
};
inline constexpr std::array<std::string_view, 12> FATIGUE_PARAMS_CORRECTION_WORDS{"None",
                                                                                  "GoodmanCorrection",
                                                                                  "GerberCorrection",
                                                                                  "GoodmanTensionCorrection",
                                                                                  "GerberTensionCorrection",
                                                                                  "FkmGuideline",
                                                                                  "Simple",
                                                                                  "InterpolationMethod",
                                                                                  "SmithWatsonTopper",
                                                                                  "MorrowCorrection",
                                                                                  "WalkerCorrection",
                                                                                  "MetallicHandbookData"};

// The plasticity correction (the PLAST field): the format lists the Neuber method only.
enum class FatigueParamsPlasticity : std::uint8_t
{
    Neuber
};
inline constexpr std::array<std::string_view, 1> FATIGUE_PARAMS_PLASTICITY_WORDS{"Neuber"};

// Where the fatigue lives are reported (the LOC field): at the grid points from averaged stresses, at the nodes of each
// element, or at the centre of each element.
enum class FatigueParamsLocation : std::uint8_t
{
    Noda,
    Node,
    Elem
};
inline constexpr std::array<std::string_view, 3> FATIGUE_PARAMS_LOCATION_WORDS{"NodalAveraging", "Node", "Elements"};

// How the stresses at a point are recovered (the RECOVER field, the words of the STRESS case control).
enum class FatigueParamsRecovery : std::uint8_t
{
    Corner,
    Sgage,
    Bilin,
    Cubic,
    Center
};
inline constexpr std::array<std::string_view, 5> FATIGUE_PARAMS_RECOVERY_WORDS{"CornerPoints", "StrainGageFit",
                                                                               "BilinearFit", "CubicFit", "Center"};

// The data reduction of the RAINFLOW line: only LOAD exists.
inline constexpr std::string_view FATIGUE_PARAMS_RAINFLOW_TYPE = "Load";

// The option of the FOS line: a factor of safety from the targeted life, or none.
enum class FatigueParamsSafetyOption : std::uint8_t
{
    Life,
    None
};
inline constexpr std::array<std::string_view, 2> FATIGUE_PARAMS_SAFETY_OPTION_WORDS{"LifeResult", "None"};

// Where a seam weld reports its life (the LOCSM field).
enum class FatigueParamsSeamLocation : std::uint8_t
{
    Node,
    Sgage,
    Corner,
    Bilin,
    Cubic
};
inline constexpr std::array<std::string_view, 5> FATIGUE_PARAMS_SEAM_LOCATION_WORDS{
    "Node", "StrainGageFit", "CornerPoints", "BilinearFit", "CubicFit"};

// The method of the multiaxial assessment (the MMTHD field). The format writes the third word as "STANDard": both the
// abbreviation and the whole word are read and kept as written.
enum class FatigueParamsMultiMethod : std::uint8_t
{
    None,
    Simple,
    Standard,
    Stand,
    Auto
};
inline constexpr std::array<std::string_view, 5> FATIGUE_PARAMS_MULTI_METHOD_WORDS{"None", "Simple", "Standard",
                                                                                   "Standard", "Automatic"};

// How the grid point stresses are averaged (the MTHD field of the NAVG line): topologically or geometrically.
enum class FatigueParamsAveraging : std::uint8_t
{
    Topological,
    Geometric
};
inline constexpr std::array<std::string_view, 2> FATIGUE_PARAMS_AVERAGING_WORDS{"Topology", "GeometryOption"};

// The direction that defines the surface normal or the stress Z axis (the NORMAL field of the NAVG line): the radius
// vector from the origin or an axis, each possibly reversed.
enum class FatigueParamsNormal : std::uint8_t
{
    R,
    X1,
    X2,
    X3,
    Mr,
    Mx1,
    Mx2,
    Mx3
};
inline constexpr std::array<std::string_view, 8> FATIGUE_PARAMS_NORMAL_WORDS{
    "RadiusVector",         "Axis1",         "Axis2",         "Axis3",
    "ReversedRadiusVector", "ReversedAxis1", "ReversedAxis2", "ReversedAxis3"};

// The method of the PDF of the rainflow ranges of a random vibration analysis (the ATYPE field of the VIBFTG line).
enum class FatigueParamsSpectrumMethod : std::uint8_t
{
    Dirlik,
    Narrow,
    Stein,
    Sines
};
inline constexpr std::array<std::string_view, 4> FATIGUE_PARAMS_SPECTRUM_WORDS{"DirlikMethod", "NarrowBand",
                                                                               "SteinbergModel", "SinesCorrection"};

// How the number of cycles to failure of a stress bin is calculated (the NCALC field): from its middle, its upper
// limit, or the average over the bin. The format spells it UPR in one place and UPPER in another: both are read and
// kept as written.
enum class FatigueParamsLifeMethod : std::uint8_t
{
    Avg,
    Mid,
    Upr,
    Upper
};
inline constexpr std::array<std::string_view, 4> FATIGUE_PARAMS_LIFE_METHOD_WORDS{"Average", "MidSurface",
                                                                                  "UniversalSlopes", "UpperKind"};

// A number the format does not describe, kept as the deck wrote it: an integer or a real.
using FatigueParamsNumber = std::variant<std::int32_t, double>;

// ---------------------------------------------------------------------------------------------------------------------
// The lines of the card, each with the defaults of the format reference stored resolved.
// ---------------------------------------------------------------------------------------------------------------------

// The STRESS or STRAIN line. A RECOVER that is blank is CORNER for LOC NODA or NODE and CENTER for LOC ELEM: it depends
// on LOC, so it is stored resolved and always written. The NANGLE, on the line below the keyword, is the number of
// angles of a critical plane analysis.
struct FatigueParamsResults
{
    FatigueParamsResultKind kind = FatigueParamsResultKind::Stress;
    FatigueParamsCombination combination = FatigueParamsCombination::AbsMaxPr;
    FatigueParamsCorrection correction = FatigueParamsCorrection::None;
    FatigueParamsPlasticity plasticity = FatigueParamsPlasticity::Neuber;
    FatigueParamsLocation location = FatigueParamsLocation::Noda;
    int interpolation = 0; // INTERP: 0 uses the highest curve of a multi-curve correction, 1 extrapolates
    FatigueParamsRecovery recovery = FatigueParamsRecovery::Corner;
    bool surfaceResolved = false; // SRESOLVE YES: a two dimensional stress state of the 3D solids
    int angles = 36;              // NANGLE: 1 to 360

    [[nodiscard]] bool operator==(const FatigueParamsResults&) const noexcept = default;
};

// The FOS line: the factor of safety analysis. LIFE has no default in the format of SOL 101, 103 and 112 and 1.0 in the
// format of SOL 108 and 111, so a blank LIFE stays blank.
struct FatigueParamsSafety
{
    FatigueParamsSafetyOption option = FatigueParamsSafetyOption::Life;
    std::optional<double> life;
    double accuracy = 1.0;      // BACKACC: the accuracy of the back calculation, in percent of the target life
    double maximumFactor = 5.0; // MAXFAC
    double minimumFactor = 0.2; // MINFAC

    [[nodiscard]] bool operator==(const FatigueParamsSafety&) const noexcept = default;
};

// The SPOTW line: the fatigue of spot welds.
struct FatigueParamsSpotWeld
{
    FatigueParamsCombination combination = FatigueParamsCombination::Standard;
    FatigueParamsCorrection correction = FatigueParamsCorrection::None;
    int angles = 18; // NANGLE: 10 to 360
    int location =
        0; // SWLOC: 0 the worst case, 1 the worst angle of each location, 2 the worst location of each angle, 3 all
    int middleSheets = 1; // MIDDLE: 1 processes the middle sheets of a weld of more than two sheets
    int torsion = 0;      // TORSION: 1 calculates the torsion

    [[nodiscard]] bool operator==(const FatigueParamsSpotWeld&) const noexcept = default;
};

// The SEAMW line: the fatigue of seam welds. RESENT has the one word STRESS, so it is not kept.
struct FatigueParamsSeamWeld
{
    FatigueParamsCombination combination = FatigueParamsCombination::AbsMaxPr;
    FatigueParamsCorrection correction = FatigueParamsCorrection::None;
    int thickness = 0; // THICK: 1 applies a thickness correction
    FatigueParamsSeamLocation location = FatigueParamsSeamLocation::Node;

    [[nodiscard]] bool operator==(const FatigueParamsSeamWeld&) const noexcept = default;
};

// The MULTI line: the biaxial and multiaxial assessment. The format describes MMTHD only; the seven numbers after it
// (NONLWR, NONUPR, BIAXLWR, BIAXMID, BIAXUPR, ZERO and the GATE of the line below) have a place in the layout and no
// text, so they are kept as the deck wrote them and as optional, and the card makes nothing of them.
struct FatigueParamsMulti
{
    std::optional<FatigueParamsMultiMethod> method; // blank: the format gives no default
    std::optional<FatigueParamsNumber> nonProportionalityLower;
    std::optional<FatigueParamsNumber> nonProportionalityUpper;
    std::optional<FatigueParamsNumber> biaxialLower;
    std::optional<FatigueParamsNumber> biaxialMiddle;
    std::optional<FatigueParamsNumber> biaxialUpper;
    std::optional<FatigueParamsNumber> zero;
    std::optional<FatigueParamsNumber> gate; // on the line below the keyword

    [[nodiscard]] bool operator==(const FatigueParamsMulti&) const noexcept = default;
};

// The NAVG line: the averaging of the grid point stresses (the controls of the SURFACE and VOLUME commands).
struct FatigueParamsAverage
{
    FatigueParamsAveraging method = FatigueParamsAveraging::Topological;
    int output = 0; // OUTPUT: -1 the element system, 0 the basic system, or the number of a coordinate system
    FatigueParamsNormal normal = FatigueParamsNormal::R;

    [[nodiscard]] bool operator==(const FatigueParamsAverage&) const noexcept = default;
};

// The VIBFTG line (SOL 108 and 111): the parameters of the vibration fatigue. A blank CLIPLVL is MAXSTR, a blank
// MAXPEAK half of CLIPLVL, and a blank STRBINS 1280 for ATYPE SINES and 32 for the others: each depends on another
// field, so they are stored resolved and always written.
struct FatigueParamsVibration
{
    FatigueParamsSpectrumMethod method = FatigueParamsSpectrumMethod::Dirlik;
    double maximumStress = 10.0;                                       // MAXSTR, in numbers of RMS levels
    double clipLevel = 10.0;                                           // CLIPLVL
    double maximumPeak = 5.0;                                          // MAXPEAK
    int bins = 32;                                                     // STRBINS
    double maximumFrequency = 99.9;                                    // MAXFREQ: percent of the first spectral moment
    FatigueParamsLifeMethod lifeMethod = FatigueParamsLifeMethod::Avg; // NCALC

    [[nodiscard]] bool operator==(const FatigueParamsVibration&) const noexcept = default;
};

// One FTGPARM card: the parameters of a fatigue analysis (type, combination of the stresses, mean stress correction,
// location of the results, and the lines for rainflow counting, certainty of survival, factor of safety, spot welds,
// seam welds, multiaxial assessment, nodal averaging, and vibration fatigue).
//
// Card layout, SOL 101, 103, 112:
//   FTGPARM  ID  TYPE  FACTOR  NTHRD  LOGLVL  LAYER                                      fields 1 to 6
//            STRESS or STRAIN  COMB  CORR  PLAST  LOC  INTERP  RECOVER  SRESOLVE         the line, then NANGLE in field
//            3 of the next RAINFLOW  RTYPE CERTNTY  SURV FOS  OPTION  LIFE  BACKACC  MAXFAC  MINFAC SPOTW  COMB  CORR
//            NANGLE  SWLOC  MIDDLE  TORSION SEAMW  COMB  CORR  THICK  LOCSM  RESENT MULTI  MMTHD  NONLWR  NONUPR
//            BIAXLWR  BIAXMID  BIAXUPR  ZERO              the line, then GATE in field 3 of the next NAVG  MTHD  OUTPUT
//            NORMAL
// Card layout, SOL 108, 111:
//   FTGPARM  ID  TYPE  (blank)  (blank)  LOGLVL  LAYER                                   fields 1 to 6
//            STRESS  COMB  CORR  PLAST  LOC  (blank)  RECOVER                              the same line with fewer
//            fields FOS  (blank)  LIFE                                                            the same line with
//            LIFE alone VIBFTG  ATYPE  MAXSTR  CLIPLVL  MAXPEAK  STRBINS  MAXFREQ  NCALC              only in this
//            format
// The format of SOL 108 and 111 puts the same fields in the same places as the other and has fewer of them, so one card
// reads both: the lines of the one are read from the fields of the other (a blank TYPE is SN in both), and only the
// VIBFTG line is its own. The card cannot know the solution; a line of the other format is not refused.
//
// Each line starts with its keyword in field 2 of a line (the STRESS and STRAIN words are the flag of one line: both on
// a card is an error) and appears once; the lines are written in the order of the format. The defaults are
// static (TYPE SN, FACTOR 1.0, LOGLVL 0, LAYER 0, and those of the lines above) and stored resolved, and a blank NTHRD
// stays blank (it means the SMP setting of the job); those that depend on another field are said in the line that has
// them.
//
// A COMB of the SPOTW line is any of the words, of the SEAMW line ABSMAXPR or CRITICAL; a CORR of the SPOTW line is
// NONE, FKM or SIMPLE, of the SEAMW line NONE or FKM; the STRESS line has no STNDRD combination and no SIMPLE
// correction (the format makes both spot weld words). The STRAIN flag is not valid for TYPE SN (a strain-life input for
// a stress-life analysis). The OUTPUT of the NAVG line is the number of a coordinate system when it is positive: a
// Coordinate reference, so a renumbering of the systems follows it.
//
// The ID is unique among the FTGPARM cards (IdSpace::PerCardType, category Other: a FATIGUE command of the Case Control
// selects it).
//
// The values control the fatigue calculation of another program: a linear analysis does not use them. They are kept,
// each under a name that says what it is, so that a deck is written back as it was read.
struct FatigueParamsCard
{
    base::EntityId id = 0;
    FatigueParamsType type = FatigueParamsType::Sn;
    double factor = 1.0;        // the global scale factor of the combined stress
    std::optional<int> threads; // NTHRD: blank is the SMP setting, 0 all the available threads
    int logLevel = 0;           // LOGLVL: 0 to 5
    int layer = 0;              // LAYER: 0 the worst layer of a shell, 1 the top, 2 the bottom
    std::optional<FatigueParamsResults> results;
    bool rainflow = false;           // the RAINFLOW line (RTYPE LOAD)
    std::optional<double> certainty; // the CERTNTY line: SURV, the certainty of survival in percent (50 when blank)
    std::optional<FatigueParamsSafety> safety;
    std::optional<FatigueParamsSpotWeld> spotWeld;
    std::optional<FatigueParamsSeamWeld> seamWeld;
    std::optional<FatigueParamsMulti> multi;
    std::optional<FatigueParamsAverage> average;
    std::optional<FatigueParamsVibration> vibration;
};

} // namespace oor::model
