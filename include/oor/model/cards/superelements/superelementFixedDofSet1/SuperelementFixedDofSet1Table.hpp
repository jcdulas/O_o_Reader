#pragma once

#include "oor/model/cards/superelements/superelementFixedDofSet1/SuperelementFixedDofSet1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementFixedDofSet1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementFixedDofSet1Card& card);
    [[nodiscard]] static std::optional<SuperelementFixedDofSet1Card> read(FieldReader& reader);
    static void write(const SuperelementFixedDofSet1Card& card, FieldWriter& writer);
};

// All the SEBSET1 cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to a
// column of their SEIDs.
class SuperelementFixedDofSet1Table final
    : public TypedTable<SuperelementFixedDofSet1Table, SuperelementFixedDofSet1Card>
{
public:
    static constexpr std::string_view NAME = "SuperelementFixedDofSet1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementFixedDofSet1Format;

    [[nodiscard]] static base::EntityId cardId(const SuperelementFixedDofSet1Card&) noexcept { return 0; }
    std::uint32_t append(const SuperelementFixedDofSet1Card& card, base::CardKey key);
    [[nodiscard]] SuperelementFixedDofSet1Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementFixedDofSet1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID is a superelement and the points are those of it; the ends of a THRU range may not exist.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementFixedDofSet1Card> m_cards;
};

} // namespace oor::model
