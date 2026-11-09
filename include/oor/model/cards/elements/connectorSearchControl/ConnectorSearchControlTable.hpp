#pragma once

#include "oor/model/cards/elements/connectorSearchControl/ConnectorSearchControlCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ConnectorSearchControlFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ConnectorSearchControlCard& card);
    [[nodiscard]] static std::optional<ConnectorSearchControlCard> read(FieldReader& reader);
    static void write(const ConnectorSearchControlCard& card, FieldWriter& writer);
};

// All the CONCTL cards. A card has no ID (IdSpace::None, category Other) and any number of them may be in a deck; its
// SET3ID names a SET3 (`Set`). The cards are kept whole.
class ConnectorSearchControlTable final : public TypedTable<ConnectorSearchControlTable, ConnectorSearchControlCard>
{
public:
    static constexpr std::string_view NAME = "ConnectorSearchControl";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ConnectorSearchControlFormat;

    [[nodiscard]] static base::EntityId cardId(const ConnectorSearchControlCard&) noexcept { return 0; }
    std::uint32_t append(const ConnectorSearchControlCard& card, base::CardKey key);
    [[nodiscard]] ConnectorSearchControlCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ConnectorSearchControlCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

private:
    CardRows<ConnectorSearchControlCard> m_cards;
};

} // namespace oor::model
