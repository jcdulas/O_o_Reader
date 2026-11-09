#include "oor/base/ModuleInfo.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("base module reports its name", "[base]")
{
    REQUIRE(oor::base::moduleName() == "base");
}
