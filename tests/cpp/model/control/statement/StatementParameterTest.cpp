#include "oor/model/control/statement/StatementParameter.hpp"

#include <catch2/catch_test_macros.hpp>
using namespace oor::model;
TEST_CASE("statement parameters preserve exact neutral names and values", "[control][statement]")
{
    const auto flag = StatementParameter::flag("NoExecution");
    REQUIRE(flag.format() == "NoExecution");
    REQUIRE(flag.isNamed("NoExecution"));
    REQUIRE_FALSE(flag.isNamed("NoEx"));
    REQUIRE_FALSE(flag.isNamed("noExecution"));
    REQUIRE(StatementParameter::withValue("FileUnit", "92").format() == "FileUnit=92");
    REQUIRE(StatementParameter::withQuotedValue("ResultsFile", "a b.bin").format() == "ResultsFile='a b.bin'");
}
TEST_CASE("parameter lookup uses exact names", "[control][statement]")
{
    std::vector<StatementParameter> parameters{StatementParameter::withValue("FileUnit", "92")};
    REQUIRE(findParameter(parameters, "FileUnit") == &parameters.front());
    REQUIRE(findParameter(parameters, "File") == nullptr);
    REQUIRE(findParameter(parameters, "fileunit") == nullptr);
}
