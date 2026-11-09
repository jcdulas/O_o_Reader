#pragma once

#include "oor/model/cards/dynamics/heatShellNodalLoad/HeatShellNodalLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatShellNodalLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatShellNodalLoadCard& card);
    [[nodiscard]] static std::optional<HeatShellNodalLoadCard> read(FieldReader& reader);
    static void write(const HeatShellNodalLoadCard& card, FieldWriter& writer);
};

// All the SLOADN1 cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load), like the SLOAD
// it parallels. The loads are three lists in CSR form (the grid points, the components and the powers); each grid point
// is a Node reference.
class HeatShellNodalLoadTable final : public TypedTable<HeatShellNodalLoadTable, HeatShellNodalLoadCard>
{
public:
    static constexpr std::string_view NAME = "HeatShellNodalLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = HeatShellNodalLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const HeatShellNodalLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatShellNodalLoadCard& card, base::CardKey key);
    [[nodiscard]] HeatShellNodalLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatShellNodalLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] VariableLengthColumn<double>& powerColumn() noexcept { return m_power; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<std::int32_t> m_components;
    VariableLengthColumn<double> m_power;
};

} // namespace oor::model
