#pragma once

#include "oor/model/cards/solution_control/rotorAxisym/RotorAxisymCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RotorAxisymFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RotorAxisymCard& card);
    [[nodiscard]] static std::optional<RotorAxisymCard> read(FieldReader& reader);
    static void write(const RotorAxisymCard& card, FieldWriter& writer);
};

// All the ROTORAX cards. The ID is the rotor ID (category Rotor, IdSpace::Group: several cards make one rotor). What
// the IDs name depends on LTYPE: elements (Element, and ElementInRange for the ends of a range), properties (Property
// and PropertyInRange) or grid points (Node and NodeInRange). The cards are kept whole next to the columns the query
// engine reads: the rotor ID and the kind of list.
class RotorAxisymTable final : public TypedTable<RotorAxisymTable, RotorAxisymCard>
{
public:
    static constexpr std::string_view NAME = "RotorAxisym";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Rotor, base::IdSpace::Group};
    using Format = RotorAxisymFormat;

    [[nodiscard]] static base::EntityId cardId(const RotorAxisymCard& card) noexcept { return card.id; }
    std::uint32_t append(const RotorAxisymCard& card, base::CardKey key);
    [[nodiscard]] RotorAxisymCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RotorAxisymCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& listTypeColumn() noexcept { return m_listType; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_listType;
    CardRows<RotorAxisymCard> m_cards;
};

} // namespace oor::model
