#pragma once

#include "oor/model/cards/explicit/hydrodynamicYield/HydrodynamicYieldCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HydrodynamicYieldFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HydrodynamicYieldCard& card);
    [[nodiscard]] static std::optional<HydrodynamicYieldCard> read(FieldReader& reader);
    static void write(const HydrodynamicYieldCard& card, FieldWriter& writer);
};

// All the YLDHY cards. The ID is the number of a yield model, unique among the cards of its category
// (IdSpace::PerCardType: the other YLDxxx cards share the category YieldModel, and a check reports an ID two of them
// have). A MATDEUL names the model. The cards are kept whole (CardRows) next to their ID: they are few, and the query
// engine reads the ID.
class HydrodynamicYieldTable final : public TypedTable<HydrodynamicYieldTable, HydrodynamicYieldCard>
{
public:
    static constexpr std::string_view NAME = "HydrodynamicYield";
    static inline const CardTypeInfo INFO{NAME, CardCategory::YieldModel, base::IdSpace::PerCardType};
    using Format = HydrodynamicYieldFormat;

    [[nodiscard]] static base::EntityId cardId(const HydrodynamicYieldCard& card) noexcept { return card.id; }
    std::uint32_t append(const HydrodynamicYieldCard& card, base::CardKey key);
    [[nodiscard]] HydrodynamicYieldCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const HydrodynamicYieldCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a YLDHY refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<HydrodynamicYieldCard> m_cards;
};

} // namespace oor::model
