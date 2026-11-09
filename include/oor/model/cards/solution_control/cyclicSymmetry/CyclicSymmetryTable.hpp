#pragma once

#include "oor/model/cards/solution_control/cyclicSymmetry/CyclicSymmetryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CyclicSymmetryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CyclicSymmetryCard& card);
    [[nodiscard]] static std::optional<CyclicSymmetryCard> read(FieldReader& reader);
    static void write(const CyclicSymmetryCard& card, FieldWriter& writer);
};

// All the CYSYM cards (no ID: IdSpace::None, category Other). The card names no other card. It is kept whole next to a
// column of its segment counts.
class CyclicSymmetryTable final : public TypedTable<CyclicSymmetryTable, CyclicSymmetryCard>
{
public:
    static constexpr std::string_view NAME = "CyclicSymmetry";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = CyclicSymmetryFormat;

    [[nodiscard]] static base::EntityId cardId(const CyclicSymmetryCard&) noexcept { return 0; }
    std::uint32_t append(const CyclicSymmetryCard& card, base::CardKey key);
    [[nodiscard]] CyclicSymmetryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CyclicSymmetryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field names another card

    [[nodiscard]] Column<std::int32_t>& segmentsColumn() noexcept { return m_segments; }

private:
    Column<std::int32_t> m_segments;
    CardRows<CyclicSymmetryCard> m_cards;
};

} // namespace oor::model
