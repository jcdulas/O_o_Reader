#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// How the excitation frequencies of a FREQ3 card are spread between two modal frequencies.
enum class FrequencySpacing : std::uint8_t
{
    Linear, // LINEAR (the default)
    Log     // LOG
};

// One FREQ3 card: excitation frequencies between the modal frequencies of a range (modal frequency response).
//
// Card layout: FREQ3  SID  F1  F2  TYPE  NEF  CLUSTER
// NEF frequencies fall between each two consecutive modal frequencies of [F1, F2] (and between F1 and the first one,
// the last one and F2), spread linearly or logarithmically and drawn towards the ends of the subrange by CLUSTER.
// A blank F2 is F1 (the format reference default): it is stored resolved and always written.
struct FreqBetweenModes3Card
{
    base::EntityId id = 0;
    double frequency1 = 0.0;
    double frequency2 = 0.0;
    FrequencySpacing kind = FrequencySpacing::Linear;
    std::int32_t frequencyCount = 10;
    double clustering = 1.0;
};

} // namespace oor::model
