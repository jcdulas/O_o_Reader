#include "oor/results/ModuleInfo.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("results module reports its name", "[results]")
{
    REQUIRE(oor::results::moduleName() == "results");
}
