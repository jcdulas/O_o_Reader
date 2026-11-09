#include "oor/query/ModuleInfo.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("query module reports its name", "[query]")
{
    REQUIRE(oor::query::moduleName() == "query");
}
