#pragma once

#include "oor/model/cards/thermal/axisymFluidFreeSurface/AxisymFluidFreeSurfaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymFluidFreeSurfaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymFluidFreeSurfaceCard& card);
    [[nodiscard]] static std::optional<AxisymFluidFreeSurfaceCard> read(FieldReader& reader);
    static void write(const AxisymFluidFreeSurfaceCard& card, FieldWriter& writer);
};

// All the FSLIST cards. A card has no ID (IdSpace::None, category Other): several of them follow the free surface, one
// for each interval of density. A blank RHO is NaN.
class AxisymFluidFreeSurfaceTable final : public TypedTable<AxisymFluidFreeSurfaceTable, AxisymFluidFreeSurfaceCard>
{
public:
    static constexpr std::string_view NAME = "AxisymFluidFreeSurface";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = AxisymFluidFreeSurfaceFormat;

    AxisymFluidFreeSurfaceTable();

    [[nodiscard]] static base::EntityId cardId(const AxisymFluidFreeSurfaceCard&) noexcept { return 0; }
    std::uint32_t append(const AxisymFluidFreeSurfaceCard& card, base::CardKey key);
    [[nodiscard]] AxisymFluidFreeSurfaceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymFluidFreeSurfaceCard& card);
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
