#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace oor::model
{

// The STATIC line: the static strength and the common parameters of the fatigue material.
struct FatigueMatStatic
{
    std::optional<double> yieldStrength;
    std::optional<double> ultimateTensileStrength;
    std::optional<int> materialCode; // the material code of the table (SOLs 101, 103, 112)
    std::string
        curveKind; // MEAN, AMEAN, RRATIO, ARRATIO, LIFE, ALIFE (SOLs 101, 103, 112) or AMPL, RANGE (108, 111); blank
    std::optional<double> testStressRatio;     // the R-ratio of the test that made the S-N curve (-1.0 when blank)
    std::optional<double> standardError;       // the standard error of log(N) (0.1 when blank)
    std::optional<double> plasticPoissonRatio; // the plastic Poisson's ratio (0.5 when blank)
    std::optional<double> areaReduction;       // the percentage reduction in area, on the second line
};

// An S-N curve: the line of a SN, SNS1, SNS2, SNBR1 or SNBR0 keyword and the three optional lines after it.
struct FatigueMatCurve
{
    // SRI1 B1 NC1 B2 NFC SE BTHRESH
    std::array<std::optional<double>, 7> curve{};
    // M1 M2 M3 M4 MSS RTHICK NTHICK (the lines of the SOLs 101, 103, 112 only)
    std::array<std::optional<double>, 7> meanStress{};
    // SF-FXY DE-FXY TE-FXY SF-MXY DE-MXY TE-MXY, then SF-FZ DE-FZ TE-FZ SF-MZ DE-MZ TE-MZ (spot welds)
    std::array<std::optional<double>, 12> spotWeld{};
};

// The EN line (an epsilon-N curve) and the line after it.
struct FatigueMatStrainLife
{
    // SF B C EF N' K' NC (SOLs 101, 103, 112) or GAMMA (SOLs 108, 111), the seventh value
    std::array<std::optional<double>, 7> curve{};
    // SEE SEP SEC NE, and TOLER (SOLs 108, 111; the eighth field of the line)
    std::array<std::optional<double>, 5> errors{};
};

// A run of material IDs of the MATID line: a single ID (first == last) or "first THRU last".
struct FatigueMatIdRun
{
    base::EntityId first = 0;
    base::EntityId last = 0;
};

// The MATID line: the elastic constants and the list of the MAT1 that the MATFTG serves.
struct FatigueMatMaterials
{
    std::optional<double> youngsModulus;
    std::optional<double> poissonRatio;
    std::vector<FatigueMatIdRun> matIds;
};

// One MATFTG card: the fatigue properties of an isotropic metal (SOLs 101, 103, 112 in the time domain, SOLs 108 and
// 111 in the frequency domain).
//
// Card layout (the fields are numbered across the lines, eight to a line; the format reference draws a format for each
// family of solutions which differ in the optional values only, so one reader serves both):
//   MATFTG  MID    CNVRT  MATOUT                                                    fields 1 to 3 (MATOUT: SOLs 108 and
//   111)
// then blocks of lines, each starting on a line of its own with a keyword in the first field of the line, and in any
// order:
//   "STATIC"  YS  UTS  CODE  TYPE  RR  SE  MP                                       then, on the next line, RA (second
//   field) "SN" (or "SNS1", "SNS2", "SNBR1", "SNBR0")  SRI1  B1  NC1  B2  NFC  SE  BTHRESH
//             M1  M2  M3  M4  MSS  RTHICK  NTHICK                                   (optional lines with a blank first
//             field) SF-FXY DE-FXY TE-FXY SF-MXY DE-MXY TE-MXY SF-FZ  DE-FZ  TE-FZ  SF-MZ  DE-MZ  TE-MZ
//   "TABLE"   VALUE1 TID1 VALUE2 TID2 VALUE3 TID3                                   (three pairs to a line)
//   "MMPDS"   A1  A2  A3  A4
//   "EN"      SF  B  C  EF  N'  K'  NC/GAMMA                                        then SEE SEP SEC NE (blank first
//   field)
//                                                                                   and TOLER in its eighth field
//   "MATID"   E   NU                                                                then the MIDi, seven to a line,
//   with THRU
// CNVRT is the conversion factor of the stress units (1.0 when blank), MATOUT (0) the debug flag of the SOLs 108 and
// 111. The STATIC line is required. MID is the ID of the MAT1 that the card serves, unless there is a MATID line, in
// which case MID is the ID of the card alone and the MIDi name the MAT1 that it serves. A blank value is kept absent.
struct FatigueMatCard
{
    static constexpr std::size_t CURVES = 5; // SN, SNS1, SNS2, SNBR1, SNBR0

    base::EntityId id = 0;
    double stressUnitFactor = 1.0;
    int debugOutput = 0;
    FatigueMatStatic fixed;
    std::array<std::optional<FatigueMatCurve>, CURVES> curves{};
    std::optional<std::vector<std::pair<double, base::EntityId>>> table; // (VALUEi, TIDi)
    std::optional<std::array<std::optional<double>, 4>> handbookCoefficients;
    std::optional<FatigueMatStrainLife> strainLife;
    std::optional<FatigueMatMaterials> materials;
};

} // namespace oor::model
