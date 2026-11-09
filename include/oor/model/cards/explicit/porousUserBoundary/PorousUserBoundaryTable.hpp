#pragma once

#include "oor/model/cards/explicit/porousUserBoundary/PorousUserBoundaryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PorousUserBoundaryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PorousUserBoundaryCard& card);
    [[nodiscard]] static std::optional<PorousUserBoundaryCard> read(FieldReader& reader);
    static void write(const PorousUserBoundaryCard& card, FieldWriter& writer);
};

// All the PORUDS cards: the PID is unique among them (IdSpace::PerCardType, category Porosity). A LEAKAGE names it; the
// card references nothing (its service is connected in the File Management Section).
class PorousUserBoundaryTable final : public TypedTable<PorousUserBoundaryTable, PorousUserBoundaryCard>
{
public:
    static constexpr std::string_view NAME = "PorousUserBoundary";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Porosity, base::IdSpace::PerCardType};
    using Format = PorousUserBoundaryFormat;

    [[nodiscard]] static base::EntityId cardId(const PorousUserBoundaryCard& card) noexcept { return card.id; }
    std::uint32_t append(const PorousUserBoundaryCard& card, base::CardKey key);
    [[nodiscard]] PorousUserBoundaryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PorousUserBoundaryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a PORUDS refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PorousUserBoundaryCard> m_cards;
};

} // namespace oor::model
