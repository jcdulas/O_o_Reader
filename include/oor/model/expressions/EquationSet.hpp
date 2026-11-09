#pragma once

#include "oor/model/expressions/EquationFunctions.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

enum class ExpressionKind : std::uint8_t
{
    Number,
    Variable,
    Negate,
    Binary,
    Call,
};

enum class BinaryOperator : std::uint8_t
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Power,
};

// One node of the tree of an expression. The nodes of a set of equations live in one vector and refer to each other by
// index; which members are used depends on the kind.
struct ExpressionNode
{
    ExpressionKind kind = ExpressionKind::Number;
    BinaryOperator op = BinaryOperator::Add;           // Binary
    EquationFunction function = EquationFunction::Abs; // Call
    bool isInteger = false;                            // Number: written without a point or an exponent
    double number = 0.0;                               // Number
    std::string text;                                  // Number: the spelling, upper case; Variable: the name
    std::int32_t first = -1;                           // Negate: the operand; Binary: the left operand
    std::int32_t second = -1;                          // Binary: the right operand
    std::vector<std::int32_t> arguments;               // Call
    // The character of the text the node comes from, from 1 (blanks counted): the number or the name, the sign of a
    // Negate, the operator of a Binary, the name of the function of a Call.
    std::int32_t position = 0;
};

// One equation of a DEQATN: `target = expression`.
struct Equation
{
    std::string target;
    std::int32_t root = -1;    // the node of the expression
    std::int32_t position = 0; // the character of the text where the name of the target starts, from 1
};

// A DEQATN as a program: the equations in the order they are evaluated. The first one names the function and lists its
// arguments, the others assign intermediate variables that the following equations may use, and the last one gives the
// value of the whole. Built by parseEquations and never changed afterwards.
//
// The rules of the format that the type keeps:
//  - names are not case sensitive, so every name is stored in upper case;
//  - the sign of the first term of an expression applies to the whole term, so -X**2 is -(X**2);
//  - arithmetic follows the type of the operands: a number written without a point is an integer, and two integers give
//    an integer (1/2 is 0), while every variable and every function value is real.
class EquationSet
{
public:
    EquationSet() = default;

    // The name of the function (the target of the first equation).
    [[nodiscard]] const std::string& functionName() const noexcept { return m_equations.front().target; }
    // The arguments of the function, in the order the card lists them.
    [[nodiscard]] const std::vector<std::string>& parameters() const noexcept { return m_parameters; }
    [[nodiscard]] const std::vector<Equation>& equations() const noexcept { return m_equations; }
    [[nodiscard]] const ExpressionNode& node(std::int32_t index) const
    {
        return m_nodes[static_cast<std::size_t>(index)];
    }
    [[nodiscard]] std::size_t nodeCount() const noexcept { return m_nodes.size(); }

    // The text of the equations in a form that depends only on their meaning: upper case, no blanks, the least number
    // of parentheses that gives the same tree, equations separated by ";". It reads back to the same tree.
    [[nodiscard]] std::string canonicalText() const;

    // For the parser, which builds the set.
    std::int32_t addNode(ExpressionNode node);
    void addEquation(Equation equation) { m_equations.push_back(std::move(equation)); }
    void setParameters(std::vector<std::string> parameters) { m_parameters = std::move(parameters); }

private:
    std::vector<ExpressionNode> m_nodes;
    std::vector<Equation> m_equations;
    std::vector<std::string> m_parameters;
};

// What is wrong with the text of a DEQATN: a message and the position (1 is the first character of the text, blanks
// included) where the parser stopped.
struct EquationProblem
{
    std::string message;
    std::size_t position = 1;
};

// The result of parsing: the equations, or the first problem found.
struct EquationParse
{
    std::optional<EquationSet> equations;
    std::optional<EquationProblem> problem;

    [[nodiscard]] bool ok() const noexcept { return equations.has_value(); }
};

// Parses the text of a DEQATN (the lines of the card joined). Never throws: a text that is not an equation gives the
// problem, so that a deck with a bad DEQATN is reported and the rest of it is read.
[[nodiscard]] EquationParse parseEquations(std::string_view text);

} // namespace oor::model
