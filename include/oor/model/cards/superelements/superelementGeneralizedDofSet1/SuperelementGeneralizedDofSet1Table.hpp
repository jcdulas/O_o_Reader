#pragma once

#include "oor/model/cards/superelements/superelementGeneralizedDofSet1/SuperelementGeneralizedDofSet1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementGeneralizedDofSet1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementGeneralizedDofSet1Card& card);
    [[nodiscard]] static std::optional<SuperelementGeneralizedDofSet1Card> read(FieldReader& reader);
    static void write(const SuperelementGeneralizedDofSet1Card& card, FieldWriter& writer);
};

// All the SEQSET1 cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to a
// column of their SEIDs.
class SuperelementGeneralizedDofSet1Table final
    : public TypedTable<SuperelementGeneralizedDofSet1Table, SuperelementGeneralizedDofSet1Card>
{
public:
    static constexpr std::string_view NAME = "SuperelementGeneralizedDofSet1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementGeneralizedDofSet1Format;

    [[nodiscard]] static base::EntityId cardId(const SuperelementGeneralizedDofSet1Card&) noexcept { return 0; }
    std::uint32_t append(const SuperelementGeneralizedDofSet1Card& card, base::CardKey key);
    [[nodiscard]] SuperelementGeneralizedDofSet1Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementGeneralizedDofSet1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID is a superelement and the points are those of it; the ends of a THRU range may not exist.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementGeneralizedDofSet1Card> m_cards;
};

} // namespace oor::model
