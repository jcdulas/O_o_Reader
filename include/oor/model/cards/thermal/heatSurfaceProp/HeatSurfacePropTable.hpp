#pragma once

#include "oor/model/cards/thermal/heatSurfaceProp/HeatSurfacePropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatSurfacePropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatSurfacePropCard& card);
    [[nodiscard]] static std::optional<HeatSurfacePropCard> read(FieldReader& reader);
    static void write(const HeatSurfacePropCard& card, FieldWriter& writer);
};

// All the PHBDY cards. The ID is unique among the PHBDY cards only, so it has its own space (category
// HeatBoundaryProperty, IdSpace::PerCardType) and is not a property ID of the structure. The absent values are NaN.
class HeatSurfacePropTable final : public TypedTable<HeatSurfacePropTable, HeatSurfacePropCard>
{
public:
    static constexpr std::string_view NAME = "HeatSurfaceProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::HeatBoundaryProperty, base::IdSpace::PerCardType};
    using Format = HeatSurfacePropFormat;

    HeatSurfacePropTable();

    [[nodiscard]] static base::EntityId cardId(const HeatSurfacePropCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatSurfacePropCard& card, base::CardKey key);
    [[nodiscard]] HeatSurfacePropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatSurfacePropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a PHBDY refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& areaColumn() noexcept { return m_area; }
    [[nodiscard]] Column<double>& diameter1Column() noexcept { return m_diameter1; }
    [[nodiscard]] Column<double>& diameter2Column() noexcept { return m_diameter2; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_area;
    Column<double> m_diameter1;
    Column<double> m_diameter2;
};

} // namespace oor::model
