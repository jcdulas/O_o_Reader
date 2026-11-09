#pragma once

#include "oor/model/cards/loads/pointsPressureLoad/PointsPressureLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct PointsPressureLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PointsPressureLoadCard& card);
    [[nodiscard]] static std::optional<PointsPressureLoadCard> read(FieldReader& reader);
    static void write(const PointsPressureLoadCard& card, FieldWriter& writer);
};

// All the PLOAD cards. The ID is a load set ID shared by many cards (IdSpace::Group).
class PointsPressureLoadTable final : public TypedTable<PointsPressureLoadTable, PointsPressureLoadCard>
{
public:
    static constexpr std::string_view NAME = "PointsPressureLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = PointsPressureLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const PointsPressureLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const PointsPressureLoadCard& card, base::CardKey key);
    [[nodiscard]] PointsPressureLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const PointsPressureLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& pressureColumn() noexcept { return m_pressure; }
    [[nodiscard]] Column<std::array<base::EntityId, 4>>& nodesColumn() noexcept { return m_nodes; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_pressure;
    Column<std::array<base::EntityId, 4>> m_nodes;
};

} // namespace oor::model
