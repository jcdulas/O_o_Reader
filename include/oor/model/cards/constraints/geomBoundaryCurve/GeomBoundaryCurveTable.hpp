#pragma once

#include "oor/model/cards/constraints/geomBoundaryCurve/GeomBoundaryCurveCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GeomBoundaryCurveFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GeomBoundaryCurveCard& card);
    [[nodiscard]] static std::optional<GeomBoundaryCurveCard> read(FieldReader& reader);
    static void write(const GeomBoundaryCurveCard& card, FieldWriter& writer);
};

// All the GMBNDC cards, in the category GeometricBoundary that a CINTC names (the BIDs of its LIST). The BID is unique
// per card type (IdSpace::PerCardType). The end points and the listed grid points are references to grid points. The
// cards are kept whole next to a column of their BIDs.
class GeomBoundaryCurveTable final : public TypedTable<GeomBoundaryCurveTable, GeomBoundaryCurveCard>
{
public:
    static constexpr std::string_view NAME = "GeomBoundaryCurve";
    static inline const CardTypeInfo INFO{NAME, CardCategory::GeometricBoundary, base::IdSpace::PerCardType};
    using Format = GeomBoundaryCurveFormat;

    [[nodiscard]] static base::EntityId cardId(const GeomBoundaryCurveCard& card) noexcept { return card.id; }
    std::uint32_t append(const GeomBoundaryCurveCard& card, base::CardKey key);
    [[nodiscard]] GeomBoundaryCurveCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const GeomBoundaryCurveCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<GeomBoundaryCurveCard> m_cards;
};

} // namespace oor::model
