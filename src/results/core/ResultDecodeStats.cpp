#include "oor/results/core/ResultDecodeStats.hpp"

namespace oor::results
{

ResultDecodeStats& decodeStats() noexcept
{
    static ResultDecodeStats stats;
    return stats;
}

} // namespace oor::results
