// The equations of a DEQATN: the grammar and its precedence, the functions, the integer rule (1/2 is
// 0), the canonical text, and the evaluation of one point and of many.

#include "oor/base/errors/Errors.hpp"
#include "oor/model/expressions/EquationEvaluator.hpp"
#include "oor/model/expressions/EquationSet.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using Catch::Approx;

namespace
{

EquationSet parsed(const std::string& text)
{
    EquationParse result = parseEquations(text);
    const std::string problem = result.problem ? result.problem->message : std::string();
    INFO(text);
    INFO(problem);
    REQUIRE(result.ok());
    return std::move(*result.equations);
}

// The value of an equation at a list of numbers (the arguments in order).
double valueAt(const std::string& text, std::vector<double> arguments)
{
    const EquationSet set = parsed(text);
    return evaluateEquations(set, std::span<const double>(arguments));
}

void requireProblem(const std::string& text, const std::string& fragment)
{
    const EquationParse result = parseEquations(text);
    INFO(text);
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.problem.has_value());
    INFO(result.problem->message);
    REQUIRE(result.problem->message.find(fragment) != std::string::npos);
}

} // namespace

TEST_CASE("the canonical text reads back to the same text for any placing of blanks, case and parentheses",
          "[model][expressions]")
{
    for (const std::string text : {"f(a,b)=a+b",
                                   "F( A , B ) = ( A + B )",
                                   "F(A,B)=A*(B+1)-(A-B)/2",
                                   "F(A)=-A**2",
                                   "F(A)=(-A)**2",
                                   "F(A)=A**-2",
                                   "F(A,B)=A**B**2",
                                   "F(A,B)=(A**B)**2",
                                   "F(A,B)=A-(B-1)",
                                   "F(A,B)=A-B-1",
                                   "F(A,B)=A/(B*2)",
                                   "F(A,B)=A/B*2",
                                   "F(A,B)=-(A+B)*2",
                                   "F(A,B)=(-A+B)*2",
                                   "F(A)=1.5E-3*A+2.D+2",
                                   "F(A)=MAX(A,0,-A)",
                                   "F(A)=SUM(A,A*2)+AVG(A,1)",
                                   "F(A,B)=A*-B",
                                   "F(A,B)=-A*-B",
                                   "F(A)=1/2*A",
                                   "F(A)=A+1;G=F*2"})
    {
        INFO(text);
        const std::string once = parsed(text).canonicalText();
        // The canonical text is a fixed point: it parses and prints as itself.
        const EquationSet again = parsed(once);
        REQUIRE(again.canonicalText() == once);
        // And it is the same tree: it evaluates to the same value.
        const EquationSet first = parsed(text);
        std::vector<double> arguments(first.parameters().size(), 1.7);
        for (std::size_t index = 0; index < arguments.size(); ++index)
            arguments[index] += static_cast<double>(index) * 0.3;
        REQUIRE(evaluateEquations(again, std::span<const double>(arguments)) ==
                Approx(evaluateEquations(first, std::span<const double>(arguments))));
    }
}

TEST_CASE("the operators follow the precedence of the format reference", "[model][expressions]")
{
    REQUIRE(valueAt("F(X)=-X**2", {3.0}) == Approx(-9.0));
    REQUIRE(valueAt("F(X)=(-X)**2", {3.0}) == Approx(9.0));
    REQUIRE(valueAt("F(X)=2**-X", {2.0}) == Approx(0.25));
    REQUIRE(valueAt("F(X)=2**3**2", {0.0}) == Approx(512.0)); // right to left
    REQUIRE(valueAt("F(X)=10-X-3", {2.0}) == Approx(5.0));    // left to right
    REQUIRE(valueAt("F(X)=100/X/5", {2.0}) == Approx(10.0));
    REQUIRE(valueAt("F(X,Y)=X*Y-X/Y", {6.0, 3.0}) == Approx(16.0));
    REQUIRE(valueAt("F(X,Y)=X+Y*2", {1.0, 3.0}) == Approx(7.0));
    REQUIRE(valueAt("F(X)=-X-X", {2.0}) == Approx(-4.0)); // -X-Y is (-X)-Y
    REQUIRE(valueAt("F(X)=X*-2", {3.0}) == Approx(-6.0));
}

TEST_CASE("a quantity written without a point is an integer and two integers give an integer", "[model][expressions]")
{
    // A**(1/2) is A**0, because 1/2 is 0.
    REQUIRE(valueAt("F(A)=A**(1/2)", {9.0}) == Approx(1.0));
    REQUIRE(valueAt("F(A)=A**(.5)", {9.0}) == Approx(3.0));
    REQUIRE(valueAt("F(A)=A*(7/2)", {1.0}) == Approx(3.0));
    REQUIRE(valueAt("F(A)=A*(-7/2)", {1.0}) == Approx(-3.0)); // truncated toward zero
    REQUIRE(valueAt("F(A)=A*(7./2)", {1.0}) == Approx(3.5));
    REQUIRE(valueAt("F(A)=A*(2**-1)", {1.0}) == Approx(0.0));
    REQUIRE(valueAt("F(A)=A*(2**3)", {1.0}) == Approx(8.0));
    REQUIRE(valueAt("F(A)=A/2", {7.0}) == Approx(3.5)); // a variable is real
    REQUIRE_THROWS_AS(valueAt("F(A)=A*(1/0)", {1.0}), base::UsageError);
    REQUIRE(valueAt("F(A)=A/0.", {1.0}) == std::numeric_limits<double>::infinity());
}

TEST_CASE("every function of the format reference gives its value", "[model][expressions]")
{
    const double pi = std::numbers::pi;
    REQUIRE(valueAt("F(X)=ABS(X)", {-2.5}) == Approx(2.5));
    REQUIRE(valueAt("F(X)=ACOS(X)", {0.3}) == Approx(std::acos(0.3)));
    REQUIRE(valueAt("F(X)=ACOSH(X)", {1.7}) == Approx(std::acosh(1.7)));
    REQUIRE(valueAt("F(X)=ASIN(X)", {0.3}) == Approx(std::asin(0.3)));
    REQUIRE(valueAt("F(X)=ASINH(X)", {0.3}) == Approx(std::asinh(0.3)));
    REQUIRE(valueAt("F(X)=ATAN(X)", {0.3}) == Approx(std::atan(0.3)));
    REQUIRE(valueAt("F(X,Y)=ATAN2(X,Y)", {1.0, -2.0}) == Approx(std::atan2(1.0, -2.0)));
    REQUIRE(valueAt("F(X)=ATANH(X)", {0.3}) == Approx(std::atanh(0.3)));
    REQUIRE(valueAt("F(X,Y)=ATANH2(X,Y)", {1.0, 4.0}) == Approx(std::atanh(0.25)));
    REQUIRE(valueAt("F(X,Y,Z)=AVG(X,Y,Z)", {1.0, 2.0, 6.0}) == Approx(3.0));
    REQUIRE(valueAt("F(X)=COS(X)", {0.3}) == Approx(std::cos(0.3)));
    REQUIRE(valueAt("F(X)=COSH(X)", {0.3}) == Approx(std::cosh(0.3)));
    REQUIRE(valueAt("F(P,PREF)=DB(P,PREF)", {2.0, 0.5}) == Approx(20.0 * std::log10(4.0)));
    REQUIRE(valueAt("F(X,Y)=DIM(X,Y)", {5.0, 3.0}) == Approx(2.0));
    REQUIRE(valueAt("F(X,Y)=DIM(X,Y)", {3.0, 5.0}) == Approx(0.0));
    REQUIRE(valueAt("F(X)=EXP(X)", {0.3}) == Approx(std::exp(0.3)));
    REQUIRE(valueAt("F(D,PREF)=INVDB(D,PREF)", {40.0, 2.0}) == Approx(2.0 * 100.0));
    REQUIRE(valueAt("F(X)=LOG(X)", {5.0}) == Approx(std::log(5.0)));
    REQUIRE(valueAt("F(X)=LOG10(X)", {5.0}) == Approx(std::log10(5.0)));
    REQUIRE(valueAt("F(X,Y)=LOGX(X,Y)", {2.0, 8.0}) == Approx(3.0)); // base x logarithm of y
    REQUIRE(valueAt("F(X,Y,Z)=MAX(X,Y,Z)", {1.0, 7.0, 3.0}) == Approx(7.0));
    REQUIRE(valueAt("F(X,Y,Z)=MIN(X,Y,Z)", {4.0, 7.0, 3.0}) == Approx(3.0));
    REQUIRE(valueAt("F(X,Y)=MOD(X,Y)", {7.0, 3.0}) == Approx(1.0));
    REQUIRE(valueAt("F(X,Y)=MOD(X,Y)", {-7.0, 3.0}) == Approx(-1.0)); // x - y * INT(x / y)
    REQUIRE(valueAt("F(X)=PI(X)", {2.0}) == Approx(2.0 * pi));
    REQUIRE(valueAt("F(X,Y)=RSS(X,Y)", {3.0, 4.0}) == Approx(5.0));
    REQUIRE(valueAt("F(X)=SIN(X)", {0.3}) == Approx(std::sin(0.3)));
    REQUIRE(valueAt("F(X)=SINH(X)", {0.3}) == Approx(std::sinh(0.3)));
    REQUIRE(valueAt("F(X)=SQRT(X)", {2.0}) == Approx(std::sqrt(2.0)));
    REQUIRE(valueAt("F(X,Y)=SSQ(X,Y)", {3.0, 4.0}) == Approx(25.0));
    REQUIRE(valueAt("F(X,Y)=SUM(X,Y)", {3.0, 4.0}) == Approx(7.0));
    REQUIRE(valueAt("F(X)=TAN(X)", {0.3}) == Approx(std::tan(0.3)));
    REQUIRE(valueAt("F(X)=TANH(X)", {0.3}) == Approx(std::tanh(0.3)));
    // PI alone is the number, unless an argument has the name.
    REQUIRE(valueAt("F(X)=PI*X", {2.0}) == Approx(2.0 * pi));
    REQUIRE(valueAt("F(PI)=PI*2", {5.0}) == Approx(10.0));
}

TEST_CASE("DBA and INVDBA use the weighting curve of the format reference", "[model][expressions]")
{
    // At 1000 Hz the A weighting is close to 0 dB.
    const double dba = valueAt("F(P,PREF,FQ)=DBA(P,PREF,FQ)", {2.0, 1.0, 1000.0});
    REQUIRE(dba == Approx(20.0 * std::log10(2.0)).margin(0.2));
    // INVDBA undoes the weighting and the logarithm of the format reference, which has no PREF in it.
    const double back = valueAt("F(D,PREF,FQ)=INVDBA(D,PREF,FQ)", {dba, 1.0, 1000.0});
    REQUIRE(back == Approx(2.0));
}

TEST_CASE("several equations pass intermediate variables to each other", "[model][expressions]")
{
    // The last equation gives the value; each one may use the variables the ones before it gave.
    REQUIRE(valueAt("F1(A,B)=A+B; G=F1*F1; H=G-A", {2.0, 3.0}) == Approx(23.0));
    // A variable may be given again, from its own value.
    REQUIRE(valueAt("F(A)=A+1; F=F*2", {4.0}) == Approx(10.0));
    // One that no earlier equation gave is not defined.
    requireProblem("F(A)=X; X=A", "not an argument");
    requireProblem("F(A)=A; G=H; H=A", "not an argument");
}

TEST_CASE("an equation that cannot be understood is a problem, not an exception", "[model][expressions]")
{
    requireProblem("", "empty");
    requireProblem("F(X)=", "expected a number");
    requireProblem("F(X)=X+", "expected a number");
    requireProblem("F(X)=(X", "expected \")\"");
    requireProblem("F(X)=X)", "expected \";\"");
    requireProblem("F(X)=X Y", "expected \";\"");
    requireProblem("F(X)=Z", "not an argument");
    requireProblem("F(X)=FOO(X)", "not one of the functions");
    requireProblem("F(X)=SIN(X,X)", "takes 1 argument");
    requireProblem("F(X)=ATAN2(X)", "takes 2 arguments");
    requireProblem("F(X)=PI()", "takes 1 argument");
    requireProblem("F(X,X)=X", "listed twice");
    requireProblem("F(F)=F", "also one of its arguments");
    requireProblem("F(X)=X; G(X)=X", "only the first equation");
    requireProblem("F(X)=X^2", "not part of an equation");
    requireProblem("F(X)=1.5E", "needs digits");
    requireProblem("F(X)=.", "next to digits");
    requireProblem("F(AND)=AND", "DMAP logical operator");
    requireProblem("F(X)=X$", "not part of an equation");
    requireProblem("F(X)=" + std::string(500, '('), "nested");
    requireProblem("F(X)=" + std::string(32000, 'X'), "32 000");
    std::string list = "X";
    for (int index = 0; index < 96; ++index)
        list += ",X";
    requireProblem("F(X)=SUM(" + list + ")", "takes 1 to 96 arguments");
}

TEST_CASE("one equation is evaluated at many points at once", "[model][expressions]")
{
    const EquationSet set = parsed("F(A,B)=A*B+1");
    const std::vector<double> a{1.0, 2.0, 3.0};
    const std::vector<double> b{10.0, 20.0, 30.0};
    const std::vector<EquationVariable> variables{{"a", a}, {"B", b}};
    REQUIRE(evaluateEquations(set, variables) == std::vector<double>{11.0, 41.0, 91.0});

    // An argument with one value is the same at every point.
    const std::vector<double> scalar{2.0};
    const std::vector<EquationVariable> mixed{{"A", scalar}, {"B", b}};
    REQUIRE(evaluateEquations(set, mixed) == std::vector<double>{21.0, 41.0, 61.0});
    const std::vector<EquationVariable> allScalar{{"A", scalar}, {"B", scalar}};
    REQUIRE(evaluateEquations(set, allScalar) == std::vector<double>{5.0});

    // The aggregate functions combine the arguments at each point, not the points.
    const EquationSet maximum = parsed("F(A,B)=MAX(A,B)+SUM(A,B)");
    REQUIRE(evaluateEquations(maximum, variables) == std::vector<double>{21.0, 42.0, 63.0});

    // No points, no values.
    const std::vector<double> none;
    const std::vector<EquationVariable> empty{{"A", none}, {"B", none}};
    REQUIRE(evaluateEquations(set, empty).empty());
}

TEST_CASE("arguments that do not match the equation are refused", "[model][expressions]")
{
    const EquationSet set = parsed("F(A,B)=A+B");
    const std::vector<double> one{1.0};
    const std::vector<double> two{1.0, 2.0};
    const std::vector<double> three{1.0, 2.0, 3.0};
    const std::vector<EquationVariable> missing{{"A", one}};
    REQUIRE_THROWS_AS(evaluateEquations(set, missing), base::UsageError);
    const std::vector<EquationVariable> unknown{{"A", one}, {"B", one}, {"C", one}};
    REQUIRE_THROWS_AS(evaluateEquations(set, unknown), base::UsageError);
    const std::vector<EquationVariable> twice{{"A", one}, {"a", one}, {"B", one}};
    REQUIRE_THROWS_AS(evaluateEquations(set, twice), base::UsageError);
    const std::vector<EquationVariable> shapes{{"A", two}, {"B", three}};
    REQUIRE_THROWS_AS(evaluateEquations(set, shapes), base::UsageError);
    std::vector<double> tooMany{1.0, 2.0, 3.0};
    REQUIRE_THROWS_AS(evaluateEquations(set, std::span<const double>(tooMany)), base::UsageError);
}

// The robustness test of the parser: the equations are text from a file, so any bytes may arrive. A fixed seed and a
// fixed number of tries keep it quick and repeatable (the project has no long fuzz runs).
TEST_CASE("random and damaged equations never crash the parser and the ones that parse print and read back",
          "[model][expressions][robustness]")
{
    const std::vector<std::string> corpus{
        "F1(A, B, C, D, R) = A + B*C - (D ** 3 + 10.0) + SIN(PI(1)*R) + A**2/(B - C); F = A + B - F1*D",
        "F(X)=-X**2+3*X-1",
        "F(A,B)=MAX(A,B)+SUM(A,B)/2.0E-3",
        "F(P1,P2)=DB(P1)-DBA(P2)+ATAN2(P1,P2)",
        "F(X)=((((X))))",
    };
    const std::string alphabet = "ABXYZ019 .,;()=+-*/E$'\"^_dD\t";

    std::uint64_t state = 0x9E3779B97F4A7C15ULL;
    const auto next = [&state](std::size_t bound)
    {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<std::size_t>((state >> 33) % bound);
    };

    std::size_t parsedCount = 0;
    for (int attempt = 0; attempt < 4000; ++attempt)
    {
        std::string text = corpus[next(corpus.size())];
        const std::size_t changes = 1 + next(4);
        for (std::size_t change = 0; change < changes && !text.empty(); ++change)
        {
            const std::size_t position = next(text.size());
            switch (next(4))
            {
            case 0:
                text[position] = alphabet[next(alphabet.size())];
                break;
            case 1:
                text.erase(position, 1 + next(3));
                break;
            case 2:
                text.insert(position, 1, alphabet[next(alphabet.size())]);
                break;
            default:
                text.resize(position); // truncated, as a damaged file is
                break;
            }
        }
        if (attempt % 50 == 0)
            text.push_back(static_cast<char>(next(256))); // a byte of any value now and then

        const EquationParse result = parseEquations(text);
        REQUIRE(result.ok() != result.problem.has_value());
        if (!result.ok())
        {
            REQUIRE_FALSE(result.problem->message.empty());
            continue;
        }
        ++parsedCount;
        // What parses prints, and what is printed reads back to the same text.
        const std::string canonical = result.equations->canonicalText();
        const EquationParse again = parseEquations(canonical);
        INFO(text);
        INFO(canonical);
        REQUIRE(again.ok());
        REQUIRE(again.equations->canonicalText() == canonical);
        // And evaluates, or says why it cannot, with no other kind of exception.
        std::vector<double> arguments(result.equations->parameters().size(), 1.5);
        try
        {
            (void)evaluateEquations(*result.equations, std::span<const double>(arguments));
        }
        catch (const base::UsageError&)
        {
        }
    }
    // The damage is small, so some of the texts still parse: the test looks at both sides.
    REQUIRE(parsedCount > 100);
    REQUIRE(parsedCount < 4000);
}
