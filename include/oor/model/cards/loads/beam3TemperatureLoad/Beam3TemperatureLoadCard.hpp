#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/thermal/ThermalRuns.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace oor::model
{

// One TEMPB3 card: the temperature field of the three-node beam element CBEAM3. Each temperature is given at the three
// stations of the element, A, B and C, so the values are arrays of three, indexed by the station.
//
// Card layout: TEMPB3  SID    EID    T(A)   T(B)   T(C)   TPY(A) TPZ(A) TPY(B)        fields 1 to 8
//                     TPZ(B) TPY(C) TPZ(C) TC(A)  TD(A)  TE(A)  TF(A)  TC(B)         fields 9 to 16
//                     TD(B)  TE(B)  TF(B)  TC(C)  TD(C)  TE(C)  TF(C)                fields 17 to 23, 24 blank
//                     the element ID list                                              fields 25 on
// T(j) is the temperature on the neutral axis, TPY(j) and TPZ(j) the effective linear gradients in the local directions
// y and z, and TC(j) to TF(j) the temperatures at the four stress recovery points C, D, E and F of the PBEAM3: every
// one is 0.0 when blank. The format adds that when none of the eight point temperatures is given the program assumes
// linear gradients for stress recovery, which is not the same as four zeros: the card keeps them as NaN when none was
// given (as the TEMPRB does), and when at least one is given the blank ones are the 0.0 of the format, so the twelve
// are always written. The list holds CBEAM3 element IDs with THRU and BY, kept as written, and at least one is required
// (the format says so); EID is a field of its own, a single element, kept apart from the list.
struct Beam3TemperatureLoadCard
{
    static constexpr double NO_POINT_TEMPERATURE = std::numeric_limits<double>::quiet_NaN();

    base::EntityId id = 0;
    base::EntityId elemId = 0;
    std::array<double, 3> temperature{0.0, 0.0, 0.0}; // T(A), T(B), T(C)
    std::array<double, 3> gradientY{0.0, 0.0, 0.0};   // TPY(A), TPY(B), TPY(C)
    std::array<double, 3> gradientZ{0.0, 0.0, 0.0};   // TPZ(A), TPZ(B), TPZ(C)
    std::array<double, 3> temperatureC{NO_POINT_TEMPERATURE, NO_POINT_TEMPERATURE,
                                       NO_POINT_TEMPERATURE}; // TC(A), TC(B), TC(C)
    std::array<double, 3> temperatureD{NO_POINT_TEMPERATURE, NO_POINT_TEMPERATURE, NO_POINT_TEMPERATURE};
    std::array<double, 3> temperatureE{NO_POINT_TEMPERATURE, NO_POINT_TEMPERATURE, NO_POINT_TEMPERATURE};
    std::array<double, 3> temperatureF{NO_POINT_TEMPERATURE, NO_POINT_TEMPERATURE, NO_POINT_TEMPERATURE};
    std::vector<ThermalRun> elements;

    // True when the card has point temperatures (all twelve are then numbers).
    [[nodiscard]] bool hasPointTemperatures() const noexcept
    {
        for (const std::array<double, 3>* points : {&temperatureC, &temperatureD, &temperatureE, &temperatureF})
        {
            for (const double value : *points)
            {
                if (!std::isnan(value))
                    return true;
            }
        }
        return false;
    }
};

} // namespace oor::model
