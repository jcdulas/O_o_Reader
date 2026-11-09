#include "ExpressionLexer.hpp"

#include <cctype>
#include <charconv>

namespace oor::model
{

namespace
{

bool isDigit(char character) noexcept
{
    return character >= '0' && character <= '9';
}

bool isNameStart(char character) noexcept
{
    return std::isalpha(static_cast<unsigned char>(character)) != 0 || character == '_';
}

bool isNameCharacter(char character) noexcept
{
    return isNameStart(character) || isDigit(character);
}

char upper(char character) noexcept
{
    return static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
}

} // namespace

void ExpressionLexer::fail(std::string message, std::size_t index)
{
    if (m_problem.empty())
    {
        m_problem = std::move(message);
        m_problemPosition = index + 1;
    }
}

Token ExpressionLexer::readNumber()
{
    const std::size_t start = m_index;
    std::size_t digits = 0;
    bool isInteger = true;
    while (!atEnd() && isDigit(m_text[m_index]))
    {
        ++m_index;
        ++digits;
    }
    if (!atEnd() && m_text[m_index] == '.')
    {
        isInteger = false;
        ++m_index;
        while (!atEnd() && isDigit(m_text[m_index]))
        {
            ++m_index;
            ++digits;
        }
    }
    if (digits == 0)
    {
        fail("a point must be next to digits", start);
        return Token{};
    }
    // An exponent starts with E or D (a single precision or a double precision constant) and must have digits.
    if (!atEnd() && (upper(m_text[m_index]) == 'E' || upper(m_text[m_index]) == 'D'))
    {
        std::size_t cursor = m_index + 1;
        if (cursor < m_text.size() && (m_text[cursor] == '+' || m_text[cursor] == '-'))
            ++cursor;
        if (cursor >= m_text.size() || !isDigit(m_text[cursor]))
        {
            fail("the exponent of a number needs digits", start);
            return Token{};
        }
        while (cursor < m_text.size() && isDigit(m_text[cursor]))
            ++cursor;
        m_index = cursor;
        isInteger = false;
    }

    Token token;
    token.kind = TokenKind::Number;
    token.isInteger = isInteger;
    token.position = start + 1;
    for (std::size_t index = start; index < m_index; ++index)
        token.text.push_back(upper(m_text[index] == 'd' || m_text[index] == 'D' ? 'E' : m_text[index]));
    // from_chars takes the Fortran spelling once D is E; a point at either end is fine ("1." and ".5").
    const char* const first = token.text.data();
    const char* const last = first + token.text.size();
    const auto [end, error] = std::from_chars(first, last, token.number);
    if (error != std::errc{} || end != last)
    {
        fail("the number " + token.text + " is out of range", start);
        return Token{};
    }
    return token;
}

Token ExpressionLexer::readName()
{
    Token token;
    token.kind = TokenKind::Name;
    token.position = m_index + 1;
    while (!atEnd() && isNameCharacter(m_text[m_index]))
    {
        token.text.push_back(upper(m_text[m_index]));
        ++m_index;
    }
    return token;
}

std::vector<Token> ExpressionLexer::tokens()
{
    std::vector<Token> result;
    while (m_problem.empty())
    {
        while (!atEnd() && (m_text[m_index] == ' ' || m_text[m_index] == '\t'))
            ++m_index;
        if (atEnd())
            break;
        const char character = m_text[m_index];
        Token token;
        token.position = m_index + 1;
        if (isDigit(character) || character == '.')
        {
            token = readNumber();
            if (!m_problem.empty())
                break;
            result.push_back(std::move(token));
            continue;
        }
        if (isNameStart(character))
        {
            result.push_back(readName());
            continue;
        }
        switch (character)
        {
        case '+':
            token.kind = TokenKind::Plus;
            break;
        case '-':
            token.kind = TokenKind::Minus;
            break;
        case '/':
            token.kind = TokenKind::Slash;
            break;
        case '(':
            token.kind = TokenKind::LeftParenthesis;
            break;
        case ')':
            token.kind = TokenKind::RightParenthesis;
            break;
        case ',':
            token.kind = TokenKind::Comma;
            break;
        case '=':
            token.kind = TokenKind::Equals;
            break;
        case ';':
            token.kind = TokenKind::Semicolon;
            break;
        case '*':
            if (m_index + 1 < m_text.size() && m_text[m_index + 1] == '*')
            {
                token.kind = TokenKind::Power;
                ++m_index;
            }
            else
            {
                token.kind = TokenKind::Star;
            }
            break;
        default:
            fail(std::string("the character \"") + character + "\" is not part of an equation", m_index);
            break;
        }
        if (!m_problem.empty())
            break;
        ++m_index;
        result.push_back(std::move(token));
    }
    Token end;
    end.kind = TokenKind::End;
    end.position = m_text.size() + 1;
    result.push_back(std::move(end));
    return result;
}

} // namespace oor::model
