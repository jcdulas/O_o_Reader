#pragma once

#include "oor/model/cards/solution_control/fatigueProp/FatiguePropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FatiguePropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FatiguePropCard& card);
    [[nodiscard]] static std::optional<FatiguePropCard> read(FieldReader& reader);
    static void write(const FatiguePropCard& card, FieldWriter& writer);
};

// All the PFTG cards. The ID is unique among them (IdSpace::PerCardType) and is what an FTGDEF names: the category
// FatigueProperty. A PFTG names no other card. The cards are kept whole (CardRows) next to their ID: they are few, and
// the query engine reads the ID.
class FatiguePropTable final : public TypedTable<FatiguePropTable, FatiguePropCard>
{
public:
    static constexpr std::string_view NAME = "FatigueProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::FatigueProperty, base::IdSpace::PerCardType};
    using Format = FatiguePropFormat;

    [[nodiscard]] static base::EntityId cardId(const FatiguePropCard& card) noexcept { return card.id; }
    std::uint32_t append(const FatiguePropCard& card, base::CardKey key);
    [[nodiscard]] FatiguePropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FatiguePropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a PFTG refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FatiguePropCard> m_cards;
};

} // namespace oor::model
