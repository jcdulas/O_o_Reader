#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

enum class TokenKind : std::uint8_t
{
    Number,
    Name,
    Plus,
    Minus,
    Star,
    Slash,
    Power, // **
    LeftParenthesis,
    RightParenthesis,
    Comma,
    Equals,
    Semicolon,
    End,
};

struct Token
{
    TokenKind kind = TokenKind::End;
    std::string text; // Number: the spelling in upper case; Name: the name in upper case
    double number = 0.0;
    bool isInteger = false;   // Number: no point and no exponent
    std::size_t position = 1; // of the first character, 1 is the first character of the text
};

// Cuts the text of a DEQATN into tokens. Blanks mean nothing (the lines of the card are joined as they are, and the format reference
// says the equation may hold embedded blanks), the case means nothing, and a number is the Fortran one: digits with an
// optional point and an optional exponent that starts with E or D.
class ExpressionLexer
{
public:
    explicit ExpressionLexer(std::string_view text) noexcept : m_text(text) {}

    // The tokens, ending with an End token. When a character cannot start a token, `problem()` says so and the tokens
    // end there.
    [[nodiscard]] std::vector<Token> tokens();
    [[nodiscard]] const std::string& problem() const noexcept { return m_problem; }
    [[nodiscard]] std::size_t problemPosition() const noexcept { return m_problemPosition; }

private:
    [[nodiscard]] bool atEnd() const noexcept { return m_index >= m_text.size(); }
    Token readNumber();
    Token readName();
    void fail(std::string message, std::size_t index);

    std::string_view m_text;
    std::size_t m_index = 0;
    std::string m_problem;
    std::size_t m_problemPosition = 1;
};

} // namespace oor::model
