#include "oor/deck/ModuleInfo.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("deck module reports its name", "[deck]")
{
    REQUIRE(oor::deck::moduleName() == "deck");
}
