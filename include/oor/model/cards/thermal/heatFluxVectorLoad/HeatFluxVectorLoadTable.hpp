#pragma once

#include "oor/model/cards/thermal/heatFluxVectorLoad/HeatFluxVectorLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct HeatFluxVectorLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatFluxVectorLoadCard& card);
    [[nodiscard]] static std::optional<HeatFluxVectorLoadCard> read(FieldReader& reader);
    static void write(const HeatFluxVectorLoadCard& card, FieldWriter& writer);
};

// All the QVECT cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load). A blank Q0 or
// TSOUR is NaN and a blank CE is NO_DIRECTION_FRAME in the columns; the element lists are one CSR column of (first,
// last, by) runs.
class HeatFluxVectorLoadTable final : public TypedTable<HeatFluxVectorLoadTable, HeatFluxVectorLoadCard>
{
public:
    static constexpr std::string_view NAME = "HeatFluxVectorLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = HeatFluxVectorLoadFormat;

    // The value of the CE column for a blank field (CE -1, 0 and the system IDs are all values).
    static constexpr std::int32_t NO_DIRECTION_FRAME = -2;

    HeatFluxVectorLoadTable();

    [[nodiscard]] static base::EntityId cardId(const HeatFluxVectorLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatFluxVectorLoadCard& card, base::CardKey key);
    [[nodiscard]] HeatFluxVectorLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatFluxVectorLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& heatFluxColumn() noexcept { return m_heatFlux; }
    [[nodiscard]] Column<double>& sourceTemperatureColumn() noexcept { return m_sourceTemperature; }
    [[nodiscard]] Column<std::int32_t>& directionFrameIdColumn() noexcept { return m_directionFrameId; }
    [[nodiscard]] Column<std::array<double, 3>>& directionColumn() noexcept { return m_direction; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& tableIdColumn() noexcept { return m_tableId; }
    [[nodiscard]] Column<base::EntityId>& controlNodeColumn() noexcept { return m_controlNode; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_heatFlux;
    Column<double> m_sourceTemperature;
    Column<std::int32_t> m_directionFrameId;
    Column<std::array<double, 3>> m_direction;
    Column<std::array<base::EntityId, 3>> m_tableId;
    Column<base::EntityId> m_controlNode;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
