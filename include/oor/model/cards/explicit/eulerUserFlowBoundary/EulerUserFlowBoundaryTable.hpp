#pragma once

#include "oor/model/cards/explicit/eulerUserFlowBoundary/EulerUserFlowBoundaryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerUserFlowBoundaryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerUserFlowBoundaryCard& card);
    [[nodiscard]] static std::optional<EulerUserFlowBoundaryCard> read(FieldReader& reader);
    static void write(const EulerUserFlowBoundaryCard& card, FieldWriter& writer);
};

// All the FLOWUDS cards. The LID is a set ID shared by the cards of the set (IdSpace::Group, category Other: no typed
// card names it). It is the excitation set that a TLOAD1 names in its EXCITEID, so the table says it defines excitation
// IDs (CardTable::definesExcitationIds). The BCID names a set of BCSEG cards (ContactSurface) and the MESH an Euler
// mesh. The cards are kept whole (CardRows) next to their ID.
class EulerUserFlowBoundaryTable final : public TypedTable<EulerUserFlowBoundaryTable, EulerUserFlowBoundaryCard>
{
public:
    static constexpr std::string_view NAME = "EulerUserFlowBoundary";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = EulerUserFlowBoundaryFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerUserFlowBoundaryCard& card) noexcept { return card.setId; }
    std::uint32_t append(const EulerUserFlowBoundaryCard& card, base::CardKey key);
    [[nodiscard]] EulerUserFlowBoundaryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerUserFlowBoundaryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] bool definesExcitationIds() const noexcept override { return true; }
    [[nodiscard]] base::EntityId definedExcitationId(std::uint32_t row) const override { return m_setId.get(row); }

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }

private:
    Column<base::EntityId> m_setId;
    CardRows<EulerUserFlowBoundaryCard> m_cards;
};

} // namespace oor::model
