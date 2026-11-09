#pragma once

#include <chrono>

namespace oor::base
{

// When the thread that watches a long operation calls the `progress=` callable: at most ten times per
// second (a call at least INTERVAL after the start and after the previous call), then once more at the end to report
// the final state, and never for an operation that ends within INTERVAL of its start, which is not reported at all.
// The watcher asks with the time of its own clock, so the rules are tested without waiting.
class ProgressSchedule
{
public:
    using Clock = std::chrono::steady_clock;
    static constexpr std::chrono::milliseconds INTERVAL{100};

    explicit ProgressSchedule(Clock::time_point start) noexcept : m_start(start), m_last(start) {}

    // At a wake-up of the watcher while the operation runs: true when a call is due now (the call is then counted).
    [[nodiscard]] bool isDue(Clock::time_point now) noexcept
    {
        if (now - m_last < INTERVAL)
            return false;
        m_last = now;
        return true;
    }

    // When the operation has ended: true when the end is reported (the operation lasted INTERVAL or more).
    [[nodiscard]] bool reportsEnd(Clock::time_point now) const noexcept { return now - m_start >= INTERVAL; }

private:
    Clock::time_point m_start;
    Clock::time_point m_last;
};

} // namespace oor::base
