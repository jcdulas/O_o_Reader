#pragma once

#include "oor/query/operators/Expression.hpp"

namespace oor::query
{

enum class NodeKind : std::uint8_t
{
    Literal,
    Column,
    Negate,
    Not,
    Binary,
    In,
    IsNull,
    Call
};

enum class BinaryOp : std::uint8_t
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Power,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    And,
    Or
};

enum class Function : std::uint8_t
{
    Abs,
    Sqrt,
    IsNull,
    IsNan
};

// A node of the tree of an expression (shared by the parser and the evaluator).
struct Expression::Node
{
    NodeKind kind = NodeKind::Literal;
    Literal literal;                                    // Literal
    std::string name;                                   // Column
    BinaryOp op = BinaryOp::Add;                        // Binary
    std::shared_ptr<const Node> left;                   // Binary, and the operand of Negate, Not, In, IsNull
    std::shared_ptr<const Node> right;                  // Binary
    std::vector<Literal> list;                          // In
    bool negated = false;                               // In (not in), IsNull (is not null)
    Function function = Function::Abs;                  // Call
    std::vector<std::shared_ptr<const Node>> arguments; // Call
};

} // namespace oor::query
