#pragma once

#include "oor/model/cards/thermal/heatVolumeSourceLoad/HeatVolumeSourceLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct HeatVolumeSourceLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatVolumeSourceLoadCard& card);
    [[nodiscard]] static std::optional<HeatVolumeSourceLoadCard> read(FieldReader& reader);
    static void write(const HeatVolumeSourceLoadCard& card, FieldWriter& writer);
};

// All the QVOL cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load), so that
// `LOAD = n` and a TLOAD1 EXCITEID find it. The element lists are one CSR column of (first, last, by) runs.
class HeatVolumeSourceLoadTable final : public TypedTable<HeatVolumeSourceLoadTable, HeatVolumeSourceLoadCard>
{
public:
    static constexpr std::string_view NAME = "HeatVolumeSourceLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = HeatVolumeSourceLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const HeatVolumeSourceLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatVolumeSourceLoadCard& card, base::CardKey key);
    [[nodiscard]] HeatVolumeSourceLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatVolumeSourceLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& heatGenerationColumn() noexcept { return m_heatGeneration; }
    [[nodiscard]] Column<base::EntityId>& controlNodeColumn() noexcept { return m_controlNode; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_heatGeneration;
    Column<base::EntityId> m_controlNode;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
