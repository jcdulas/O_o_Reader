#pragma once

#include "oor/model/cards/superelements/superelementFixedDofSet/SuperelementFixedDofSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementFixedDofSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementFixedDofSetCard& card);
    [[nodiscard]] static std::optional<SuperelementFixedDofSetCard> read(FieldReader& reader);
    static void write(const SuperelementFixedDofSetCard& card, FieldWriter& writer);
};

// All the SEBSET cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to a column
// of their SEIDs.
class SuperelementFixedDofSetTable final : public TypedTable<SuperelementFixedDofSetTable, SuperelementFixedDofSetCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementFixedDofSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementFixedDofSetFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementFixedDofSetCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementFixedDofSetCard& card, base::CardKey key);
    [[nodiscard]] SuperelementFixedDofSetCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementFixedDofSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID is a superelement and the points are those of it.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementFixedDofSetCard> m_cards;
};

} // namespace oor::model
