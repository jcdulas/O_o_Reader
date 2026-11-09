#include "oor/deck/raw/CardName.hpp"

namespace oor::deck
{
std::optional<CardName> CardName::fromText(std::string_view text)
{
    if (text.empty())
        return std::nullopt;
    for (const unsigned char character : text)
        if (character < 0x20 || character == 0x7F)
            return std::nullopt;
    CardName name;
    name.m_text = std::string(text);
    return name;
}
} // namespace oor::deck
