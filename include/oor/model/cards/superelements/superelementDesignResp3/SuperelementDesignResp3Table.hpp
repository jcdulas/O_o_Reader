#pragma once

#include "oor/model/cards/superelements/superelementDesignResp3/SuperelementDesignResp3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementDesignResp3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementDesignResp3Card& card);
    [[nodiscard]] static std::optional<SuperelementDesignResp3Card> read(FieldReader& reader);
    static void write(const SuperelementDesignResp3Card& card, FieldWriter& writer);
};

// All the SEDRSP3 cards, filed under their ID (IdSpace::PerCardType, category DesignResponse like DRESP3). The cards
// are kept whole next to a column of their IDs.
class SuperelementDesignResp3Table final : public TypedTable<SuperelementDesignResp3Table, SuperelementDesignResp3Card>
{
public:
    static constexpr std::string_view NAME = "SuperelementDesignResp3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DesignResponse, base::IdSpace::PerCardType};
    using Format = SuperelementDesignResp3Format;

    [[nodiscard]] static base::EntityId cardId(const SuperelementDesignResp3Card& card) noexcept { return card.id; }
    std::uint32_t append(const SuperelementDesignResp3Card& card, base::CardKey key);
    [[nodiscard]] SuperelementDesignResp3Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementDesignResp3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    // Every item names a superelement and what it holds.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<SuperelementDesignResp3Card> m_cards;
};

} // namespace oor::model
