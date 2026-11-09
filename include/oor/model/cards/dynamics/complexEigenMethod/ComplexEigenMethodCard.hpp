#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

// METHOD of a complex eigenvalue extraction. INV, HESS and CLAN are the methods the METHOD field lists; IRAM
// is a method the format accepts although that field does not list it; DET is the determinant method that the format of
// EIGP names (poles come from the EIGP cards of the same ID).
enum class ComplexMethod : std::uint8_t
{
    Inv,  // INV: inverse power
    Hess, // HESS: upper Hessenberg
    Clan, // CLAN: complex Lanczos
    Iram, // IRAM: implicitly restarted Arnoldi
    Det   // DET: determinant method
};

// NORM of a complex eigenvalue extraction: how the eigenvectors are scaled.
enum class ComplexNorm : std::uint8_t
{
    Max,  // MAX: the largest component is one (the default)
    Point // POINT: the component of point G and component C is one
};

// One continuation line of an EIGC card: a search region of the complex plane (or, for CLAN, a shift point), in the
// field layout of the method of the card.
//
// Region form (INV, DET, HESS, IRAM):  ALPHAAJ OMEGAAJ ALPHABJ OMEGABJ LJ NEJ NDJ   (fields 9 to 15 of the line)
// Block Lanczos form (CLAN):           ALPHAAJ OMEGAAJ MBLKSZ IBLKSZ KSTEPS (blank) NJI
// A member of the other form holds its default and is not written. A blank NDJ of an INV region is 3 * NEJ (table
// 9-11): stored resolved. 0 is a blank NEJ or NDJ.
struct ComplexEigenMethodRegion
{
    double alphaA = 0.0; // the real part of the region's first end point, or of the shift point
    double omegaA = 0.0; // the imaginary part
    double alphaB = 0.0; // the second end point (region form)
    double omegaB = 0.0;
    double regionWidth = 1.0;            // the width of the region (region form)
    std::int32_t estimatedModeCount = 0; // the estimated number of roots in the region (region form)
    double maximumBlockSize = 7.0;       // the maximum block size (block Lanczos; a real by the format reference)
    double initialBlockSize = 2.0;       // the initial block size (block Lanczos; a real by the format reference)
    std::int32_t solveInterval = 5;      // the frequency of solves (block Lanczos)
    std::int32_t desiredModeCount = 0;   // the number of roots wanted
};

// One EIGC card: the data of a complex eigenvalue extraction (selected by CMETHOD in Case Control).
//
// Card layout:
//   EIGC  SID  METHOD  NORM  G  C  E  ND0
//         ALPHAAJ  OMEGAAJ  ALPHABJ  OMEGABJ  LJ  NEJ  NDJ            (one line per search region)
// ND0 is the number of roots wanted when there is no continuation line; with lines NDJ is on them (the card keeps what
// it was given and does not refuse both). A blank E is the default of the method: nothing is stored for it. G and C
// are the point of NORM = POINT, the format reference requires them there and nowhere else.
struct ComplexEigenMethodCard
{
    base::EntityId id = 0;
    ComplexMethod methodName = ComplexMethod::Inv;
    ComplexNorm norm = ComplexNorm::Max;
    base::EntityId normNode = 0;                // grid or scalar point of NORM = POINT; 0 is blank
    std::int32_t normComponent = 0;             // its component; 0 is blank
    std::optional<double> convergenceTolerance; // the convergence criterion; blank is the default of the method
    std::int32_t desiredModeCount0 = 0;         // the number of roots wanted; 0 is blank
    std::vector<ComplexEigenMethodRegion> regions;
};

} // namespace oor::model
