#pragma once

#include "oor/model/cards/explicit/porousHydrostaticBoundary/PorousHydrostaticBoundaryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PorousHydrostaticBoundaryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PorousHydrostaticBoundaryCard& card);
    [[nodiscard]] static std::optional<PorousHydrostaticBoundaryCard> read(FieldReader& reader);
    static void write(const PorousHydrostaticBoundaryCard& card, FieldWriter& writer);
};

// All the PORHYDS cards: the PID is unique among them (IdSpace::PerCardType, category Porosity). A LEAKAGE names it;
// the card references nothing.
class PorousHydrostaticBoundaryTable final
    : public TypedTable<PorousHydrostaticBoundaryTable, PorousHydrostaticBoundaryCard>
{
public:
    static constexpr std::string_view NAME = "PorousHydrostaticBoundary";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Porosity, base::IdSpace::PerCardType};
    using Format = PorousHydrostaticBoundaryFormat;

    [[nodiscard]] static base::EntityId cardId(const PorousHydrostaticBoundaryCard& card) noexcept { return card.id; }
    std::uint32_t append(const PorousHydrostaticBoundaryCard& card, base::CardKey key);
    [[nodiscard]] PorousHydrostaticBoundaryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PorousHydrostaticBoundaryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a PORHYDS refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PorousHydrostaticBoundaryCard> m_cards;
};

} // namespace oor::model
