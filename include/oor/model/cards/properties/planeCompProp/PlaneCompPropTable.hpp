#pragma once

#include "oor/model/cards/properties/planeCompProp/PlaneCompPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PlaneCompPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlaneCompPropCard& card);
    [[nodiscard]] static std::optional<PlaneCompPropCard> read(FieldReader& reader);
    static void write(const PlaneCompPropCard& card, FieldWriter& writer);
};

// All the PLCOMP cards, kept whole next to a column of their PIDs. A PLCOMP is a property (IdSpace::Property): the PID
// of a plane element finds it, and it shares its number with the PSHELL, PLPLANE, PSOLID, PCOMPG and every other
// property.
class PlaneCompPropTable final : public TypedTable<PlaneCompPropTable, PlaneCompPropCard>
{
public:
    static constexpr std::string_view NAME = "PlaneCompProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = PlaneCompPropFormat;

    [[nodiscard]] static base::EntityId cardId(const PlaneCompPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const PlaneCompPropCard& card, base::CardKey key);
    [[nodiscard]] PlaneCompPropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PlaneCompPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PlaneCompPropCard> m_cards;
};

} // namespace oor::model
