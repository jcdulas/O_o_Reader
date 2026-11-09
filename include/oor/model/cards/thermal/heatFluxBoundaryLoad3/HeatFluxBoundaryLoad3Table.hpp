#pragma once

#include "oor/model/cards/thermal/heatFluxBoundaryLoad3/HeatFluxBoundaryLoad3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct HeatFluxBoundaryLoad3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatFluxBoundaryLoad3Card& card);
    [[nodiscard]] static std::optional<HeatFluxBoundaryLoad3Card> read(FieldReader& reader);
    static void write(const HeatFluxBoundaryLoad3Card& card, FieldWriter& writer);
};

// All the QBDY3 cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load). The element lists
// are one CSR column of (first, last, by) runs.
class HeatFluxBoundaryLoad3Table final : public TypedTable<HeatFluxBoundaryLoad3Table, HeatFluxBoundaryLoad3Card>
{
public:
    static constexpr std::string_view NAME = "HeatFluxBoundaryLoad3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = HeatFluxBoundaryLoad3Format;

    [[nodiscard]] static base::EntityId cardId(const HeatFluxBoundaryLoad3Card& card) noexcept { return card.id; }
    std::uint32_t append(const HeatFluxBoundaryLoad3Card& card, base::CardKey key);
    [[nodiscard]] HeatFluxBoundaryLoad3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatFluxBoundaryLoad3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& heatFluxColumn() noexcept { return m_heatFlux; }
    [[nodiscard]] Column<base::EntityId>& controlNodeColumn() noexcept { return m_controlNode; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_heatFlux;
    Column<base::EntityId> m_controlNode;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
