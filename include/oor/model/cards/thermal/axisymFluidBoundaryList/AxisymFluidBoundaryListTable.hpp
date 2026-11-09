#pragma once

#include "oor/model/cards/thermal/axisymFluidBoundaryList/AxisymFluidBoundaryListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymFluidBoundaryListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymFluidBoundaryListCard& card);
    [[nodiscard]] static std::optional<AxisymFluidBoundaryListCard> read(FieldReader& reader);
    static void write(const AxisymFluidBoundaryListCard& card, FieldWriter& writer);
};

// All the BDYLIST cards. A card has no ID (IdSpace::None, category Other): several of them follow the boundary, one for
// each interval of density. A blank RHO is NaN.
class AxisymFluidBoundaryListTable final : public TypedTable<AxisymFluidBoundaryListTable, AxisymFluidBoundaryListCard>
{
public:
    static constexpr std::string_view NAME = "AxisymFluidBoundaryList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = AxisymFluidBoundaryListFormat;

    AxisymFluidBoundaryListTable();

    [[nodiscard]] static base::EntityId cardId(const AxisymFluidBoundaryListCard&) noexcept { return 0; }
    std::uint32_t append(const AxisymFluidBoundaryListCard& card, base::CardKey key);
    [[nodiscard]] AxisymFluidBoundaryListCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymFluidBoundaryListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the RINGFL cards are not typed

    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& pointsColumn() noexcept { return m_points; }

private:
    Column<double> m_density;
    VariableLengthColumn<base::EntityId> m_points;
};

} // namespace oor::model
