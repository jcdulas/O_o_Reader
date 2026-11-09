#include "oor/base/text/NumberParser.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <string>

using oor::base::NumberParser;

TEST_CASE("integers", "[base][numbers]")
{
    REQUIRE(NumberParser::parseInteger("0") == 0);
    REQUIRE(NumberParser::parseInteger("  42  ") == 42);
    REQUIRE(NumberParser::parseInteger("-7") == -7);
    REQUIRE(NumberParser::parseInteger("+7") == 7);
    REQUIRE(NumberParser::parseInteger("0007") == 7);
    REQUIRE(NumberParser::parseInteger("9223372036854775807") == std::numeric_limits<std::int64_t>::max());
    REQUIRE(NumberParser::parseInteger("-9223372036854775808") == std::numeric_limits<std::int64_t>::min());
    REQUIRE_FALSE(NumberParser::parseInteger("9223372036854775808").has_value());
    REQUIRE_FALSE(NumberParser::parseInteger("-9223372036854775809").has_value());
    for (const char* bad : {"", " ", "+", "-", "1.", "1.0", "1E3", "1+3", "12a", "1 2", "--1", "0x10"})
        REQUIRE_FALSE(NumberParser::parseInteger(bad).has_value());
}

TEST_CASE("decimal reals with standard exponents", "[base][numbers]")
{
    REQUIRE(NumberParser::parseReal("1.5") == 1.5);
    REQUIRE(NumberParser::parseReal("-1.5") == -1.5);
    REQUIRE(NumberParser::parseReal("+.5") == 0.5);
    REQUIRE(NumberParser::parseReal("5.") == 5.0);
    REQUIRE(NumberParser::parseReal("1.5E3") == 1500.0);
    REQUIRE(NumberParser::parseReal("1.5e-3") == 1.5e-3);
    REQUIRE(NumberParser::parseReal("  2.5  ") == 2.5);
}

TEST_CASE("malformed reals are rejected, not thrown", "[base][numbers]")
{
    for (const char* bad : {"",      " ",      ".",     "+",      "E3",    "1.0E",  "1.0E+", "1..0", "1.0.0",
                            "--1.0", "1.0 E3", "1 .0",  "abc",    "1,0",   "0x10",  "NaN",   "inf",  "1.0++3",
                            "1e999", "-1e999", "1.5D3", "1.5d+3", "1.5+3", "1.5-3", "1.-3",  ".5-3"})
        REQUIRE_FALSE(NumberParser::parseReal(bad).has_value());
    // Longer than any field: refused, no allocation.
    REQUIRE_FALSE(NumberParser::parseReal(std::string(100, '1')).has_value());
}
