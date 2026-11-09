#pragma once

#include "oor/model/cards/materials/shapeMemoryMat/ShapeMemoryMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ShapeMemoryMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShapeMemoryMatCard& card);
    [[nodiscard]] static std::optional<ShapeMemoryMatCard> read(FieldReader& reader);
    static void write(const ShapeMemoryMatCard& card, FieldWriter& writer);
};

// All the MATSMA cards. The ID is a material ID, in the space every material type shares (IdSpace::Material): the
// properties of the elements name it. The cards are kept whole (CardRows: the values are many and positional); the
// query engine reads the ID only.
class ShapeMemoryMatTable final : public TypedTable<ShapeMemoryMatTable, ShapeMemoryMatCard>
{
public:
    static constexpr std::string_view NAME = "ShapeMemoryMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = ShapeMemoryMatFormat;

    [[nodiscard]] static base::EntityId cardId(const ShapeMemoryMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const ShapeMemoryMatCard& card, base::CardKey key);
    [[nodiscard]] ShapeMemoryMatCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ShapeMemoryMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a MATSMA refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ShapeMemoryMatCard> m_cards;
};

} // namespace oor::model
