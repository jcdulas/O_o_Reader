#pragma once

#include "oor/model/cards/explicit/porousFlowBoundary/PorousFlowBoundaryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PorousFlowBoundaryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PorousFlowBoundaryCard& card);
    [[nodiscard]] static std::optional<PorousFlowBoundaryCard> read(FieldReader& reader);
    static void write(const PorousFlowBoundaryCard& card, FieldWriter& writer);
};

// All the PORFLOW cards: the FID is unique among them (IdSpace::PerCardType, category Porosity). A LEAKAGE names it;
// the value of a MATERIAL pair is a MATDEUL.
class PorousFlowBoundaryTable final : public TypedTable<PorousFlowBoundaryTable, PorousFlowBoundaryCard>
{
public:
    static constexpr std::string_view NAME = "PorousFlowBoundary";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Porosity, base::IdSpace::PerCardType};
    using Format = PorousFlowBoundaryFormat;

    [[nodiscard]] static base::EntityId cardId(const PorousFlowBoundaryCard& card) noexcept { return card.id; }
    std::uint32_t append(const PorousFlowBoundaryCard& card, base::CardKey key);
    [[nodiscard]] PorousFlowBoundaryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PorousFlowBoundaryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PorousFlowBoundaryCard> m_cards;
};

} // namespace oor::model
