#pragma once

#include "oor/model/cards/explicit/porousTimeFlowBoundary/PorousTimeFlowBoundaryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PorousTimeFlowBoundaryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PorousTimeFlowBoundaryCard& card);
    [[nodiscard]] static std::optional<PorousTimeFlowBoundaryCard> read(FieldReader& reader);
    static void write(const PorousTimeFlowBoundaryCard& card, FieldWriter& writer);
};

// All the PORFLWT cards: the FID is unique among them (IdSpace::PerCardType, category Porosity). A LEAKAGE names it;
// the MID is a MATDEUL and the four quantities may be TABLED1.
class PorousTimeFlowBoundaryTable final : public TypedTable<PorousTimeFlowBoundaryTable, PorousTimeFlowBoundaryCard>
{
public:
    static constexpr std::string_view NAME = "PorousTimeFlowBoundary";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Porosity, base::IdSpace::PerCardType};
    using Format = PorousTimeFlowBoundaryFormat;

    [[nodiscard]] static base::EntityId cardId(const PorousTimeFlowBoundaryCard& card) noexcept { return card.id; }
    std::uint32_t append(const PorousTimeFlowBoundaryCard& card, base::CardKey key);
    [[nodiscard]] PorousTimeFlowBoundaryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PorousTimeFlowBoundaryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PorousTimeFlowBoundaryCard> m_cards;
};

} // namespace oor::model
