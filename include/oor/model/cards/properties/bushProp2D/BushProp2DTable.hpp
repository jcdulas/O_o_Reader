#pragma once

#include "oor/model/cards/properties/bushProp2D/BushProp2DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BushProp2DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BushProp2DCard& card);
    [[nodiscard]] static std::optional<BushProp2DCard> read(FieldReader& reader);
    static void write(const BushProp2DCard& card, FieldWriter& writer);
};

// All the PBUSH2D properties, each kept whole (the nominal values and, at most, one of the two options): the ID is in
// the space every property type shares (IdSpace::Property), so the PID of a CBUSH2D finds it. A PBUSH2D names no other
// card.
class BushProp2DTable final : public TypedTable<BushProp2DTable, BushProp2DCard>
{
public:
    static constexpr std::string_view NAME = "BushProp2D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BushProp2DFormat;

    [[nodiscard]] static base::EntityId cardId(const BushProp2DCard& card) noexcept { return card.id; }
    std::uint32_t append(const BushProp2DCard& card, base::CardKey key);
    [[nodiscard]] BushProp2DCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const BushProp2DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a PBUSH2D refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<BushProp2DCard> m_cards;
};

} // namespace oor::model
