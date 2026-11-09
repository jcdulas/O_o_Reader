#include "oor/query/operators/Expression.hpp"

#include "ExpressionNode.hpp"

#include "oor/query/QueryErrors.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <system_error>

namespace oor::query
{

std::string_view valueTypeName(ValueType type) noexcept
{
    switch (type)
    {
    case ValueType::Bool:
        return "bool";
    case ValueType::Int:
        return "integer";
    case ValueType::Double:
        return "number";
    case ValueType::Text:
        return "text";
    }
    return "?";
}

Literal Literal::ofBool(bool value)
{
    Literal literal;
    literal.kind = Kind::Bool;
    literal.boolean = value;
    return literal;
}

Literal Literal::ofInt(std::int64_t value)
{
    Literal literal;
    literal.kind = Kind::Int;
    literal.integer = value;
    return literal;
}

Literal Literal::ofDouble(double value)
{
    Literal literal;
    literal.kind = Kind::Double;
    literal.real = value;
    return literal;
}

Literal Literal::ofText(std::string value)
{
    Literal literal;
    literal.kind = Kind::Text;
    literal.text = std::move(value);
    return literal;
}

std::string Literal::toString() const
{
    switch (kind)
    {
    case Kind::Null:
        return "null";
    case Kind::Bool:
        return boolean ? "true" : "false";
    case Kind::Int:
        return std::to_string(integer);
    case Kind::Double:
    {
        char buffer[40];
        const auto result = std::to_chars(buffer, buffer + sizeof(buffer), real);
        std::string text(buffer, result.ptr);
        if (text.find_first_of(".eEn") == std::string::npos)
            text += ".0";
        return text;
    }
    case Kind::Text:
    {
        std::string quoted = "'";
        for (const char character : text)
        {
            if (character == '\'' || character == '\\')
                quoted += '\\';
            quoted += character;
        }
        return quoted + "'";
    }
    }
    return "?";
}

namespace
{

// --- the tokens
// -------------------------------------------------------------------------------------------------------

enum class TokenKind : std::uint8_t
{
    Number,
    Text,
    Name,
    Operator, // + - * / % ** == != <> < <= > >= =
    LeftParenthesis,
    RightParenthesis,
    LeftBracket,
    RightBracket,
    Comma,
    End
};

struct Token
{
    TokenKind kind = TokenKind::End;
    std::string text;
    std::size_t position = 0;
    bool isInteger = false;
    std::int64_t integer = 0;
    double real = 0.0;
};

bool isNameStart(char character)
{
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_';
}

bool isDigit(char character)
{
    return character >= '0' && character <= '9';
}

class Tokenizer
{
public:
    explicit Tokenizer(std::string_view text) : m_text(text) {}

    [[noreturn]] void fail(const std::string& what, std::size_t position) const
    {
        throw QueryError("bad expression '" + std::string(m_text) + "': " + what + " at position " +
                         std::to_string(position + 1));
    }

    std::vector<Token> run()
    {
        std::vector<Token> tokens;
        std::size_t at = 0;
        while (true)
        {
            while (at < m_text.size() &&
                   (m_text[at] == ' ' || m_text[at] == '\t' || m_text[at] == '\n' || m_text[at] == '\r'))
                ++at;
            Token token;
            token.position = at;
            if (at >= m_text.size())
            {
                token.kind = TokenKind::End;
                tokens.push_back(token);
                return tokens;
            }
            const char character = m_text[at];
            if (isDigit(character) || (character == '.' && at + 1 < m_text.size() && isDigit(m_text[at + 1])))
                at = number(at, token);
            else if (isNameStart(character))
            {
                std::size_t end = at;
                while (end < m_text.size() && (isNameStart(m_text[end]) || isDigit(m_text[end])))
                    ++end;
                token.kind = TokenKind::Name;
                token.text = std::string(m_text.substr(at, end - at));
                at = end;
            }
            else if (character == '\'' || character == '"')
                at = quoted(at, token);
            else
                at = punctuation(at, token);
            tokens.push_back(std::move(token));
        }
    }

private:
    std::size_t number(std::size_t at, Token& token) const
    {
        std::size_t end = at;
        bool real = false;
        while (end < m_text.size() && isDigit(m_text[end]))
            ++end;
        if (end < m_text.size() && m_text[end] == '.')
        {
            real = true;
            ++end;
            while (end < m_text.size() && isDigit(m_text[end]))
                ++end;
        }
        if (end < m_text.size() && (m_text[end] == 'e' || m_text[end] == 'E'))
        {
            std::size_t look = end + 1;
            if (look < m_text.size() && (m_text[look] == '+' || m_text[look] == '-'))
                ++look;
            if (look < m_text.size() && isDigit(m_text[look]))
            {
                real = true;
                while (look < m_text.size() && isDigit(m_text[look]))
                    ++look;
                end = look;
            }
        }
        const char* first = m_text.data() + at;
        const char* last = m_text.data() + end;
        token.kind = TokenKind::Number;
        token.text = std::string(m_text.substr(at, end - at));
        if (!real)
        {
            const auto result = std::from_chars(first, last, token.integer);
            if (result.ec == std::errc())
            {
                token.isInteger = true;
                return end;
            }
            // an integer too large for 64 bits is a real number
        }
        const auto result = std::from_chars(first, last, token.real);
        if (result.ec != std::errc())
            fail("the number '" + token.text + "' is not valid", at);
        return end;
    }

    std::size_t quoted(std::size_t at, Token& token) const
    {
        const char quote = m_text[at];
        std::size_t end = at + 1;
        std::string value;
        while (end < m_text.size() && m_text[end] != quote)
        {
            if (m_text[end] == '\\' && end + 1 < m_text.size())
                ++end;
            value += m_text[end];
            ++end;
        }
        if (end >= m_text.size())
            fail("the text is not closed", at);
        token.kind = TokenKind::Text;
        token.text = std::move(value);
        return end + 1;
    }

    std::size_t punctuation(std::size_t at, Token& token) const
    {
        const char character = m_text[at];
        const char next = at + 1 < m_text.size() ? m_text[at + 1] : '\0';
        const auto op = [&](std::string text, std::size_t length)
        {
            token.kind = TokenKind::Operator;
            token.text = std::move(text);
            return at + length;
        };
        switch (character)
        {
        case '(':
            token.kind = TokenKind::LeftParenthesis;
            return at + 1;
        case ')':
            token.kind = TokenKind::RightParenthesis;
            return at + 1;
        case '[':
            token.kind = TokenKind::LeftBracket;
            return at + 1;
        case ']':
            token.kind = TokenKind::RightBracket;
            return at + 1;
        case ',':
            token.kind = TokenKind::Comma;
            return at + 1;
        case '+':
        case '-':
        case '/':
        case '%':
            return op(std::string(1, character), 1);
        case '*':
            return next == '*' ? op("**", 2) : op("*", 1);
        case '=':
            return next == '=' ? op("==", 2) : op("==", 1);
        case '!':
            if (next == '=')
                return op("!=", 2);
            break;
        case '<':
            if (next == '=')
                return op("<=", 2);
            if (next == '>')
                return op("!=", 2);
            return op("<", 1);
        case '>':
            return next == '=' ? op(">=", 2) : op(">", 1);
        default:
            break;
        }
        fail(std::string("unexpected character '") + character + "'", at);
    }

    std::string_view m_text;
};

// --- the parser
// -------------------------------------------------------------------------------------------------------

using NodePointer = std::shared_ptr<const Expression::Node>;

NodePointer makeNode(Expression::Node node)
{
    return std::make_shared<const Expression::Node>(std::move(node));
}

NodePointer binary(BinaryOp op, NodePointer left, NodePointer right)
{
    Expression::Node node;
    node.kind = NodeKind::Binary;
    node.op = op;
    node.left = std::move(left);
    node.right = std::move(right);
    return makeNode(std::move(node));
}

std::string lowered(std::string_view text)
{
    std::string result(text);
    for (char& character : result)
    {
        if (character >= 'A' && character <= 'Z')
            character = static_cast<char>(character - 'A' + 'a');
    }
    return result;
}

class Parser
{
public:
    Parser(std::string_view text, std::vector<Token> tokens) : m_text(text), m_tokens(std::move(tokens)) {}

    NodePointer run()
    {
        NodePointer root = parseOr();
        if (peek().kind != TokenKind::End)
            fail("unexpected '" + describe(peek()) + "'");
        return root;
    }

private:
    [[noreturn]] void fail(const std::string& what) const
    {
        throw QueryError("bad expression '" + std::string(m_text) + "': " + what + " at position " +
                         std::to_string(peek().position + 1));
    }

    [[nodiscard]] const Token& peek(std::size_t ahead = 0) const
    {
        return m_tokens[std::min(m_next + ahead, m_tokens.size() - 1)];
    }
    Token take() { return m_tokens[std::min(m_next++, m_tokens.size() - 1)]; }

    static std::string describe(const Token& token)
    {
        return token.kind == TokenKind::End ? "the end of the expression" : token.text;
    }

    [[nodiscard]] bool isWord(const Token& token, std::string_view word) const
    {
        return token.kind == TokenKind::Name && lowered(token.text) == word;
    }
    [[nodiscard]] bool isOperator(const Token& token, std::string_view op) const
    {
        return token.kind == TokenKind::Operator && token.text == op;
    }

    NodePointer parseOr()
    {
        NodePointer left = parseAnd();
        while (isWord(peek(), "or"))
        {
            take();
            left = binary(BinaryOp::Or, std::move(left), parseAnd());
        }
        return left;
    }

    NodePointer parseAnd()
    {
        NodePointer left = parseNot();
        while (isWord(peek(), "and"))
        {
            take();
            left = binary(BinaryOp::And, std::move(left), parseNot());
        }
        return left;
    }

    NodePointer parseNot()
    {
        if (isWord(peek(), "not"))
        {
            take();
            Expression::Node node;
            node.kind = NodeKind::Not;
            node.left = parseNot();
            return makeNode(std::move(node));
        }
        return parseComparison();
    }

    NodePointer parseComparison()
    {
        NodePointer left = parseAdditive();
        const Token& next = peek();
        if (next.kind == TokenKind::Operator)
        {
            BinaryOp op = BinaryOp::Equal;
            bool known = true;
            if (next.text == "==")
                op = BinaryOp::Equal;
            else if (next.text == "!=")
                op = BinaryOp::NotEqual;
            else if (next.text == "<")
                op = BinaryOp::Less;
            else if (next.text == "<=")
                op = BinaryOp::LessEqual;
            else if (next.text == ">")
                op = BinaryOp::Greater;
            else if (next.text == ">=")
                op = BinaryOp::GreaterEqual;
            else
                known = false;
            if (known)
            {
                take();
                NodePointer result = binary(op, std::move(left), parseAdditive());
                if (peek().kind == TokenKind::Operator &&
                    (peek().text == "==" || peek().text == "!=" || peek().text == "<" || peek().text == "<=" ||
                     peek().text == ">" || peek().text == ">="))
                    fail("a comparison cannot be chained: join the two with 'and'");
                return result;
            }
            return left;
        }
        bool negated = false;
        std::size_t ahead = 0;
        if (isWord(next, "not") && isWord(peek(1), "in"))
        {
            negated = true;
            ahead = 1;
        }
        if (isWord(peek(ahead), "in"))
        {
            for (std::size_t skip = 0; skip <= ahead; ++skip)
                take();
            Expression::Node node;
            node.kind = NodeKind::In;
            node.left = std::move(left);
            node.negated = negated;
            node.list = parseLiteralList();
            return makeNode(std::move(node));
        }
        if (isWord(next, "is"))
        {
            take();
            Expression::Node node;
            node.kind = NodeKind::IsNull;
            node.left = std::move(left);
            if (isWord(peek(), "not"))
            {
                take();
                node.negated = true;
            }
            if (!isWord(peek(), "null"))
                fail("expected 'null' after 'is'");
            take();
            return makeNode(std::move(node));
        }
        return left;
    }

    std::vector<Literal> parseLiteralList()
    {
        const TokenKind close =
            peek().kind == TokenKind::LeftBracket ? TokenKind::RightBracket : TokenKind::RightParenthesis;
        if (peek().kind != TokenKind::LeftParenthesis && peek().kind != TokenKind::LeftBracket)
            fail("expected a list of values in parentheses after 'in'");
        take();
        std::vector<Literal> values;
        while (peek().kind != close)
        {
            values.push_back(parseLiteral());
            if (peek().kind == TokenKind::Comma)
                take();
            else if (peek().kind != close)
                fail("expected ',' or the end of the list");
        }
        take();
        return values;
    }

    Literal parseLiteral()
    {
        bool negative = false;
        if (isOperator(peek(), "-"))
        {
            take();
            negative = true;
        }
        const Token token = take();
        if (token.kind == TokenKind::Number)
        {
            if (token.isInteger)
                return Literal::ofInt(negative ? -token.integer : token.integer);
            return Literal::ofDouble(negative ? -token.real : token.real);
        }
        if (negative)
            fail("expected a number after '-'");
        if (token.kind == TokenKind::Text)
            return Literal::ofText(token.text);
        if (isWord(token, "true"))
            return Literal::ofBool(true);
        if (isWord(token, "false"))
            return Literal::ofBool(false);
        if (isWord(token, "null"))
            return Literal::null();
        --m_next;
        fail("a list holds numbers, texts, true, false and null");
    }

    NodePointer parseAdditive()
    {
        NodePointer left = parseMultiplicative();
        while (isOperator(peek(), "+") || isOperator(peek(), "-"))
        {
            const BinaryOp op = take().text == "+" ? BinaryOp::Add : BinaryOp::Subtract;
            left = binary(op, std::move(left), parseMultiplicative());
        }
        return left;
    }

    NodePointer parseMultiplicative()
    {
        NodePointer left = parseUnary();
        while (isOperator(peek(), "*") || isOperator(peek(), "/") || isOperator(peek(), "%"))
        {
            const std::string text = take().text;
            const BinaryOp op = text == "*" ? BinaryOp::Multiply : (text == "/" ? BinaryOp::Divide : BinaryOp::Modulo);
            left = binary(op, std::move(left), parseUnary());
        }
        return left;
    }

    NodePointer parseUnary()
    {
        if (isOperator(peek(), "-"))
        {
            take();
            Expression::Node node;
            node.kind = NodeKind::Negate;
            node.left = parseUnary();
            return makeNode(std::move(node));
        }
        if (isOperator(peek(), "+"))
        {
            take();
            return parseUnary();
        }
        return parsePower();
    }

    NodePointer parsePower()
    {
        NodePointer base = parsePrimary();
        if (isOperator(peek(), "**"))
        {
            take();
            return binary(BinaryOp::Power, std::move(base), parseUnary());
        }
        return base;
    }

    NodePointer parsePrimary()
    {
        const Token token = take();
        Expression::Node node;
        switch (token.kind)
        {
        case TokenKind::Number:
            node.kind = NodeKind::Literal;
            node.literal = token.isInteger ? Literal::ofInt(token.integer) : Literal::ofDouble(token.real);
            return makeNode(std::move(node));
        case TokenKind::Text:
            node.kind = NodeKind::Literal;
            node.literal = Literal::ofText(token.text);
            return makeNode(std::move(node));
        case TokenKind::LeftParenthesis:
        {
            NodePointer inner = parseOr();
            if (peek().kind != TokenKind::RightParenthesis)
                fail("expected ')'");
            take();
            return inner;
        }
        case TokenKind::Name:
            return parseName(token);
        default:
            --m_next;
            fail("expected a value or a column name, found " +
                 (token.kind == TokenKind::End ? describe(token) : "'" + token.text + "'"));
        }
    }

    NodePointer parseName(const Token& token)
    {
        const std::string lower = lowered(token.text);
        Expression::Node node;
        if (lower == "true" || lower == "false")
        {
            node.kind = NodeKind::Literal;
            node.literal = Literal::ofBool(lower == "true");
            return makeNode(std::move(node));
        }
        if (peek().kind == TokenKind::LeftParenthesis)
        {
            node.kind = NodeKind::Call;
            if (lower == "abs")
                node.function = Function::Abs;
            else if (lower == "sqrt")
                node.function = Function::Sqrt;
            else if (lower == "isnull")
                node.function = Function::IsNull;
            else if (lower == "isnan")
                node.function = Function::IsNan;
            else
            {
                --m_next;
                fail("unknown function '" + token.text + "' (abs, sqrt, isnull and isnan exist)");
            }
            take();
            node.arguments.push_back(parseOr());
            if (peek().kind != TokenKind::RightParenthesis)
                fail("expected ')' after the argument of " + lower);
            take();
            return makeNode(std::move(node));
        }
        if (lower == "and" || lower == "or" || lower == "not" || lower == "in" || lower == "is" || lower == "null")
        {
            --m_next;
            fail("expected a value, found the word '" + token.text + "'");
        }
        node.kind = NodeKind::Column;
        node.name = token.text;
        return makeNode(std::move(node));
    }

    std::string_view m_text;
    std::vector<Token> m_tokens;
    std::size_t m_next = 0;
};

// --- walking the tree
// --------------------------------------------------------------------------------------------------

void collectColumns(const Expression::Node* node, std::vector<std::string>& names)
{
    if (node == nullptr)
        return;
    if (node->kind == NodeKind::Column && std::find(names.begin(), names.end(), node->name) == names.end())
        names.push_back(node->name);
    collectColumns(node->left.get(), names);
    collectColumns(node->right.get(), names);
    for (const NodePointer& argument : node->arguments)
        collectColumns(argument.get(), names);
}

const char* operatorText(BinaryOp op)
{
    switch (op)
    {
    case BinaryOp::Add:
        return "+";
    case BinaryOp::Subtract:
        return "-";
    case BinaryOp::Multiply:
        return "*";
    case BinaryOp::Divide:
        return "/";
    case BinaryOp::Modulo:
        return "%";
    case BinaryOp::Power:
        return "**";
    case BinaryOp::Equal:
        return "==";
    case BinaryOp::NotEqual:
        return "!=";
    case BinaryOp::Less:
        return "<";
    case BinaryOp::LessEqual:
        return "<=";
    case BinaryOp::Greater:
        return ">";
    case BinaryOp::GreaterEqual:
        return ">=";
    case BinaryOp::And:
        return "and";
    case BinaryOp::Or:
        return "or";
    }
    return "?";
}

const char* functionText(Function function)
{
    switch (function)
    {
    case Function::Abs:
        return "abs";
    case Function::Sqrt:
        return "sqrt";
    case Function::IsNull:
        return "isnull";
    case Function::IsNan:
        return "isnan";
    }
    return "?";
}

std::string nodeText(const Expression::Node& node)
{
    switch (node.kind)
    {
    case NodeKind::Literal:
        return node.literal.toString();
    case NodeKind::Column:
        return node.name;
    case NodeKind::Negate:
        return "-" + nodeText(*node.left);
    case NodeKind::Not:
        return "not " + nodeText(*node.left);
    case NodeKind::Binary:
        return "(" + nodeText(*node.left) + " " + operatorText(node.op) + " " + nodeText(*node.right) + ")";
    case NodeKind::In:
    {
        std::string text = "(" + nodeText(*node.left) + (node.negated ? " not in (" : " in (");
        for (std::size_t index = 0; index < node.list.size(); ++index)
            text += (index == 0 ? "" : ", ") + node.list[index].toString();
        return text + "))";
    }
    case NodeKind::IsNull:
        return "(" + nodeText(*node.left) + (node.negated ? " is not null)" : " is null)");
    case NodeKind::Call:
        return std::string(functionText(node.function)) + "(" + nodeText(*node.arguments.front()) + ")";
    }
    return "?";
}

bool isNumber(ValueType type)
{
    return type == ValueType::Int || type == ValueType::Double || type == ValueType::Bool;
}

ValueType typeOfNode(const Expression::Node& node,
                     const std::function<std::optional<ColumnType>(const std::string&)>& typeOfColumn)
{
    const auto fail = [&](const std::string& what) { throw QueryTypeError(what + " in '" + nodeText(node) + "'"); };
    switch (node.kind)
    {
    case NodeKind::Literal:
        switch (node.literal.kind)
        {
        case Literal::Kind::Bool:
            return ValueType::Bool;
        case Literal::Kind::Int:
            return ValueType::Int;
        case Literal::Kind::Double:
            return ValueType::Double;
        case Literal::Kind::Text:
            return ValueType::Text;
        case Literal::Kind::Null:
            return ValueType::Double;
        }
        return ValueType::Double;
    case NodeKind::Column:
    {
        const std::optional<ColumnType> type = typeOfColumn(node.name);
        if (!type)
            throw QueryTypeError("column '" + node.name + "' holds lists: it cannot be used in an expression");
        switch (*type)
        {
        case ColumnType::Bool:
            return ValueType::Bool;
        case ColumnType::UInt8:
        case ColumnType::Int32:
        case ColumnType::UInt32:
        case ColumnType::Int64:
            return ValueType::Int;
        case ColumnType::Float32:
        case ColumnType::Float64:
            return ValueType::Double;
        case ColumnType::Utf8:
        case ColumnType::Dictionary:
            return ValueType::Text;
        default:
            throw QueryTypeError("column '" + node.name + "' holds lists: it cannot be used in an expression");
        }
    }
    case NodeKind::Negate:
    {
        const ValueType operand = typeOfNode(*node.left, typeOfColumn);
        if (!isNumber(operand))
            fail("a " + std::string(valueTypeName(operand)) + " cannot be negated");
        return operand == ValueType::Double ? ValueType::Double : ValueType::Int;
    }
    case NodeKind::Not:
        if (typeOfNode(*node.left, typeOfColumn) != ValueType::Bool)
            fail("'not' needs a condition");
        return ValueType::Bool;
    case NodeKind::Binary:
    {
        const ValueType left = typeOfNode(*node.left, typeOfColumn);
        const ValueType right = typeOfNode(*node.right, typeOfColumn);
        switch (node.op)
        {
        case BinaryOp::And:
        case BinaryOp::Or:
            if (left != ValueType::Bool || right != ValueType::Bool)
                fail("'" + std::string(operatorText(node.op)) + "' needs two conditions, not a " +
                     std::string(valueTypeName(left != ValueType::Bool ? left : right)));
            return ValueType::Bool;
        case BinaryOp::Equal:
        case BinaryOp::NotEqual:
        case BinaryOp::Less:
        case BinaryOp::LessEqual:
        case BinaryOp::Greater:
        case BinaryOp::GreaterEqual:
            if ((left == ValueType::Text) != (right == ValueType::Text))
                fail("a " + std::string(valueTypeName(left)) + " cannot be compared with a " +
                     std::string(valueTypeName(right)));
            return ValueType::Bool;
        default:
            break;
        }
        if (!isNumber(left) || !isNumber(right))
            fail("'" + std::string(operatorText(node.op)) + "' needs numbers, not a " +
                 std::string(valueTypeName(!isNumber(left) ? left : right)));
        const bool integers = left != ValueType::Double && right != ValueType::Double;
        if (node.op == BinaryOp::Divide || node.op == BinaryOp::Power)
            return ValueType::Double;
        return integers ? ValueType::Int : ValueType::Double;
    }
    case NodeKind::In:
    {
        const ValueType operand = typeOfNode(*node.left, typeOfColumn);
        for (const Literal& literal : node.list)
        {
            if (literal.kind == Literal::Kind::Null)
                continue;
            const bool text = literal.kind == Literal::Kind::Text;
            if ((operand == ValueType::Text) != text)
                fail("a " + std::string(valueTypeName(operand)) + " cannot be looked up in a list of " +
                     (text ? "texts" : "numbers"));
        }
        return ValueType::Bool;
    }
    case NodeKind::IsNull:
        (void)typeOfNode(*node.left, typeOfColumn);
        return ValueType::Bool;
    case NodeKind::Call:
    {
        const ValueType operand = typeOfNode(*node.arguments.front(), typeOfColumn);
        switch (node.function)
        {
        case Function::IsNull:
            return ValueType::Bool;
        case Function::IsNan:
            if (!isNumber(operand))
                fail("isnan needs a number");
            return ValueType::Bool;
        case Function::Sqrt:
            if (!isNumber(operand))
                fail("sqrt needs a number");
            return ValueType::Double;
        case Function::Abs:
            if (!isNumber(operand))
                fail("abs needs a number");
            return operand == ValueType::Double ? ValueType::Double : ValueType::Int;
        }
    }
    }
    return ValueType::Double;
}

void splitConjuncts(const NodePointer& node, std::vector<NodePointer>& parts)
{
    if (node->kind == NodeKind::Binary && node->op == BinaryOp::And)
    {
        splitConjuncts(node->left, parts);
        splitConjuncts(node->right, parts);
        return;
    }
    parts.push_back(node);
}

} // namespace

// The expression class keeps its column list next to the tree.
namespace
{
std::vector<std::string> columnsOf(const NodePointer& root)
{
    std::vector<std::string> names;
    collectColumns(root.get(), names);
    return names;
}
} // namespace

Expression Expression::parse(std::string_view text)
{
    Tokenizer tokenizer(text);
    Parser parser(text, tokenizer.run());
    Expression expression;
    expression.m_root = parser.run();
    expression.m_columns = columnsOf(expression.m_root);
    return expression;
}

Expression Expression::equals(const std::string& column, const Literal& value)
{
    Node name;
    name.kind = NodeKind::Column;
    name.name = column;
    Expression expression;
    if (value.kind == Literal::Kind::Null)
    {
        Node node;
        node.kind = NodeKind::IsNull;
        node.left = makeNode(std::move(name));
        expression.m_root = makeNode(std::move(node));
    }
    else
    {
        Node constant;
        constant.kind = NodeKind::Literal;
        constant.literal = value;
        expression.m_root = binary(BinaryOp::Equal, makeNode(std::move(name)), makeNode(std::move(constant)));
    }
    expression.m_columns = columnsOf(expression.m_root);
    return expression;
}

Expression Expression::isIn(const std::string& column, const std::vector<Literal>& values)
{
    Node name;
    name.kind = NodeKind::Column;
    name.name = column;
    Node node;
    node.kind = NodeKind::In;
    node.left = makeNode(std::move(name));
    node.list = values;
    Expression expression;
    expression.m_root = makeNode(std::move(node));
    expression.m_columns = columnsOf(expression.m_root);
    return expression;
}

Expression Expression::allOf(const std::vector<Expression>& parts)
{
    Expression expression;
    for (const Expression& part : parts)
    {
        if (part.isEmpty())
            continue;
        expression.m_root =
            expression.m_root == nullptr ? part.m_root : binary(BinaryOp::And, expression.m_root, part.m_root);
    }
    expression.m_columns = columnsOf(expression.m_root);
    return expression;
}

std::vector<Expression> Expression::conjuncts() const
{
    std::vector<NodePointer> parts;
    if (m_root != nullptr)
        splitConjuncts(m_root, parts);
    std::vector<Expression> result;
    result.reserve(parts.size());
    for (const NodePointer& part : parts)
    {
        Expression expression;
        expression.m_root = part;
        expression.m_columns = columnsOf(part);
        result.push_back(std::move(expression));
    }
    return result;
}

std::string Expression::toString() const
{
    return m_root == nullptr ? std::string() : nodeText(*m_root);
}

std::optional<std::string> Expression::asColumnName() const
{
    if (m_root != nullptr && m_root->kind == NodeKind::Column)
        return m_root->name;
    return std::nullopt;
}

ValueType Expression::resultType(const std::function<std::optional<ColumnType>(const std::string&)>& typeOfColumn) const
{
    if (m_root == nullptr)
        throw QueryError("the expression is empty");
    return typeOfNode(*m_root, typeOfColumn);
}

} // namespace oor::query
