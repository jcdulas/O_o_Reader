#pragma once

#include "oor/model/cards/elements/solidPropNonlinear/SolidPropNonlinearCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SolidPropNonlinearFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SolidPropNonlinearCard& card);
    [[nodiscard]] static std::optional<SolidPropNonlinearCard> read(FieldReader& reader);
    static void write(const SolidPropNonlinearCard& card, FieldWriter& writer);
};

// All the PSLDN1 cards, kept whole next to a column of their PIDs. The ID space is the card type's own: the ID is the
// one of the PSOLID that the card extends, so it is also a reference to a property (a renumbering of the properties
// follows into it).
class SolidPropNonlinearTable final : public TypedTable<SolidPropNonlinearTable, SolidPropNonlinearCard>
{
public:
    static constexpr std::string_view NAME = "SolidPropNonlinear";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::PerCardType};
    using Format = SolidPropNonlinearFormat;

    [[nodiscard]] static base::EntityId cardId(const SolidPropNonlinearCard& card) noexcept { return card.id; }
    std::uint32_t append(const SolidPropNonlinearCard& card, base::CardKey key);
    [[nodiscard]] SolidPropNonlinearCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SolidPropNonlinearCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<SolidPropNonlinearCard> m_cards;
};

} // namespace oor::model
