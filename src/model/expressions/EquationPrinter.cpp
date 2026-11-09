#include "oor/model/expressions/EquationSet.hpp"

#include <string>

namespace oor::model
{

namespace
{

// How tightly an operator binds, the lowest first. A child that binds less tightly than its place asks needs
// parentheses.
constexpr int SUM = 1;
constexpr int PRODUCT = 2;
constexpr int POWER = 3;

int precedenceOf(BinaryOperator op) noexcept
{
    switch (op)
    {
    case BinaryOperator::Add:
    case BinaryOperator::Subtract:
        return SUM;
    case BinaryOperator::Multiply:
    case BinaryOperator::Divide:
        return PRODUCT;
    case BinaryOperator::Power:
        break;
    }
    return POWER;
}

const char* symbolOf(BinaryOperator op) noexcept
{
    switch (op)
    {
    case BinaryOperator::Add:
        return "+";
    case BinaryOperator::Subtract:
        return "-";
    case BinaryOperator::Multiply:
        return "*";
    case BinaryOperator::Divide:
        return "/";
    case BinaryOperator::Power:
        break;
    }
    return "**";
}

// Writes a tree as text that reads back to the same tree. The tree has no node for a parenthesis, so the parentheses
// are found from the operators: the operators of a sum and of a product group from the left, the power from the right,
// and the sign that opens an expression takes the whole first term (-A*B is -(A*B)), so a negation anywhere else, and
// the negation of a factor, is written in parentheses.
class ExpressionPrinter
{
public:
    explicit ExpressionPrinter(const EquationSet& set) noexcept : m_set(set) {}

    // `parentPrecedence`: the precedence of the place the text goes in (0 for a whole expression); `atStart`: the text
    // opens an expression, where a sign needs no parentheses.
    [[nodiscard]] std::string print(std::int32_t index, int parentPrecedence, bool atStart) const
    {
        const ExpressionNode& node = m_set.node(index);
        switch (node.kind)
        {
        case ExpressionKind::Number:
        case ExpressionKind::Variable:
            return node.text;
        case ExpressionKind::Call:
            return printCall(node);
        case ExpressionKind::Negate:
            if (!(atStart && parentPrecedence <= SUM))
                return "(" + print(index, 0, true) + ")";
            return "-" + print(node.first, PRODUCT, false);
        case ExpressionKind::Binary:
            break;
        }
        const int precedence = precedenceOf(node.op);
        if (precedence < parentPrecedence)
            return "(" + print(index, 0, true) + ")";
        switch (node.op)
        {
        case BinaryOperator::Add:
        case BinaryOperator::Subtract:
            return print(node.first, SUM, atStart) + symbolOf(node.op) + print(node.second, SUM + 1, false);
        case BinaryOperator::Multiply:
        case BinaryOperator::Divide:
            return print(node.first, PRODUCT, false) + symbolOf(node.op) + print(node.second, PRODUCT + 1, false);
        case BinaryOperator::Power:
            break;
        }
        return print(node.first, POWER + 1, false) + "**" + print(node.second, POWER, false);
    }

private:
    [[nodiscard]] std::string printCall(const ExpressionNode& node) const
    {
        std::string text = node.text + "(";
        for (std::size_t argument = 0; argument < node.arguments.size(); ++argument)
        {
            if (argument > 0)
                text += ",";
            text += print(node.arguments[argument], 0, true);
        }
        return text + ")";
    }

    const EquationSet& m_set;
};

} // namespace

std::string EquationSet::canonicalText() const
{
    const ExpressionPrinter printer(*this);
    std::string text;
    for (std::size_t index = 0; index < m_equations.size(); ++index)
    {
        if (index > 0)
            text += ";";
        text += m_equations[index].target;
        if (index == 0 && !m_parameters.empty())
        {
            text += "(";
            for (std::size_t parameter = 0; parameter < m_parameters.size(); ++parameter)
            {
                if (parameter > 0)
                    text += ",";
                text += m_parameters[parameter];
            }
            text += ")";
        }
        text += "=" + printer.print(m_equations[index].root, 0, true);
    }
    return text;
}

} // namespace oor::model
