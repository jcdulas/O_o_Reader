#pragma once

#include "oor/model/cards/dynamics/dynamicLoadScale/DynamicLoadScaleCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct DynamicLoadScaleFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DynamicLoadScaleCard& card);
    [[nodiscard]] static std::optional<DynamicLoadScaleCard> read(FieldReader& reader);
    static void write(const DynamicLoadScaleCard& card, FieldWriter& writer);
};

// All the DAREA cards. The ID is a set ID shared by many cards (IdSpace::Group), in the same ID space as the dynamic
// loads that refer to it. The two groups of a card are kept as three columns of two values.
class DynamicLoadScaleTable final : public TypedTable<DynamicLoadScaleTable, DynamicLoadScaleCard>
{
public:
    static constexpr std::string_view NAME = "DynamicLoadScale";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = DynamicLoadScaleFormat;

    [[nodiscard]] static base::EntityId cardId(const DynamicLoadScaleCard& card) noexcept { return card.id; }
    std::uint32_t append(const DynamicLoadScaleCard& card, base::CardKey key);
    [[nodiscard]] DynamicLoadScaleCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DynamicLoadScaleCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, 2>>& pointsColumn() noexcept { return m_points; }
    [[nodiscard]] Column<std::array<std::int32_t, 2>>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] Column<std::array<double, 2>>& scaleFactorsColumn() noexcept { return m_scaleFactors; }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, 2>> m_points;
    Column<std::array<std::int32_t, 2>> m_components;
    Column<std::array<double, 2>> m_scaleFactors;
};

} // namespace oor::model
