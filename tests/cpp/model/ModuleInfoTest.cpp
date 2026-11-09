#include "oor/model/ModuleInfo.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("model module reports its name", "[model]")
{
    REQUIRE(oor::model::moduleName() == "model");
}
