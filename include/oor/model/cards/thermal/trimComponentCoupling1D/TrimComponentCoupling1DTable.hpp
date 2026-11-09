#pragma once

#include "oor/model/cards/thermal/trimComponentCoupling1D/TrimComponentCoupling1DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TrimComponentCoupling1DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TrimComponentCoupling1DCard& card);
    [[nodiscard]] static std::optional<TrimComponentCoupling1DCard> read(FieldReader& reader);
    static void write(const TrimComponentCoupling1DCard& card, FieldWriter& writer);
};

// All the ACPMCP1 cards, in the category Other. The TID is the trim component, which no card defines and which several
// ACPMCP1 cards share (IdSpace::Group). The four sets are references to SET3 cards of the part BEGIN BULK TRMC=TID
// (ReferenceTarget::Set, ReferenceVisitor::visitInPart). The cards are kept whole next to a column of their TIDs.
class TrimComponentCoupling1DTable final : public TypedTable<TrimComponentCoupling1DTable, TrimComponentCoupling1DCard>
{
public:
    static constexpr std::string_view NAME = "TrimComponentCoupling1D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = TrimComponentCoupling1DFormat;

    [[nodiscard]] static base::EntityId cardId(const TrimComponentCoupling1DCard& card) noexcept { return card.id; }
    std::uint32_t append(const TrimComponentCoupling1DCard& card, base::CardKey key);
    [[nodiscard]] TrimComponentCoupling1DCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TrimComponentCoupling1DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TrimComponentCoupling1DCard> m_cards;
};

} // namespace oor::model
