#include "ExpressionLexer.hpp"

#include "oor/model/expressions/EquationSet.hpp"

#include <algorithm>
#include <set>
#include <utility>

namespace oor::model
{

namespace
{

// Thrown inside the parser to stop at the first problem; parseEquations turns it into an EquationProblem, so nothing
// leaves this file.
struct ParseStop
{
    std::string message;
    std::size_t position = 1;
};

// The DMAP logical operators cannot be the name of a variable.
bool isReservedName(const std::string& name) noexcept
{
    return name == "NOT" || name == "AND" || name == "OR" || name == "XOR" || name == "XQV";
}

// Recursive descent over the tokens. The grammar follows the format reference tables of operators:
//
//   equations  = equation { ";" equation } [ ";" ]
//   equation   = name [ "(" name { "," name } ")" ] "=" expression      (the argument list: first equation only)
//   expression = [ sign ] term { sign term }          the sign of the first term applies to the whole term
//   term       = factor { ( "*" | "/" ) factor }
//   factor     = [ sign ] power                       a sign after an operator applies to the factor after it
//   power      = primary [ "**" [ sign ] power ]      right to left: A**B**C is A**(B**C)
//   primary    = number | name | name "(" expression { "," expression } ")" | "(" expression ")"
//
// A sign after an operator is not Fortran but is what X**-Y in the table means, and what Python accepts.
class EquationParser
{
public:
    explicit EquationParser(std::vector<Token> tokens) : m_tokens(std::move(tokens)) {}

    EquationSet parse()
    {
        while (peek().kind == TokenKind::Semicolon)
            advance();
        if (peek().kind == TokenKind::End)
            fail("the equation is empty", peek().position);
        bool isFirst = true;
        while (peek().kind != TokenKind::End)
        {
            parseEquation(isFirst);
            isFirst = false;
            if (peek().kind == TokenKind::Semicolon)
            {
                while (peek().kind == TokenKind::Semicolon)
                    advance();
            }
            else if (peek().kind != TokenKind::End)
            {
                fail("expected \";\" or the end of the equation, not " + describe(peek()), peek().position);
            }
        }
        return std::move(m_set);
    }

private:
    [[nodiscard]] const Token& peek() const noexcept { return m_tokens[m_cursor]; }
    const Token& advance() noexcept
    {
        const Token& token = m_tokens[m_cursor];
        if (token.kind != TokenKind::End)
            ++m_cursor;
        return token;
    }
    [[noreturn]] static void fail(std::string message, std::size_t position)
    {
        throw ParseStop{std::move(message), position};
    }

    [[nodiscard]] static std::string describe(const Token& token)
    {
        switch (token.kind)
        {
        case TokenKind::Number:
        case TokenKind::Name:
            return "\"" + token.text + "\"";
        case TokenKind::Plus:
            return "\"+\"";
        case TokenKind::Minus:
            return "\"-\"";
        case TokenKind::Star:
            return "\"*\"";
        case TokenKind::Slash:
            return "\"/\"";
        case TokenKind::Power:
            return "\"**\"";
        case TokenKind::LeftParenthesis:
            return "\"(\"";
        case TokenKind::RightParenthesis:
            return "\")\"";
        case TokenKind::Comma:
            return "\",\"";
        case TokenKind::Equals:
            return "\"=\"";
        case TokenKind::Semicolon:
            return "\";\"";
        case TokenKind::End:
            break;
        }
        return "the end of the equation";
    }

    void expect(TokenKind kind, const char* what)
    {
        if (peek().kind != kind)
            fail(std::string("expected ") + what + ", not " + describe(peek()), peek().position);
        advance();
    }

    // A name that a variable may have: not one of the DMAP logical operators.
    std::string takeName(const char* what)
    {
        if (peek().kind != TokenKind::Name)
            fail(std::string("expected ") + what + ", not " + describe(peek()), peek().position);
        const Token token = advance();
        if (isReservedName(token.text))
            fail("\"" + token.text + "\" is a DMAP logical operator and cannot be the name of a variable",
                 token.position);
        return token.text;
    }

    void parseEquation(bool isFirst)
    {
        const std::int32_t targetPosition = positionOf(peek());
        std::string target = takeName("the name of the variable the equation gives");
        std::vector<std::string> parameters;
        if (peek().kind == TokenKind::LeftParenthesis)
        {
            const std::size_t position = peek().position;
            if (!isFirst)
                fail(
                    "only the first equation lists the arguments of the function; the others give a variable by its "
                    "name "
                    "(\"" +
                        target + " = ...\")",
                    position);
            advance();
            if (peek().kind != TokenKind::RightParenthesis)
            {
                while (true)
                {
                    const std::size_t argumentPosition = peek().position;
                    std::string name = takeName("the name of an argument");
                    if (name == target)
                        fail("the name of the function, " + name + ", is also one of its arguments", argumentPosition);
                    if (std::find(parameters.begin(), parameters.end(), name) != parameters.end())
                        fail("the argument " + name + " is listed twice", argumentPosition);
                    parameters.push_back(std::move(name));
                    if (peek().kind != TokenKind::Comma)
                        break;
                    advance();
                }
            }
            expect(TokenKind::RightParenthesis, "\")\" after the arguments");
        }
        expect(TokenKind::Equals, "\"=\" after the name of the variable");
        if (isFirst)
        {
            m_defined.insert(parameters.begin(), parameters.end());
            m_set.setParameters(std::move(parameters));
        }
        const std::int32_t root = parseExpression();
        m_defined.insert(target);
        m_set.addEquation(Equation{std::move(target), root, targetPosition});
    }

    std::int32_t makeNode(ExpressionNode node) { return m_set.addNode(std::move(node)); }

    static std::int32_t positionOf(const Token& token) noexcept { return static_cast<std::int32_t>(token.position); }

    std::int32_t makeNegate(std::int32_t operand, std::int32_t position)
    {
        ExpressionNode node;
        node.kind = ExpressionKind::Negate;
        node.first = operand;
        node.position = position;
        return makeNode(std::move(node));
    }

    std::int32_t makeBinary(BinaryOperator op, std::int32_t left, std::int32_t right, std::int32_t position)
    {
        ExpressionNode node;
        node.kind = ExpressionKind::Binary;
        node.op = op;
        node.first = left;
        node.second = right;
        node.position = position;
        return makeNode(std::move(node));
    }

    // Nesting is limited so that a hostile text of parentheses or exponents cannot exhaust the stack.
    static constexpr int MAX_DEPTH = 200;

    class DepthGuard
    {
    public:
        explicit DepthGuard(EquationParser& parser, std::size_t position) : m_parser(parser)
        {
            if (++m_parser.m_depth > MAX_DEPTH)
                fail("the equation is nested more than " + std::to_string(MAX_DEPTH) + " levels deep", position);
        }
        DepthGuard(const DepthGuard&) = delete;
        DepthGuard& operator=(const DepthGuard&) = delete;
        ~DepthGuard() { --m_parser.m_depth; }

    private:
        EquationParser& m_parser;
    };

    std::int32_t parseExpression()
    {
        const DepthGuard guard(*this, peek().position);
        // The sign of the first term applies to the whole term: -A*B is -(A*B), and so is -A/B. The other terms take
        // their sign from the operator between them.
        bool negate = false;
        std::int32_t signPosition = 0;
        if (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus)
        {
            signPosition = positionOf(peek());
            negate = advance().kind == TokenKind::Minus;
        }
        std::int32_t left = parseTerm();
        if (negate)
            left = makeNegate(left, signPosition);
        while (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus)
        {
            const std::int32_t position = positionOf(peek());
            const BinaryOperator op =
                advance().kind == TokenKind::Plus ? BinaryOperator::Add : BinaryOperator::Subtract;
            left = makeBinary(op, left, parseTerm(), position);
        }
        return left;
    }

    std::int32_t parseTerm()
    {
        std::int32_t left = parseFactor();
        while (peek().kind == TokenKind::Star || peek().kind == TokenKind::Slash)
        {
            const std::int32_t position = positionOf(peek());
            const BinaryOperator op =
                advance().kind == TokenKind::Star ? BinaryOperator::Multiply : BinaryOperator::Divide;
            left = makeBinary(op, left, parseFactor(), position);
        }
        return left;
    }

    std::int32_t parseFactor()
    {
        if (peek().kind == TokenKind::Plus)
        {
            advance();
            return parsePower();
        }
        if (peek().kind == TokenKind::Minus)
        {
            const std::int32_t position = positionOf(advance());
            return makeNegate(parsePower(), position);
        }
        return parsePower();
    }

    std::int32_t parsePower()
    {
        const DepthGuard guard(*this, peek().position);
        const std::int32_t base = parsePrimary();
        if (peek().kind != TokenKind::Power)
            return base;
        const std::int32_t position = positionOf(advance());
        return makeBinary(BinaryOperator::Power, base, parseFactor(), position);
    }

    std::int32_t parsePrimary()
    {
        const Token token = peek();
        switch (token.kind)
        {
        case TokenKind::Number:
        {
            advance();
            ExpressionNode node;
            node.kind = ExpressionKind::Number;
            node.number = token.number;
            node.isInteger = token.isInteger;
            node.text = token.text;
            node.position = positionOf(token);
            return makeNode(std::move(node));
        }
        case TokenKind::Name:
            advance();
            if (peek().kind == TokenKind::LeftParenthesis)
                return parseCall(token);
            return parseVariable(token);
        case TokenKind::LeftParenthesis:
        {
            advance();
            const std::int32_t inner = parseExpression();
            expect(TokenKind::RightParenthesis, "\")\"");
            return inner;
        }
        default:
            break;
        }
        fail("expected a number, a variable or a function, not " + describe(token), token.position);
    }

    std::int32_t parseVariable(const Token& token)
    {
        // PI alone is the number, unless an argument or a variable has that name (the format reference gives the
        // function PI(x), which is x times the number; a deck may write the constant alone).
        if (token.text != "PI" && !m_defined.contains(token.text))
            fail("the variable " + token.text + " is not an argument of the function and no earlier equation gives it",
                 token.position);
        ExpressionNode node;
        node.kind = ExpressionKind::Variable;
        node.text = token.text;
        node.position = positionOf(token);
        return makeNode(std::move(node));
    }

    std::int32_t parseCall(const Token& nameToken)
    {
        const std::optional<EquationFunction> function = findEquationFunction(nameToken.text);
        if (!function)
            fail("\"" + nameToken.text + "\" is not one of the functions of a DEQATN", nameToken.position);
        advance(); // the parenthesis
        ExpressionNode node;
        node.kind = ExpressionKind::Call;
        node.function = *function;
        node.text = nameToken.text;
        node.position = positionOf(nameToken);
        if (peek().kind != TokenKind::RightParenthesis)
        {
            while (true)
            {
                node.arguments.push_back(parseExpression());
                if (peek().kind != TokenKind::Comma)
                    break;
                advance();
            }
        }
        const std::size_t closing = peek().position;
        expect(TokenKind::RightParenthesis, "\")\" after the arguments of the function");
        const EquationFunctionInfo& info = infoOf(*function);
        const std::size_t count = node.arguments.size();
        if (count < info.minimumArguments || count > info.maximumArguments)
        {
            std::string expected =
                info.minimumArguments == info.maximumArguments
                    ? std::to_string(info.minimumArguments)
                    : std::to_string(info.minimumArguments) + " to " + std::to_string(info.maximumArguments);
            fail(std::string(info.name) + " takes " + expected + " argument" + (info.maximumArguments == 1 ? "" : "s") +
                     ", not " + std::to_string(count),
                 closing);
        }
        return makeNode(std::move(node));
    }

    std::vector<Token> m_tokens;
    std::size_t m_cursor = 0;
    int m_depth = 0;
    EquationSet m_set;
    std::set<std::string> m_defined;
};

} // namespace

std::int32_t EquationSet::addNode(ExpressionNode node)
{
    m_nodes.push_back(std::move(node));
    return static_cast<std::int32_t>(m_nodes.size() - 1);
}

// The format reference allows fewer than 32 000 non blank characters in an equation.
constexpr std::size_t MAX_NON_BLANK_CHARACTERS = 32000;

EquationParse parseEquations(std::string_view text)
{
    const std::size_t nonBlank = static_cast<std::size_t>(
        std::count_if(text.begin(), text.end(), [](char character) { return character != ' ' && character != '\t'; }));
    if (nonBlank >= MAX_NON_BLANK_CHARACTERS)
    {
        EquationParse tooLong;
        tooLong.problem = EquationProblem{"the equation has 32 000 non blank characters or more", 1};
        return tooLong;
    }
    ExpressionLexer lexer(text);
    std::vector<Token> tokens = lexer.tokens();
    EquationParse result;
    if (!lexer.problem().empty())
    {
        result.problem = EquationProblem{lexer.problem(), lexer.problemPosition()};
        return result;
    }
    try
    {
        result.equations = EquationParser(std::move(tokens)).parse();
    }
    catch (const ParseStop& stop)
    {
        result.problem = EquationProblem{stop.message, stop.position};
    }
    return result;
}

} // namespace oor::model
