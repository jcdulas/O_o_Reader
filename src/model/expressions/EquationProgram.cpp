#include "oor/model/expressions/EquationProgram.hpp"

#include <algorithm>
#include <map>
#include <numbers>
#include <utility>

namespace oor::model
{

std::string_view opcodeName(EquationOpcode opcode) noexcept
{
    switch (opcode)
    {
    case EquationOpcode::RealConstant:
        return "realConstant";
    case EquationOpcode::IntegerConstant:
        return "integerConstant";
    case EquationOpcode::Argument:
        return "argument";
    case EquationOpcode::Variable:
        return "variable";
    case EquationOpcode::Negate:
        return "negate";
    case EquationOpcode::Add:
        return "add";
    case EquationOpcode::Subtract:
        return "subtract";
    case EquationOpcode::Multiply:
        return "multiply";
    case EquationOpcode::Divide:
        return "divide";
    case EquationOpcode::Power:
        return "power";
    case EquationOpcode::Call:
        break;
    }
    return "call";
}

std::string_view symbolKindName(EquationSymbolKind kind) noexcept
{
    return kind == EquationSymbolKind::Argument ? "argument" : "variable";
}

// Turns the trees of the equations into the instructions of the stack machine, in post-order: the operands of an
// operation are pushed before it, the arguments of a function in their order.
class EquationProgramBuilder
{
public:
    explicit EquationProgramBuilder(const EquationSet& equations) : m_equations(equations) {}

    EquationProgramBuild run()
    {
        EquationProgramBuild result;
        const std::vector<std::string>& parameters = m_equations.parameters();
        for (std::size_t slot = 0; slot < parameters.size(); ++slot)
        {
            m_arguments.emplace(parameters[slot], static_cast<std::int32_t>(slot));
            m_program.m_symbols.push_back(
                EquationSymbol{EquationSymbolKind::Argument, static_cast<std::int32_t>(slot), parameters[slot]});
        }
        const std::vector<Equation>& equations = m_equations.equations();
        for (std::size_t index = 0; index < equations.size(); ++index)
        {
            const Equation& equation = equations[index];
            m_statement = static_cast<std::int32_t>(index);
            emit(equation.root);
            // The name is given only once its expression is done: the expression reads the names before it.
            if (const std::optional<std::string> taken = describeTaken(equation.target))
            {
                result.problem =
                    EquationProblem{"the equation gives " + equation.target + " again: " + *taken +
                                        "; each equation gives a new variable",
                                    static_cast<std::size_t>(std::max(equation.position, 1))};
                return result;
            }
            m_variables.emplace(equation.target, m_statement);
            m_program.m_symbols.push_back(EquationSymbol{EquationSymbolKind::Variable, m_statement, equation.target});
        }
        result.program = std::move(m_program);
        return result;
    }

private:
    [[nodiscard]] std::optional<std::string> describeTaken(const std::string& name) const
    {
        if (m_arguments.contains(name))
            return name + " is an argument of the function";
        if (const auto variable = m_variables.find(name); variable != m_variables.end())
            return "equation " + std::to_string(variable->second + 1) + " gives it";
        return std::nullopt;
    }

    void push(EquationOpcode opcode, std::int32_t operand, double value, std::int32_t position,
              std::optional<EquationFunction> function = std::nullopt)
    {
        m_program.m_instructions.push_back(
            EquationInstruction{m_statement, opcode, operand, value, function, position});
    }

    void emit(std::int32_t index)
    {
        const ExpressionNode& node = m_equations.node(index);
        switch (node.kind)
        {
        case ExpressionKind::Number:
            push(node.isInteger ? EquationOpcode::IntegerConstant : EquationOpcode::RealConstant, 0, node.number,
                 node.position);
            return;
        case ExpressionKind::Variable:
            emitName(node);
            return;
        case ExpressionKind::Negate:
            emit(node.first);
            push(EquationOpcode::Negate, 0, 0.0, node.position);
            return;
        case ExpressionKind::Binary:
            emit(node.first);
            emit(node.second);
            push(opcodeOf(node.op), 0, 0.0, node.position);
            return;
        case ExpressionKind::Call:
            break;
        }
        for (const std::int32_t argument : node.arguments)
            emit(argument);
        push(EquationOpcode::Call, static_cast<std::int32_t>(node.arguments.size()), 0.0, node.position, node.function);
    }

    // A name is the variable of the latest equation that gave it, else an argument; the parser accepted it, so the one
    // name that is neither is PI, the number (an argument or a variable named PI stands for itself).
    void emitName(const ExpressionNode& node)
    {
        if (const auto variable = m_variables.find(node.text); variable != m_variables.end())
            push(EquationOpcode::Variable, variable->second, 0.0, node.position);
        else if (const auto argument = m_arguments.find(node.text); argument != m_arguments.end())
            push(EquationOpcode::Argument, argument->second, 0.0, node.position);
        else
            push(EquationOpcode::RealConstant, 0, std::numbers::pi, node.position);
    }

    [[nodiscard]] static EquationOpcode opcodeOf(BinaryOperator op) noexcept
    {
        switch (op)
        {
        case BinaryOperator::Add:
            return EquationOpcode::Add;
        case BinaryOperator::Subtract:
            return EquationOpcode::Subtract;
        case BinaryOperator::Multiply:
            return EquationOpcode::Multiply;
        case BinaryOperator::Divide:
            return EquationOpcode::Divide;
        case BinaryOperator::Power:
            break;
        }
        return EquationOpcode::Power;
    }

    const EquationSet& m_equations;
    EquationProgram m_program;
    std::map<std::string, std::int32_t> m_arguments;
    std::map<std::string, std::int32_t> m_variables;
    std::int32_t m_statement = 0;
};

EquationProgramBuild buildEquationProgram(const EquationSet& equations)
{
    return EquationProgramBuilder(equations).run();
}

EquationProgramBuild compileEquationProgram(std::string_view text)
{
    EquationParse parsed = parseEquations(text);
    if (!parsed.ok())
    {
        EquationProgramBuild result;
        result.problem = std::move(parsed.problem);
        return result;
    }
    return buildEquationProgram(*parsed.equations);
}

} // namespace oor::model
