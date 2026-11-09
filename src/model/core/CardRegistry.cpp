#include "oor/model/core/CardRegistry.hpp"

#include "oor/model/core/UnknownCardTable.hpp"

namespace oor::model
{

// Defined in src/model/cards/AllCards.cpp: one line per card type.
void registerAllCards(CardRegistry& registry);

const CardRegistry& CardRegistry::instance()
{
    static const CardRegistry REGISTRY = []
    {
        CardRegistry built;
        built.add(CardTypeInfo{UnknownCardTable::NAME, CardCategory::Unknown, base::IdSpace::None},
                  [] { return std::unique_ptr<CardTable>(std::make_unique<UnknownCardTable>()); });
        registerAllCards(built);
        return built;
    }();
    return REGISTRY;
}

void CardRegistry::add(const CardTypeInfo& info, CardTableFactory factory)
{
    const std::uint16_t index = static_cast<std::uint16_t>(m_entries.size());
    m_entries.push_back(Entry{info, factory});
    m_indexByName.emplace(std::string(info.name), index);
}

std::optional<std::uint16_t> CardRegistry::typeIndexOf(std::string_view name) const
{
    const auto found = m_indexByName.find(name);
    if (found == m_indexByName.end())
        return std::nullopt;
    return found->second;
}

} // namespace oor::model
