#pragma once

#include "oor/model/cards/superelements/superelementReleasedDofs/SuperelementReleasedDofsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementReleasedDofsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementReleasedDofsCard& card);
    [[nodiscard]] static std::optional<SuperelementReleasedDofsCard> read(FieldReader& reader);
    static void write(const SuperelementReleasedDofsCard& card, FieldWriter& writer);
};

// All the RELEASE cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to a
// column of their SEIDs.
class SuperelementReleasedDofsTable final
    : public TypedTable<SuperelementReleasedDofsTable, SuperelementReleasedDofsCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementReleasedDofs";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementReleasedDofsFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementReleasedDofsCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementReleasedDofsCard& card, base::CardKey key);
    [[nodiscard]] SuperelementReleasedDofsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementReleasedDofsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID is a superelement and the points are those of it.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementReleasedDofsCard> m_cards;
};

} // namespace oor::model
