#pragma once

#include "oor/model/cards/explicit/fluidFilledContainer/FluidFilledContainerCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FluidFilledContainerFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FluidFilledContainerCard& card);
    [[nodiscard]] static std::optional<FluidFilledContainerCard> read(FieldReader& reader);
    static void write(const FluidFilledContainerCard& card, FieldWriter& writer);
};

// All the FFCONTR cards: the FFID is unique among them (IdSpace::PerCardType, CardCategory::Other: no card of the
// library names an FFCONTR; a SURFOUT of the solver reports on it). The surface is a ContactSurface, the two tables
// TABLED1.
class FluidFilledContainerTable final : public TypedTable<FluidFilledContainerTable, FluidFilledContainerCard>
{
public:
    static constexpr std::string_view NAME = "FluidFilledContainer";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = FluidFilledContainerFormat;

    [[nodiscard]] static base::EntityId cardId(const FluidFilledContainerCard& card) noexcept { return card.id; }
    std::uint32_t append(const FluidFilledContainerCard& card, base::CardKey key);
    [[nodiscard]] FluidFilledContainerCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FluidFilledContainerCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FluidFilledContainerCard> m_cards;
};

} // namespace oor::model
