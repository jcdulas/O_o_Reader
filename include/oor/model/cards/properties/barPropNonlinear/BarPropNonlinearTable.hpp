#pragma once

#include "oor/model/cards/properties/barPropNonlinear/BarPropNonlinearCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BarPropNonlinearFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BarPropNonlinearCard& card);
    [[nodiscard]] static std::optional<BarPropNonlinearCard> read(FieldReader& reader);
    static void write(const BarPropNonlinearCard& card, FieldWriter& writer);
};

// All the PBARN1 cards, kept whole next to a column of their PIDs. The ID space is the card type's own: the ID is the
// one of the PBAR or PBARL that the card extends, so it is also a reference to a property (a renumbering of the
// properties follows into it).
class BarPropNonlinearTable final : public TypedTable<BarPropNonlinearTable, BarPropNonlinearCard>
{
public:
    static constexpr std::string_view NAME = "BarPropNonlinear";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::PerCardType};
    using Format = BarPropNonlinearFormat;

    [[nodiscard]] static base::EntityId cardId(const BarPropNonlinearCard& card) noexcept { return card.id; }
    std::uint32_t append(const BarPropNonlinearCard& card, base::CardKey key);
    [[nodiscard]] BarPropNonlinearCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const BarPropNonlinearCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<BarPropNonlinearCard> m_cards;
};

} // namespace oor::model
