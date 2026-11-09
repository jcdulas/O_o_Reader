#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/NumberFormatter.hpp"
#include "oor/base/text/NumberParser.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <random>
using oor::base::NumberFormatter;
using oor::base::NumberParser;

TEST_CASE("model numbers have standard decimal syntax and preserve exact values", "[base][numbers]")
{
    CHECK(NumberFormatter::formatRealFree(0.1) == "0.1");
    CHECK(NumberFormatter::formatRealFree(100.0) == "100.");
    CHECK(NumberFormatter::formatRealFree(-0.5) == "-0.5");
    CHECK(NumberParser::parseReal(NumberFormatter::formatRealFree(1.0 / 3.0)) == 1.0 / 3.0);
}
TEST_CASE("the precision option rounds significant digits and reports a changed value", "[base][numbers]")
{
    const auto three = NumberFormatter::formatRealWithDigits(3.14159265358979, 3);
    CHECK(three.text == "3.14");
    CHECK_FALSE(three.isExact);
    CHECK(NumberFormatter::formatRealWithDigits(0.5, 3).isExact);
}
TEST_CASE("integers are written in decimal", "[base][numbers]")
{
    CHECK(NumberFormatter::formatInteger(0) == "0");
    CHECK(NumberFormatter::formatInteger(-1234) == "-1234");
}
TEST_CASE("nonfinite model numbers cannot be written", "[base][numbers]")
{
    CHECK_THROWS_AS(NumberFormatter::formatRealFree(std::nan("")), oor::base::UsageError);
    CHECK_THROWS_AS(NumberFormatter::formatRealFree(std::numeric_limits<double>::infinity()), oor::base::UsageError);
}
TEST_CASE("exact model reals round trip over a wide range of magnitudes", "[base][numbers][property]")
{
    std::mt19937_64 generator(12345);
    std::uniform_real_distribution<double> mantissa(-10.0, 10.0);
    std::uniform_int_distribution<int> exponent(-250, 250);
    for (int sample = 0; sample < 20000; ++sample)
    {
        const double value = mantissa(generator) * std::pow(10.0, exponent(generator));
        CHECK(NumberParser::parseReal(NumberFormatter::formatRealFree(value)) == value);
    }
}
