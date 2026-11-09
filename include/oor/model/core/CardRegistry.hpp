#pragma once

#include "oor/model/core/CardTable.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The card types the library knows, by name: for each, its description and a way to make an empty table. The list is
// written by hand in src/model/cards/AllCards.cpp (one line per card type). Index 0 is always the table of unknown
// cards (cards kept as text).
//
// A card type is looked up by its exact neutral name ("ShellQuad4").
class CardRegistry
{
public:
    struct Entry
    {
        CardTypeInfo info;
        CardTableFactory factory = nullptr;
    };

    static constexpr std::uint16_t UNKNOWN_TYPE = 0;

    // The registry of all card types, built on first use and then read-only (safe to read from any thread).
    [[nodiscard]] static const CardRegistry& instance();

    [[nodiscard]] std::size_t size() const noexcept { return m_entries.size(); }
    [[nodiscard]] const Entry& entry(std::uint16_t typeIndex) const noexcept { return m_entries[typeIndex]; }
    // The type index of a card type by the name the code gives it (exactly); nullopt for a card the library does not
    // know.
    [[nodiscard]] std::optional<std::uint16_t> typeIndexOf(std::string_view name) const;
    // Used by the list in AllCards.cpp.
    void add(const CardTypeInfo& info, CardTableFactory factory);

private:
    CardRegistry() = default;
    friend struct RegistryBuilder;

    std::vector<Entry> m_entries;
    std::map<std::string, std::uint16_t, std::less<>> m_indexByName; // TableT::NAME
};

// The table type index of a table class T (T::NAME), looked up once.
template <typename TableT>
[[nodiscard]] std::uint16_t typeIndexOf()
{
    static const std::uint16_t index = *CardRegistry::instance().typeIndexOf(TableT::NAME);
    return index;
}

} // namespace oor::model
