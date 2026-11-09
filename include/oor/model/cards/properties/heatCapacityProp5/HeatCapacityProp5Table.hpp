#pragma once

#include "oor/model/cards/properties/heatCapacityProp5/HeatCapacityProp5Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatCapacityProp5Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatCapacityProp5Card& card);
    [[nodiscard]] static std::optional<HeatCapacityProp5Card> read(FieldReader& reader);
    static void write(const HeatCapacityProp5Card& card, FieldWriter& writer);
};

class HeatCapacityProp5Table final : public TypedTable<HeatCapacityProp5Table, HeatCapacityProp5Card>
{
public:
    static constexpr std::string_view NAME = "HeatCapacityProp5";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = HeatCapacityProp5Format;

    [[nodiscard]] static base::EntityId cardId(const HeatCapacityProp5Card& card) noexcept { return card.id; }
    std::uint32_t append(const HeatCapacityProp5Card& card, base::CardKey key);
    [[nodiscard]] HeatCapacityProp5Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatCapacityProp5Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<double>& heatCapacityColumn() noexcept { return m_heatCapacity; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<double> m_heatCapacity;
};

} // namespace oor::model
