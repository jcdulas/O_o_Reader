#pragma once

#include "oor/model/cards/superelements/superelementConnection/SuperelementConnectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementConnectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementConnectionCard& card);
    [[nodiscard]] static std::optional<SuperelementConnectionCard> read(FieldReader& reader);
    static void write(const SuperelementConnectionCard& card, FieldWriter& writer);
};

// All the SECONCT cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to columns
// of the two superelements.
class SuperelementConnectionTable final : public TypedTable<SuperelementConnectionTable, SuperelementConnectionCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementConnection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementConnectionFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementConnectionCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementConnectionCard& card, base::CardKey key);
    [[nodiscard]] SuperelementConnectionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementConnectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEIDA and SEIDB are superelements; the points of each side are those of its superelement
    // (ReferenceVisitor::visitInSuperelement).
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdAColumn() noexcept { return m_superelementIdA; }
    [[nodiscard]] Column<base::EntityId>& superelementIdBColumn() noexcept { return m_superelementIdB; }

private:
    Column<base::EntityId> m_superelementIdA;
    Column<base::EntityId> m_superelementIdB;
    CardRows<SuperelementConnectionCard> m_cards;
};

} // namespace oor::model
