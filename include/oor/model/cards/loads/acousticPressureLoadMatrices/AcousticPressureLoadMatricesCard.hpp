#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One ACLOAD card: the acoustic pressure load matrices that a run of an acoustic program exports (SOL 108 and 111). The
// load is selected by the DLOAD Case Control command like an RLOAD1, an RLOAD2 or an ACSRCE, so SID is a dynamic load
// set ID and shares one number space with those cards.
//
// Card layout: ACLOAD  SID  UNIT1  UNIT2  SCLR  SCLI  LSQID        fields 1 to 6
// UNIT1 and UNIT2 are Fortran unit numbers that an ASSIGN statement of the File Management Section binds to the files
// that program wrote (mapped data and property matrices): they are plain integers, not references to any card. The
// scale factor SCLR + i SCLI is complex. LSQID picks one load case of such a file that has several.
struct AcousticPressureLoadMatricesCard
{
    base::EntityId id = 0;
    base::EntityId dataFileUnit = 0;
    base::EntityId propertyFileUnit = 0;
    double scaleReal = 1.0;            // the real part of the scale factor
    double scaleImaginary = 0.0;       // the imaginary part of the scale factor
    base::EntityId fileLoadCaseId = 1; // the load case of the file
};

} // namespace oor::model
