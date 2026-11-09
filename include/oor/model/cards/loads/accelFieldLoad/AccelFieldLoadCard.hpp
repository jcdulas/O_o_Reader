#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace oor::model
{

// The DIR field of an ACCEL: along which axis of the coordinate system the scale factor of the load varies.
enum class AccelFieldLoadDirection : std::uint8_t
{
    X,
    Y,
    Z
};
inline constexpr std::array<std::string_view, 3> ACCEL_FIELD_DIRECTION_WORDS{"XDirection", "YDirection", "ZDirection"};

// One ACCEL card: a static acceleration load that varies over the model. The acceleration of a grid point is the vector
// (N1, N2, N3) times a factor that is interpolated, along the DIR axis of coordinate system CID, in the table of
// (LOCi, VALi) pairs; a grid point beyond the table takes the first or the last value. The load does not act on the
// mass of scalar points.
//
// Card layout: ACCEL  SID  CID  N1  N2  N3  DIR  (blank)  (blank)         fields 1 to 6
//             (continuation)  LOC1  VAL1  LOC2  VAL2 ...                 pairs from field 9, as many as needed
// SID is a load set ID shared by the cards of the set (it may not be the SID of any other load entry, which
// the validation of load sets can check). CID defaults to the basic system (0). The N fields default to 0.0 and not all
// of them may be zero. DIR is X, Y or Z, and at least two pairs are required; the format does not say that the
// locations must increase, so the card does not check it. A dynamic load may name the SID through an LSEQ or the
// EXCITEID of a TLOADn or RLOADn, as for any load.
struct AccelFieldLoadCard
{
    base::EntityId id = 0;
    base::EntityId frameId = 0; // 0 is the basic coordinate system
    double directionX = 0.0;    // the acceleration vector, at least one component is not zero
    double directionY = 0.0;
    double directionZ = 0.0;
    AccelFieldLoadDirection direction = AccelFieldLoadDirection::X;
    std::vector<std::pair<double, double>> points; // (LOCi, VALi), at least two
};

} // namespace oor::model
