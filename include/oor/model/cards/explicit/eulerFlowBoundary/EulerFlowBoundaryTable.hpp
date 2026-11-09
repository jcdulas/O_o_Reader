#pragma once

#include "oor/model/cards/explicit/eulerFlowBoundary/EulerFlowBoundaryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerFlowBoundaryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerFlowBoundaryCard& card);
    [[nodiscard]] static std::optional<EulerFlowBoundaryCard> read(FieldReader& reader);
    static void write(const EulerFlowBoundaryCard& card, FieldWriter& writer);
};

// All the FLOW cards. The LID is a set ID shared by the cards of the set (IdSpace::Group, category Other: no typed card
// names it). It is the excitation set that a TLOAD1 names in its EXCITEID, so the table says it defines excitation IDs
// (CardTable::definesExcitationIds). The BCID names a set of BCSEG cards (ContactSurface), the MESH an Euler mesh, and
// a MATERIAL or HYDSTAT property a MATDEUL or a HYDSTAT. The cards are kept whole (CardRows) next to their ID.
class EulerFlowBoundaryTable final : public TypedTable<EulerFlowBoundaryTable, EulerFlowBoundaryCard>
{
public:
    static constexpr std::string_view NAME = "EulerFlowBoundary";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = EulerFlowBoundaryFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerFlowBoundaryCard& card) noexcept { return card.setId; }
    std::uint32_t append(const EulerFlowBoundaryCard& card, base::CardKey key);
    [[nodiscard]] EulerFlowBoundaryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerFlowBoundaryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] bool definesExcitationIds() const noexcept override { return true; }
    [[nodiscard]] base::EntityId definedExcitationId(std::uint32_t row) const override { return m_setId.get(row); }

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }

private:
    Column<base::EntityId> m_setId;
    CardRows<EulerFlowBoundaryCard> m_cards;
};

} // namespace oor::model
