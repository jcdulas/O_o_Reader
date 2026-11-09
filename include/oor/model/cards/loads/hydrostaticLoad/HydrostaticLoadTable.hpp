#pragma once

#include "oor/model/cards/loads/hydrostaticLoad/HydrostaticLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct HydrostaticLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HydrostaticLoadCard& card);
    [[nodiscard]] static std::optional<HydrostaticLoadCard> read(FieldReader& reader);
    static void write(const HydrostaticLoadCard& card, FieldWriter& writer);
};

// All the HYDROS cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load). SETE, SETG and
// CNTL are Set references (a blank one is not visited); CID/GRD is a Coordinate reference when it is above zero and a
// Node reference (of its absolute value, the sign staying on the card) when it is below zero.
class HydrostaticLoadTable final : public TypedTable<HydrostaticLoadTable, HydrostaticLoadCard>
{
public:
    static constexpr std::string_view NAME = "HydrostaticLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = HydrostaticLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const HydrostaticLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const HydrostaticLoadCard& card, base::CardKey key);
    [[nodiscard]] HydrostaticLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HydrostaticLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& nominalPressureColumn() noexcept { return m_nominalPressure; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& gravityColumn() noexcept { return m_gravity; }
    [[nodiscard]] Column<base::EntityId>& elementSetIdColumn() noexcept { return m_elementSetId; }
    [[nodiscard]] Column<base::EntityId>& nodeSetIdColumn() noexcept { return m_nodeSetId; }
    [[nodiscard]] Column<std::int32_t>& punchPressureLoadsColumn() noexcept { return m_punchPressureLoads; }
    [[nodiscard]] Column<std::int32_t>& overrideOptionColumn() noexcept { return m_overrideOption; }
    [[nodiscard]] Column<base::EntityId>& frameOrNodeColumn() noexcept { return m_frameOrNode; }
    [[nodiscard]] Column<double>& baseHeightColumn() noexcept { return m_baseHeight; }
    [[nodiscard]] Column<double>& heightAboveColumn() noexcept { return m_heightAbove; }
    [[nodiscard]] Column<base::EntityId>& controlNodeSetIdColumn() noexcept { return m_controlNodeSetId; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_nominalPressure;
    Column<double> m_density;
    Column<double> m_gravity{1.0}; // the default of G
    Column<base::EntityId> m_elementSetId;
    Column<base::EntityId> m_nodeSetId;
    Column<std::int32_t> m_punchPressureLoads;
    Column<std::int32_t> m_overrideOption;
    Column<base::EntityId> m_frameOrNode;
    Column<double> m_baseHeight;
    Column<double> m_heightAbove;
    Column<base::EntityId> m_controlNodeSetId;
};

} // namespace oor::model
