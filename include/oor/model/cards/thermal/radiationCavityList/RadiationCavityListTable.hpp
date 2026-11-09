#pragma once

#include "oor/model/cards/thermal/radiationCavityList/RadiationCavityListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct RadiationCavityListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RadiationCavityListCard& card);
    [[nodiscard]] static std::optional<RadiationCavityListCard> read(FieldReader& reader);
    static void write(const RadiationCavityListCard& card, FieldWriter& writer);
};

// All the RADLST cards. The ID is the ICAVITY of the cavity (category RadiationCavity, IdSpace::PerCardType: one RADLST
// per cavity). The element list is one CSR column of (first, last, by) runs; a blank MTXTYP is 0 in the column.
class RadiationCavityListTable final : public TypedTable<RadiationCavityListTable, RadiationCavityListCard>
{
public:
    static constexpr std::string_view NAME = "RadiationCavityList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::RadiationCavity, base::IdSpace::PerCardType};
    using Format = RadiationCavityListFormat;

    // The value of the MTXTYP column for a blank field (0 is not a matrix type).
    static constexpr std::int32_t NO_MATRIX_TYPE = 0;

    [[nodiscard]] static base::EntityId cardId(const RadiationCavityListCard& card) noexcept { return card.cavityId; }
    std::uint32_t append(const RadiationCavityListCard& card, base::CardKey key);
    [[nodiscard]] RadiationCavityListCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RadiationCavityListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_cavityId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_cavityId.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& cavityIdColumn() noexcept { return m_cavityId; }
    [[nodiscard]] Column<std::int32_t>& matrixKindColumn() noexcept { return m_matrixKind; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_cavityId;
    Column<std::int32_t> m_matrixKind;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
