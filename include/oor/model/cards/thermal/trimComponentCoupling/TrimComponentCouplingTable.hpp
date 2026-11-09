#pragma once

#include "oor/model/cards/thermal/trimComponentCoupling/TrimComponentCouplingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TrimComponentCouplingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TrimComponentCouplingCard& card);
    [[nodiscard]] static std::optional<TrimComponentCouplingCard> read(FieldReader& reader);
    static void write(const TrimComponentCouplingCard& card, FieldWriter& writer);
};

// All the ACPEMCP cards, in the category Other. The TID is the trim component, which no card defines (Case Control
// TRIMGRP is text) and which several ACPEMCP cards share (IdSpace::Group). The twelve sets are references to SET1 and
// SET3 cards of the part BEGIN BULK TRMC=TID (ReferenceTarget::Set, ReferenceVisitor::visitInPart). The cards are kept
// whole next to a column of their TIDs.
class TrimComponentCouplingTable final : public TypedTable<TrimComponentCouplingTable, TrimComponentCouplingCard>
{
public:
    static constexpr std::string_view NAME = "TrimComponentCoupling";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = TrimComponentCouplingFormat;

    [[nodiscard]] static base::EntityId cardId(const TrimComponentCouplingCard& card) noexcept { return card.id; }
    std::uint32_t append(const TrimComponentCouplingCard& card, base::CardKey key);
    [[nodiscard]] TrimComponentCouplingCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TrimComponentCouplingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TrimComponentCouplingCard> m_cards;
};

} // namespace oor::model
