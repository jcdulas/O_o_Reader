#pragma once

#include "oor/model/expressions/EquationFunctions.hpp"
#include "oor/model/expressions/EquationSet.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// A DEQATN as an expression program (the queries `designEquationSymbols` and `designEquationInstructions`):
// the names it uses and the instructions of a stack machine that computes it, so that a companion package evaluates an
// equation and its derivatives without parsing text. The contract is the one a companion package proposed (its item
// P1.3):
//
//  - a symbol is an argument (its slot is its position in the argument list of the first equation) or a variable (its
//    slot is the index of the statement that gives it); names are upper case and unique, and there is at least one
//    variable;
//  - each statement (one equation `name = expression`) is a run of instructions that leaves one value on the stack, the
//    value of its variable; the value of the program is the value of its last statement;
//  - `realConstant` and `integerConstant` push `value`; an integer constant is a number written without a point or an
//    exponent, and two integers give an integer (1/2 is 0, 2**-1 is 0), while a
//    variable, an argument and the value of every function are real;
//  - `argument` pushes the argument of slot `operand`, `variable` the value of the earlier statement `operand`;
//  - `negate` pops one value and pushes its opposite; `add`, `subtract`, `multiply`, `divide` and `power` pop b, then
//  a,
//    and push a + b, a - b, a * b, a / b and a ** b;
//  - `call` pops `operand` values (the last argument was pushed last) and pushes the value of `function`, an intrinsic
//    function; AVG, MAX, MIN, RSS, SSQ and SUM take 1 to 96 arguments, the others their fixed number.
//
// The producer has resolved the precedence of the operators (the sign of the first term applies to the whole term, **
// is right associative), the names (a name that no argument or earlier equation gives is the number pi when it is PI)
// and the arities. `evaluateEquations` (EquationEvaluator.hpp) fixes the semantics: a program run by the rules above
// gives the value it gives.

namespace oor::model
{

enum class EquationSymbolKind : std::uint8_t
{
    Argument,
    Variable,
};

struct EquationSymbol
{
    EquationSymbolKind kind = EquationSymbolKind::Argument;
    std::int32_t slot = 0;
    std::string name; // upper case
};

enum class EquationOpcode : std::uint8_t
{
    RealConstant,
    IntegerConstant,
    Argument,
    Variable,
    Negate,
    Add,
    Subtract,
    Multiply,
    Divide,
    Power,
    Call,
};

// The name of an opcode in the exported tables: "realConstant", "integerConstant", "argument", ...
[[nodiscard]] std::string_view opcodeName(EquationOpcode opcode) noexcept;
// "argument" or "variable".
[[nodiscard]] std::string_view symbolKindName(EquationSymbolKind kind) noexcept;

struct EquationInstruction
{
    std::int32_t statement = 0;
    EquationOpcode opcode = EquationOpcode::RealConstant;
    std::int32_t operand = 0; // the slot of an argument, the statement of a variable, the argument count of a call
    double value = 0.0;       // the constant
    std::optional<EquationFunction> function; // the function of a call
    std::int32_t position = 0; // the character of the text the instruction comes from, from 1; 0 when unknown
};

class EquationProgram
{
public:
    // The arguments in the order of the argument list, then the variables in the order of the statements.
    [[nodiscard]] const std::vector<EquationSymbol>& symbols() const noexcept { return m_symbols; }
    // The instructions of every statement, the statements in order.
    [[nodiscard]] const std::vector<EquationInstruction>& instructions() const noexcept { return m_instructions; }

private:
    friend class EquationProgramBuilder;

    std::vector<EquationSymbol> m_symbols;
    std::vector<EquationInstruction> m_instructions;
};

// The program of a set of equations, or the reason there is none.
struct EquationProgramBuild
{
    std::optional<EquationProgram> program;
    std::optional<EquationProblem> problem;

    [[nodiscard]] bool ok() const noexcept { return program.has_value(); }
};

// Flattens equations that parsed. Refused (with the position of the name): an equation that gives a name an argument or
// an earlier equation already has, which the evaluator accepts (the name then stands for the new value) but which
// leaves two symbols with one name; the format reference lists in the argument list every name except the variables the
// equations give.
[[nodiscard]] EquationProgramBuild buildEquationProgram(const EquationSet& equations);

// Parses the text of a DEQATN (an equation written in Python, not a card) and flattens it. The problem, if any, is the
// first one the parser or the flattening found, with its position in `text`.
[[nodiscard]] EquationProgramBuild compileEquationProgram(std::string_view text);

} // namespace oor::model
