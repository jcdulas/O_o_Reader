#pragma once

#include "oor/model/cards/properties/shearPanelPropNonlinear/ShearPanelPropNonlinearCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ShearPanelPropNonlinearFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShearPanelPropNonlinearCard& card);
    [[nodiscard]] static std::optional<ShearPanelPropNonlinearCard> read(FieldReader& reader);
    static void write(const ShearPanelPropNonlinearCard& card, FieldWriter& writer);
};

// All the PSHEARN cards, kept whole next to a column of their PIDs. The ID space is the card type's own: the ID is the
// one of the PSHEAR that the card extends, so it is also a reference to a property (a renumbering of the properties
// follows into it).
class ShearPanelPropNonlinearTable final : public TypedTable<ShearPanelPropNonlinearTable, ShearPanelPropNonlinearCard>
{
public:
    static constexpr std::string_view NAME = "ShearPanelPropNonlinear";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::PerCardType};
    using Format = ShearPanelPropNonlinearFormat;

    [[nodiscard]] static base::EntityId cardId(const ShearPanelPropNonlinearCard& card) noexcept { return card.id; }
    std::uint32_t append(const ShearPanelPropNonlinearCard& card, base::CardKey key);
    [[nodiscard]] ShearPanelPropNonlinearCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ShearPanelPropNonlinearCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ShearPanelPropNonlinearCard> m_cards;
};

} // namespace oor::model
