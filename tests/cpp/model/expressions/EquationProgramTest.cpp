// A DEQATN as an expression program (the queries `designEquationSymbols` and `designEquationInstructions`): the
// symbols, the instructions of the stack machine, their positions, and the value a small stack machine written here
// gets from them, which must be the one of the evaluator.

#include "oor/model/expressions/EquationEvaluator.hpp"
#include "oor/model/expressions/EquationProgram.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using Catch::Approx;

namespace
{

EquationProgram compiled(const std::string& text)
{
    EquationProgramBuild build = compileEquationProgram(text);
    INFO(text);
    INFO((build.problem ? build.problem->message : std::string()));
    REQUIRE(build.ok());
    return std::move(*build.program);
}

// A value on the stack: a number and whether it is an integer.
struct StackValue
{
    double number = 0.0;
    bool isInteger = false;
};

double integerPower(double base, double exponent)
{
    if (exponent >= 0.0)
        return std::round(std::pow(base, exponent));
    if (base == 1.0)
        return 1.0;
    if (base == -1.0)
        return std::fmod(exponent, 2.0) == 0.0 ? 1.0 : -1.0;
    return 0.0;
}

// The functions the cases below call.
double callFunction(EquationFunction function, const std::vector<double>& a)
{
    switch (function)
    {
    case EquationFunction::Sin:
        return std::sin(a[0]);
    case EquationFunction::Cos:
        return std::cos(a[0]);
    case EquationFunction::Sqrt:
        return std::sqrt(a[0]);
    case EquationFunction::Pi:
        return a[0] * std::numbers::pi;
    case EquationFunction::Max:
        return *std::max_element(a.begin(), a.end());
    case EquationFunction::Sum:
    {
        double sum = 0.0;
        for (const double value : a)
            sum += value;
        return sum;
    }
    case EquationFunction::Atan2:
        return std::atan2(a[0], a[1]);
    default:
        FAIL("the test machine does not know the function " << infoOf(function).name);
        return 0.0;
    }
}

// The stack machine of the contract, as a companion package would write it.
double run(const EquationProgram& program, const std::vector<double>& arguments)
{
    std::vector<StackValue> statements;
    std::vector<StackValue> stack;
    std::int32_t statement = 0;
    const auto pop = [&]
    {
        REQUIRE_FALSE(stack.empty());
        const StackValue value = stack.back();
        stack.pop_back();
        return value;
    };
    const auto finishStatement = [&]
    {
        REQUIRE(stack.size() == 1);
        statements.push_back(StackValue{stack.back().number, false}); // every variable is real
        stack.clear();
    };
    for (const EquationInstruction& instruction : program.instructions())
    {
        if (instruction.statement != statement)
        {
            REQUIRE(instruction.statement == statement + 1);
            finishStatement();
            statement = instruction.statement;
        }
        switch (instruction.opcode)
        {
        case EquationOpcode::RealConstant:
            stack.push_back(StackValue{instruction.value, false});
            break;
        case EquationOpcode::IntegerConstant:
            stack.push_back(StackValue{instruction.value, true});
            break;
        case EquationOpcode::Argument:
            stack.push_back(StackValue{arguments.at(static_cast<std::size_t>(instruction.operand)), false});
            break;
        case EquationOpcode::Variable:
            REQUIRE(instruction.operand < statement);
            stack.push_back(statements.at(static_cast<std::size_t>(instruction.operand)));
            break;
        case EquationOpcode::Negate:
        {
            const StackValue value = pop();
            stack.push_back(StackValue{-value.number, value.isInteger});
            break;
        }
        case EquationOpcode::Call:
        {
            std::vector<double> values(static_cast<std::size_t>(instruction.operand));
            for (auto value = values.rbegin(); value != values.rend(); ++value)
                *value = pop().number;
            REQUIRE(instruction.function.has_value());
            stack.push_back(StackValue{callFunction(*instruction.function, values), false});
            break;
        }
        default:
        {
            const StackValue b = pop();
            const StackValue a = pop();
            const bool integers = a.isInteger && b.isInteger;
            double result = 0.0;
            switch (instruction.opcode)
            {
            case EquationOpcode::Add:
                result = a.number + b.number;
                break;
            case EquationOpcode::Subtract:
                result = a.number - b.number;
                break;
            case EquationOpcode::Multiply:
                result = a.number * b.number;
                break;
            case EquationOpcode::Divide:
                result = integers ? std::trunc(a.number / b.number) : a.number / b.number;
                break;
            default:
                result = integers ? integerPower(a.number, b.number) : std::pow(a.number, b.number);
                break;
            }
            stack.push_back(StackValue{result, integers});
            break;
        }
        }
    }
    finishStatement();
    return statements.back().number;
}

void requireSameValue(const std::string& text, const std::vector<double>& arguments)
{
    INFO(text);
    const EquationParse parsed = parseEquations(text);
    REQUIRE(parsed.ok());
    const double expected = evaluateEquations(*parsed.equations, std::span<const double>(arguments));
    REQUIRE(run(compiled(text), arguments) == Approx(expected).epsilon(1e-14));
}

} // namespace

TEST_CASE("a program keeps the integer rule, the sign of the first term, the order of ** and the number PI",
          "[model][expressions]")
{
    // 1/2 is 0 and 2**-1 is 0 (two integers); 1./2 is a real. -X**2 is -(X**2); 2**3**2 is 2**(3**2).
    requireSameValue("F(X) = X + 1/2", {1.0});
    requireSameValue("F(X) = X + 2**(-1) + 1./2", {1.0});
    requireSameValue("F(X) = -X**2", {3.0});
    requireSameValue("F(X) = 2**3**2 + X", {0.0});
    requireSameValue("F(X) = PI*X", {2.0});
    requireSameValue("F(X, Y) = MAX(X, Y, 3) + SUM(X) + ATAN2(Y, X); G = F/2; H = SQRT(G*G) - COS(X)", {1.5, -2.0});
    // Integer arithmetic: the instruction of a number written without a point is an integer constant.
    const EquationProgram program = compiled("F(X) = X*2 + 2.");
    REQUIRE(program.instructions()[1].opcode == EquationOpcode::IntegerConstant);
    REQUIRE(program.instructions()[1].value == 2.0);
    REQUIRE(program.instructions()[3].opcode == EquationOpcode::RealConstant);
    // The operators carry the column of their sign, from 1.
    REQUIRE((program.instructions()[2].opcode == EquationOpcode::Multiply && program.instructions()[2].position == 9));
    REQUIRE((program.instructions()[4].opcode == EquationOpcode::Add && program.instructions()[4].position == 12));
}

TEST_CASE("compileEquationProgram refuses a text that does not parse with its column, and a name given twice",
          "[model][expressions]")
{
    const EquationProgramBuild bad = compileEquationProgram("F(A) = A + * 2");
    REQUIRE_FALSE(bad.ok());
    REQUIRE(bad.problem->position == 12);
    const EquationProgramBuild unknown = compileEquationProgram("F(A) = A + B");
    REQUIRE_FALSE(unknown.ok());
    REQUIRE(unknown.problem->position == 12);
    // The evaluator accepts an equation that gives an argument a new value; the program would have two symbols of one
    // name, so it is refused at the name.
    const EquationProgramBuild twice = compileEquationProgram("F(A) = A + 1; A = F*2");
    REQUIRE_FALSE(twice.ok());
    REQUIRE(twice.problem->position == 15);
    REQUIRE(twice.problem->message.find("A is an argument") != std::string::npos);
    // 96 arguments of a variadic function are allowed, 97 are not.
    std::string arguments = "X";
    for (int index = 1; index < 96; ++index)
        arguments += ",X";
    REQUIRE(compileEquationProgram("F(X) = SUM(" + arguments + ")").ok());
    REQUIRE_FALSE(compileEquationProgram("F(X) = SUM(" + arguments + ",X)").ok());
}
