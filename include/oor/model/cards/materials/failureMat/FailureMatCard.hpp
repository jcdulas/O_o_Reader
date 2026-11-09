#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// One failure criterion of a MATF card (a "CRI" block). The meaning of the values depends on the criterion, see the
// MATF card: for the maximum stress criterion (1) V1 to V9 are the stress limits, for the
// maximum strain criterion (2) W1 to W9 are the strain limits, for the Hill, Hoffman and Tsai-Wu criteria (3, 4, 5) V1
// to V9 are stress limits and V10 to V12 the interactive strength constants of the last one, and so on. A blank value
// is kept absent.
struct FailureMatCriterion
{
    static constexpr std::size_t CRITERION_VALUES = 12;
    static constexpr std::size_t STRAIN_LIMIT_VALUES = 9;

    int criterion = 0; // 1 maximum stress, 2 maximum strain, 3 Hill, 4 Hoffman, 5 Tsai-Wu, 7 Hashin, 8 Puck, 10
                       // Hashin-Tape, 11 Hashin-Fabric, 13 user defined (UFAIL); SOL 700: 101 maximum equivalent
                       // stress, 102 Johnson-Cook, 103 maximum plastic strain, 104 maximum pressure, 105 CAP
    std::array<std::optional<double>, CRITERION_VALUES> values{};
    std::optional<double> failureIndexScale; // the failure index scale factor (criteria 3, 4 and 5; 1.0 when blank)
    std::array<std::optional<double>, STRAIN_LIMIT_VALUES> strainValues{};
    // The character fields of a SOL 700 criterion, in the order of the format: MTH of 102 (CONT or DISC), TYPE (SOIL or
    // ROCK) and ITER (VEC or FULL) of 105; empty when blank and for the other criteria.
    std::array<std::string, 2> words{};
};

// One MATF card: the failure model of the linear elastic material with the same ID (SOL 400 and the linear solution
// sequences with layered solids). Up to three criteria may be given; for the progressive failure (ITYPE 2 and 3) only
// the first one counts, the other two give failure indices.
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   MATF  MID   ITYPE                                                              fields 1 and 2
// then, for each criterion, three lines (the last line of the last criterion may be left out):
//   "CRI"  CRITERION  V1   V2   V3   V4   V5   V6
//          V7    V8   V9   FIND V10  V11  V12  W1
//          W2    W3   W4   W5   W6   W7   W8   W9
// and, after the lines of the first criterion, the progressive failure line when ITYPE is 2 or 3 (it may be left out to
// take the defaults):
//   "PF"   A1  A2  A3  A4  A5
// ITYPE is 0 (the default: failure indices only), 2 (gradual selective stiffness degradation) or 3 (immediate
// degradation). The format reference also names an alternate format with a packed CRI67 word, which it does not draw;
// it is not read. A1 is the residual stiffness fraction (0.01 when blank); A2 to A5 are used by the Hashin, Puck and
// Hashin-Tape criteria only (0.0 when blank). MID is the ID of the MAT1, MAT2, MAT8, MATORT or MAT9 that the card
// extends.
//
// The SOL 700 form (the SOL 700 form of MATF in the format reference) has the same lines: MID on the first
// line, ITYPE blank, one criterion from 101 to 105, no PF line. Its values sit in the cells of the V and W values of
// the same positions, which the reader fills in order: for 101 MES and DT (V1, V2); for 102 D1 to D5 and 0DOT (V1 to
// V6), TROOM (V9), TMELT (FIND), CP (V10) and the word MTH (the cell of V11); for 103 MPS, MPS-C, VOLF and DT (V1 to
// V4); for 104 PRS (V1); for 105 ALPHA, THETA, GAMMA, BETA, R and D (V1 to V6), W (V9), X0 (FIND), CBAR (V10), N (V11),
// the words TYPE and ITER (the cells of V12 and W1) and TOFF (W4). The second line of 102 and 105 starts in its third
// cell.
struct FailureMatCard
{
    static constexpr std::size_t MAX_CRITERIA = 3;
    static constexpr std::size_t PROGRESSIVE_FAILURE_VALUES = 5;

    base::EntityId id = 0; // the material that the card extends
    int degradationKind = 0;
    std::vector<FailureMatCriterion> criteria;
    std::optional<std::array<std::optional<double>, PROGRESSIVE_FAILURE_VALUES>> progressiveFailure;
};

} // namespace oor::model
