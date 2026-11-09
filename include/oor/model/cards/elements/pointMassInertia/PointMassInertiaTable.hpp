#pragma once

#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct PointMassInertiaFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PointMassInertiaCard& card);
    [[nodiscard]] static std::optional<PointMassInertiaCard> read(FieldReader& reader);
    static void write(const PointMassInertiaCard& card, FieldWriter& writer);
};

// All the CONM2 elements. The offset is an (N, 3) NumPy view and the inertia an (N, 6) one, without copy.
class PointMassInertiaTable final : public TypedTable<PointMassInertiaTable, PointMassInertiaCard>
{
public:
    static constexpr std::string_view NAME = "PointMassInertia";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = PointMassInertiaFormat;

    PointMassInertiaTable() : m_frameId(0) {}

    [[nodiscard]] static base::EntityId cardId(const PointMassInertiaCard& card) noexcept { return card.id; }
    std::uint32_t append(const PointMassInertiaCard& card, base::CardKey key);
    [[nodiscard]] PointMassInertiaCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const PointMassInertiaCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<double>& massColumn() noexcept { return m_mass; }
    [[nodiscard]] Column<std::array<double, 3>>& offsetColumn() noexcept { return m_offset; }
    [[nodiscard]] Column<std::array<double, 6>>& inertiaColumn() noexcept { return m_inertia; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_node;
    Column<base::EntityId> m_frameId;
    Column<double> m_mass;
    Column<std::array<double, 3>> m_offset;
    Column<std::array<double, 6>> m_inertia;
};

} // namespace oor::model
