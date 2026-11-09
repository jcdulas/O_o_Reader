#pragma once

#include "oor/model/cards/constraints/multiPointConstrValue/MultiPointConstrValueCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct MultiPointConstrValueFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MultiPointConstrValueCard& card);
    [[nodiscard]] static std::optional<MultiPointConstrValueCard> read(FieldReader& reader);
    static void write(const MultiPointConstrValueCard& card, FieldWriter& writer);
};

// All the MPCD cards. The ID is a load set ID shared by many cards (IdSpace::Group): an MPCD is selected the way a load
// is (the LOAD command, as for the SPCD), not by the MPC command, so the category is Load. The two equations of a card
// are kept as three columns of two values. The points are references to grid or scalar points (a scalar point that no
// SPOINT defines is allowed).
class MultiPointConstrValueTable final : public TypedTable<MultiPointConstrValueTable, MultiPointConstrValueCard>
{
public:
    static constexpr std::string_view NAME = "MultiPointConstrValue";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = MultiPointConstrValueFormat;

    [[nodiscard]] static base::EntityId cardId(const MultiPointConstrValueCard& card) noexcept { return card.id; }
    std::uint32_t append(const MultiPointConstrValueCard& card, base::CardKey key);
    [[nodiscard]] MultiPointConstrValueCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const MultiPointConstrValueCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, 2>>& pointsColumn() noexcept { return m_points; }
    [[nodiscard]] Column<std::array<std::int32_t, 2>>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] Column<std::array<double, 2>>& valuesColumn() noexcept { return m_values; }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, 2>> m_points;
    Column<std::array<std::int32_t, 2>> m_components;
    Column<std::array<double, 2>> m_values;
};

} // namespace oor::model
