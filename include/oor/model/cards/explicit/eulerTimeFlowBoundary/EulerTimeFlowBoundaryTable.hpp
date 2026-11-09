#pragma once

#include "oor/model/cards/explicit/eulerTimeFlowBoundary/EulerTimeFlowBoundaryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerTimeFlowBoundaryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerTimeFlowBoundaryCard& card);
    [[nodiscard]] static std::optional<EulerTimeFlowBoundaryCard> read(FieldReader& reader);
    static void write(const EulerTimeFlowBoundaryCard& card, FieldWriter& writer);
};

// All the FLOWT cards. The FID is unique among them (IdSpace::PerCardType, category Other: no typed card names it). The
// format says a TLOAD1 must name the number, so the table says it defines excitation IDs
// (CardTable::definesExcitationIds). The BCID names a set of BCSEG cards (ContactSurface), the MESH an Euler mesh, the
// MID a MATDEUL and the quantities that are tables a TABLED1. The cards are kept whole (CardRows) next to their ID.
class EulerTimeFlowBoundaryTable final : public TypedTable<EulerTimeFlowBoundaryTable, EulerTimeFlowBoundaryCard>
{
public:
    static constexpr std::string_view NAME = "EulerTimeFlowBoundary";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = EulerTimeFlowBoundaryFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerTimeFlowBoundaryCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerTimeFlowBoundaryCard& card, base::CardKey key);
    [[nodiscard]] EulerTimeFlowBoundaryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerTimeFlowBoundaryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] bool definesExcitationIds() const noexcept override { return true; }
    [[nodiscard]] base::EntityId definedExcitationId(std::uint32_t row) const override { return m_id.get(row); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerTimeFlowBoundaryCard> m_cards;
};

} // namespace oor::model
