#pragma once

#include "oor/model/cards/explicit/eulerDefaultFlowBoundary/EulerDefaultFlowBoundaryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerDefaultFlowBoundaryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerDefaultFlowBoundaryCard& card);
    [[nodiscard]] static std::optional<EulerDefaultFlowBoundaryCard> read(FieldReader& reader);
    static void write(const EulerDefaultFlowBoundaryCard& card, FieldWriter& writer);
};

// All the FLOWDEF cards. The FID is unique among them (IdSpace::PerCardType) and no other card names it, so the
// category is Other. A default boundary is not excited by a load, so the table does not define an excitation ID. A
// MATERIAL or HYDSTAT property names a MATDEUL or a HYDSTAT. The cards are kept whole (CardRows) next to their ID.
class EulerDefaultFlowBoundaryTable final
    : public TypedTable<EulerDefaultFlowBoundaryTable, EulerDefaultFlowBoundaryCard>
{
public:
    static constexpr std::string_view NAME = "EulerDefaultFlowBoundary";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = EulerDefaultFlowBoundaryFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerDefaultFlowBoundaryCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerDefaultFlowBoundaryCard& card, base::CardKey key);
    [[nodiscard]] EulerDefaultFlowBoundaryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerDefaultFlowBoundaryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerDefaultFlowBoundaryCard> m_cards;
};

} // namespace oor::model
