#pragma once

#include "oor/model/cards/explicit/couplingFailedSegmentFlow/CouplingFailedSegmentFlowCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CouplingFailedSegmentFlowFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CouplingFailedSegmentFlowCard& card);
    [[nodiscard]] static std::optional<CouplingFailedSegmentFlowCard> read(FieldReader& reader);
    static void write(const CouplingFailedSegmentFlowCard& card, FieldWriter& writer);
};

// All the COUP1FL cards: the CFID is unique among them (IdSpace::PerCardType, category CouplingFailure), and a COUPLE
// names it in its COUP1FL field. The MATERIAL is a MATDEUL.
class CouplingFailedSegmentFlowTable final
    : public TypedTable<CouplingFailedSegmentFlowTable, CouplingFailedSegmentFlowCard>
{
public:
    static constexpr std::string_view NAME = "CouplingFailedSegmentFlow";
    static inline const CardTypeInfo INFO{NAME, CardCategory::CouplingFailure, base::IdSpace::PerCardType};
    using Format = CouplingFailedSegmentFlowFormat;

    [[nodiscard]] static base::EntityId cardId(const CouplingFailedSegmentFlowCard& card) noexcept { return card.id; }
    std::uint32_t append(const CouplingFailedSegmentFlowCard& card, base::CardKey key);
    [[nodiscard]] CouplingFailedSegmentFlowCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CouplingFailedSegmentFlowCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CouplingFailedSegmentFlowCard> m_cards;
};

} // namespace oor::model
