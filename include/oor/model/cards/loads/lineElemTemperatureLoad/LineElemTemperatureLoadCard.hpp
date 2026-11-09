#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/loads/ElementRuns.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace oor::model
{

// One TEMPRB card: the temperature field of one-dimensional elements (CBAR, CBEAM, CBEND, CROD, CTUBE, CONROD).
//
// Card layout: TEMPRB  SID  EID1  TA    TB    TP1A  TP1B  TP2A  TP2B
//                     TCA  TDA   TEA   TFA   TCB   TDB   TEB   TFB
//                     EID2 EID3  EID4  EID5  EID6  EID7  -etc.-
// or, on the second continuation, ranges: EID2  "THRU"  EIDi  EIDj  "THRU"  EIDk
// The elements are EID1, then the second continuation lines, kept as written: single elements and THRU ranges.
//
// TA and TB (the temperatures of the neutral axis at the two ends) and the four gradients are 0.0 when blank. The eight
// point temperatures (C, D, E, F at end A, then at end B) are for stress recovery of bars and beams:
// when none is given the program assumes linear gradients, so the card keeps them as NaN; when at least one is given
// the blank ones take TA (end A) or TB (end B), so they are stored resolved and the eight are always written.
struct LineElemTemperatureLoadCard
{
    static constexpr double NO_POINT_TEMPERATURE = std::numeric_limits<double>::quiet_NaN();

    base::EntityId id = 0;
    double temperatureA = 0.0;
    double temperatureB = 0.0;
    std::array<double, 2> gradientY{0.0, 0.0}; // TP1A, TP1B
    std::array<double, 2> gradientZ{0.0, 0.0}; // TP2A, TP2B
    std::array<double, 4> pointsA{NO_POINT_TEMPERATURE, NO_POINT_TEMPERATURE, NO_POINT_TEMPERATURE,
                                  NO_POINT_TEMPERATURE}; // TCA, TDA, TEA, TFA
    std::array<double, 4> pointsB{NO_POINT_TEMPERATURE, NO_POINT_TEMPERATURE, NO_POINT_TEMPERATURE,
                                  NO_POINT_TEMPERATURE}; // TCB, TDB, TEB, TFB
    std::vector<ElementRun> elements;                    // the first one is EID1, a single element

    // True when the card has point temperatures (all eight are then numbers).
    [[nodiscard]] bool hasPointTemperatures() const noexcept
    {
        for (const double value : pointsA)
        {
            if (!std::isnan(value))
                return true;
        }
        for (const double value : pointsB)
        {
            if (!std::isnan(value))
                return true;
        }
        return false;
    }
};

} // namespace oor::model
