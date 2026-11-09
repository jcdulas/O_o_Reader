#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// One CBARAO card: extra stations along a bar element where stresses and forces are recovered.
//
// Card layout (a list of stations, or equally spaced ones):
//   CBARAO  EID  SCALE  X1    X2      X3  X4  X5  X6
//   CBARAO  EID  SCALE  NPTS  X1  DELTAX
//
// EID is the ID of a CBAR element, not a new element. SCALE is "FractionalLength" (the stations are fractions of the
// bar length) or "ActualLength" (they are distances along the bar). A card is in one of the two forms, kept as written:
// `npts == 0` is the list form, with up to six stations in `listedStations`; `npts > 0` is the equally spaced form,
// with NPTS stations first, first + stationSpacing,
// ... (`first` is X1, `stationSpacing` is DELTAX) and `listedStations` empty.
struct Bar2StationsCard
{
    static constexpr std::size_t MAX_LISTED = 6;

    base::EntityId id = 0;
    std::string stationScale = "FractionalLength";
    std::vector<double> listedStations; // the listed stations
    std::int32_t stationCount = 0;      // 0: the stations are listed in listedStations
    double first = 0.0;                 // X1 of the equally spaced form
    double stationSpacing = 0.0;        // DELTAX of the equally spaced form

    [[nodiscard]] bool isSpaced() const noexcept { return stationCount > 0; }
    // The stations of either form.
    [[nodiscard]] std::vector<double> positions() const
    {
        if (!isSpaced())
            return listedStations;
        std::vector<double> stations;
        stations.reserve(static_cast<std::size_t>(stationCount));
        for (std::int32_t index = 0; index < stationCount; ++index)
            stations.push_back(first + stationSpacing * index);
        return stations;
    }
};

} // namespace oor::model
