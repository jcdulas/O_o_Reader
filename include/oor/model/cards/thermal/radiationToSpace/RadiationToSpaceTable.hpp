#pragma once

#include "oor/model/cards/thermal/radiationToSpace/RadiationToSpaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct RadiationToSpaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RadiationToSpaceCard& card);
    [[nodiscard]] static std::optional<RadiationToSpaceCard> read(FieldReader& reader);
    static void write(const RadiationToSpaceCard& card, FieldWriter& writer);
};

// All the RADBC cards. A RADBC has no ID (IdSpace::None, category Other: several cards may apply to the same ambient
// point). The element lists are one CSR column of (first, last, by) runs.
class RadiationToSpaceTable final : public TypedTable<RadiationToSpaceTable, RadiationToSpaceCard>
{
public:
    static constexpr std::string_view NAME = "RadiationToSpace";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = RadiationToSpaceFormat;

    [[nodiscard]] static base::EntityId cardId(const RadiationToSpaceCard&) noexcept { return 0; }
    std::uint32_t append(const RadiationToSpaceCard& card, base::CardKey key);
    [[nodiscard]] RadiationToSpaceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RadiationToSpaceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& ambientNodeColumn() noexcept { return m_ambientNode; }
    [[nodiscard]] Column<double>& ambientViewFactorColumn() noexcept { return m_ambientViewFactor; }
    [[nodiscard]] Column<base::EntityId>& controlNodeColumn() noexcept { return m_controlNode; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_ambientNode;
    Column<double> m_ambientViewFactor;
    Column<base::EntityId> m_controlNode;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
