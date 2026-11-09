#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/loads/ElementRuns.hpp"

#include <limits>
#include <vector>

namespace oor::model
{

// One TEMPP1 card: the temperature field of plate, membrane and composite elements, as an average temperature and a
// gradient through the thickness.
//
// Card layout: TEMPP1  SID  EID1  TBAR  TPRIME  T1  T2
//                     EID2  EID3  EID4  EID5  EID6  EID7  -etc.-
// or, on the continuation lines, ranges: EID2  "THRU"  EIDi  EIDj  "THRU"  EIDk
// The elements are EID1, then the continuation lines, kept as written: single elements and THRU ranges, in order. TBAR
// and TPRIME are 0.0 when blank. T1 and T2 (the temperatures at the stress points of the property) are
// NaN when blank: with both blank the program computes them from TBAR and TPRIME.
struct PlateTemperatureLoadCard
{
    static constexpr double BLANK_TEMPERATURE = std::numeric_limits<double>::quiet_NaN();

    base::EntityId id = 0;
    double meanTemperature = 0.0;
    double temperatureGradient = 0.0;
    double temperature1 = BLANK_TEMPERATURE;
    double temperature2 = BLANK_TEMPERATURE;
    std::vector<ElementRun> elements; // the first one is EID1, a single element
};

} // namespace oor::model
