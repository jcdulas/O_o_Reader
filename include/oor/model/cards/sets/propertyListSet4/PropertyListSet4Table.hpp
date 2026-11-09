#pragma once

#include "oor/model/cards/sets/propertyListSet4/PropertyListSet4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PropertyListSet4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PropertyListSet4Card& card);
    [[nodiscard]] static std::optional<PropertyListSet4Card> read(FieldReader& reader);
    static void write(const PropertyListSet4Card& card, FieldWriter& writer);
};

// All the SET4 cards, in the category Set. The ID is a set ID shared by the cards that have it (IdSpace::Group), so the
// ReferenceTarget::Set of an FTGDEF finds it as it finds a SET1 or a SET3. The properties of the list are references: a
// single ID is a ReferenceTarget::Property, the two ends of a THRU range are ReferenceTarget::PropertyInRange (a
// property of the range may not exist; the ends are renumbered with the properties). The cards are kept whole next to a
// column of their IDs.
class PropertyListSet4Table final : public TypedTable<PropertyListSet4Table, PropertyListSet4Card>
{
public:
    static constexpr std::string_view NAME = "PropertyListSet4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::Group};
    using Format = PropertyListSet4Format;

    [[nodiscard]] static base::EntityId cardId(const PropertyListSet4Card& card) noexcept { return card.id; }
    std::uint32_t append(const PropertyListSet4Card& card, base::CardKey key);
    [[nodiscard]] PropertyListSet4Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PropertyListSet4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PropertyListSet4Card> m_cards;
};

} // namespace oor::model
