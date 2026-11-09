#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace oor::deck
{

// An exact card name, including forward-compatible names unknown to the registry.
// Nonempty text preserves its spelling and case; ASCII control characters are refused.
class CardName
{
public:
    CardName() = default;
    [[nodiscard]] static std::optional<CardName> fromText(std::string_view text);
    [[nodiscard]] std::string_view view() const noexcept { return m_text; }
    [[nodiscard]] bool empty() const noexcept { return m_text.empty(); }
    [[nodiscard]] bool operator==(const CardName& other) const noexcept { return m_text == other.m_text; }
    [[nodiscard]] bool operator==(std::string_view other) const noexcept { return view() == other; }
    [[nodiscard]] std::size_t hash() const noexcept { return std::hash<std::string_view>{}(m_text); }

private:
    std::string m_text;
};

} // namespace oor::deck

template <>
struct std::hash<oor::deck::CardName>
{
    std::size_t operator()(const oor::deck::CardName& name) const noexcept { return name.hash(); }
};
