#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One ACTRIM card: the trimmed material matrices that an acoustic program exports for a frequency response analysis
// (SOL 108 and 111), with the complex factor that scales them. The Case Control command ACTRIM = NAME selects the card.
//
// Card layout: ACTRIM  NAME  UNIT1  UNIT2  SCLR  SCLI
// NAME is one to eight letters or digits, the first a letter; it is the key of the card (no number), kept in upper
// case. UNIT1 and UNIT2 are the Fortran unit numbers of the two files that program writes, the mapped data and the
// property matrices; the FMS section connects them to files with ASSIGN statements. The files are named by two
// ASSIGN lines, and a unit number cannot be given twice, so the card refuses UNIT1 equal to UNIT2 (a reading of
// the format, not a sentence of it). SCLR and SCLI are the real and the imaginary part of
// the factor: 1.0 and 0.0 when blank.
//
// The name follows the rule of the names of the matrices (letters, digits and underscores, the first a letter): the
// format reference allows letters and digits, and the underscore is accepted as elsewhere in the library.
struct TrimmedMaterialMatricesCard
{
    static constexpr double DEFAULT_SCALE_REAL = 1.0;
    static constexpr double DEFAULT_SCALE_IMAGINARY = 0.0;

    std::string name; // upper case, up to 8 characters
    base::EntityId mappedDataUnit = 0;
    base::EntityId propertyMatrixUnit = 0;
    double scaleReal = DEFAULT_SCALE_REAL;
    double scaleImaginary = DEFAULT_SCALE_IMAGINARY;
};

} // namespace oor::model
