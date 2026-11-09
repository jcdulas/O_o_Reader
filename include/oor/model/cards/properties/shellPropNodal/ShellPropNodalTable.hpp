#pragma once

#include "oor/model/cards/properties/shellPropNodal/ShellPropNodalCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ShellPropNodalFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShellPropNodalCard& card);
    [[nodiscard]] static std::optional<ShellPropNodalCard> read(FieldReader& reader);
    static void write(const ShellPropNodalCard& card, FieldWriter& writer);
};

// All the PSHELL1 cards: properties (IdSpace::Property, shared with every other property), kept whole (CardRows) next
// to the column of their IDs; the query engine reads the PID. A PSHELL1 names a material.
class ShellPropNodalTable final : public TypedTable<ShellPropNodalTable, ShellPropNodalCard>
{
public:
    static constexpr std::string_view NAME = "ShellPropNodal";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = ShellPropNodalFormat;

    [[nodiscard]] static base::EntityId cardId(const ShellPropNodalCard& card) noexcept { return card.id; }
    std::uint32_t append(const ShellPropNodalCard& card, base::CardKey key);
    [[nodiscard]] ShellPropNodalCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ShellPropNodalCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ShellPropNodalCard> m_cards;
};

} // namespace oor::model
