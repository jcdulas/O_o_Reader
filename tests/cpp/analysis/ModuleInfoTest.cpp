#include "oor/analysis/ModuleInfo.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("analysis module reports its name", "[analysis]")
{
    REQUIRE(oor::analysis::moduleName() == "analysis");
}
