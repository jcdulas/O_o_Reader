#pragma once

#include "oor/model/cards/elements/connectorSearchParams/ConnectorSearchParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ConnectorSearchParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ConnectorSearchParamsCard& card);
    [[nodiscard]] static std::optional<ConnectorSearchParamsCard> read(FieldReader& reader);
    static void write(const ConnectorSearchParamsCard& card, FieldWriter& writer);
};

// All the SWLDPRM cards (no ID: IdSpace::None, category Other). The format reference allows one in a deck; a
// second one is kept like the first, and reported by a check across the cards, not by the table.
class ConnectorSearchParamsTable final : public TypedTable<ConnectorSearchParamsTable, ConnectorSearchParamsCard>
{
public:
    static constexpr std::string_view NAME = "ConnectorSearchParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ConnectorSearchParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const ConnectorSearchParamsCard&) noexcept { return 0; }
    std::uint32_t append(const ConnectorSearchParamsCard& card, base::CardKey key);
    [[nodiscard]] ConnectorSearchParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ConnectorSearchParamsCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}
    // Only one SWLDPRM entry is allowed: a second one is reported, the first is the one in force.
    void finishRead(Model& model, const ReadContext& context) override;

private:
    CardRows<ConnectorSearchParamsCard> m_cards;
};

} // namespace oor::model
