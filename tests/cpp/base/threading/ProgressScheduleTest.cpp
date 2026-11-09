#include "oor/base/threading/ProgressSchedule.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>

using oor::base::ProgressSchedule;
using namespace std::chrono_literals;

TEST_CASE("an operation shorter than a tenth of a second is never reported", "[base][threading][progress]")
{
    const ProgressSchedule::Clock::time_point start{};
    ProgressSchedule schedule(start);
    REQUIRE_FALSE(schedule.isDue(start + 50ms));
    REQUIRE_FALSE(schedule.isDue(start + 99ms));
    REQUIRE_FALSE(schedule.reportsEnd(start + 99ms));
}

TEST_CASE("progress is called at most ten times a second, then once at the end", "[base][threading][progress]")
{
    const ProgressSchedule::Clock::time_point start{};
    ProgressSchedule schedule(start);
    int calls = 0;
    // The watcher wakes every 7 ms for two seconds: a call at every 105 ms (the first wake-up a tenth of a second or
    // more after the last call), 19 calls before the end.
    auto last = start;
    for (auto now = start + 7ms; now < start + 2s; now += 7ms)
    {
        if (schedule.isDue(now))
        {
            ++calls;
            REQUIRE(now - last >= 100ms);
            last = now;
        }
    }
    REQUIRE(calls == 19);
    REQUIRE(schedule.reportsEnd(start + 2s));
    REQUIRE(schedule.reportsEnd(start + 100ms));
}
