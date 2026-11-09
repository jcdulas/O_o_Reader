#pragma once

#include "oor/model/cards/superelements/superelementBoundarySearch/SuperelementBoundarySearchCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementBoundarySearchFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementBoundarySearchCard& card);
    [[nodiscard]] static std::optional<SuperelementBoundarySearchCard> read(FieldReader& reader);
    static void write(const SuperelementBoundarySearchCard& card, FieldWriter& writer);
};

// All the SEBNDRY cards, category Superelement. A card has no ID of its own (IdSpace::None): it describes a
// superelement that other cards define. They are kept whole next to a column of their SEIDAs.
class SuperelementBoundarySearchTable final
    : public TypedTable<SuperelementBoundarySearchTable, SuperelementBoundarySearchCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementBoundarySearch";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementBoundarySearchFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementBoundarySearchCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementBoundarySearchCard& card, base::CardKey key);
    [[nodiscard]] SuperelementBoundarySearchCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementBoundarySearchCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEIDA and SEIDB are superelements; the points are those of superelement SEIDA.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdAColumn() noexcept { return m_superelementIdA; }

private:
    Column<base::EntityId> m_superelementIdA;
    CardRows<SuperelementBoundarySearchCard> m_cards;
};

} // namespace oor::model
