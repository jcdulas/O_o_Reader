#pragma once

#include "oor/model/cards/loads/imperfectionGeometry/ImperfectionGeometryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ImperfectionGeometryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ImperfectionGeometryCard& card);
    [[nodiscard]] static std::optional<ImperfectionGeometryCard> read(FieldReader& reader);
    static void write(const ImperfectionGeometryCard& card, FieldWriter& writer);
};

// All the IMPGEOM cards, in the category ImperfectionGeometry: the ID is unique per card type (IdSpace::PerCardType)
// and is what the IMPFIDi of an IMPCASE names (ReferenceTarget::ImperfectionGeometry). The cards are kept whole next to
// their ID. The sets of the card and of its groups are Set references; a zero names nothing.
class ImperfectionGeometryTable final : public TypedTable<ImperfectionGeometryTable, ImperfectionGeometryCard>
{
public:
    static constexpr std::string_view NAME = "ImperfectionGeometry";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ImperfectionGeometry, base::IdSpace::PerCardType};
    using Format = ImperfectionGeometryFormat;

    [[nodiscard]] static base::EntityId cardId(const ImperfectionGeometryCard& card) noexcept { return card.id; }
    std::uint32_t append(const ImperfectionGeometryCard& card, base::CardKey key);
    [[nodiscard]] ImperfectionGeometryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ImperfectionGeometryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ImperfectionGeometryCard> m_cards;
};

} // namespace oor::model
