#include "oor/io/ModuleInfo.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("io module reports its name", "[io]")
{
    REQUIRE(oor::io::moduleName() == "io");
}
