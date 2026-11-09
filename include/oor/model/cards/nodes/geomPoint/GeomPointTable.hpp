#pragma once

#include "oor/model/cards/nodes/geomPoint/GeomPointCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct GeomPointFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GeomPointCard& card);
    [[nodiscard]] static std::optional<GeomPointCard> read(FieldReader& reader);
    static void write(const GeomPointCard& card, FieldWriter& writer);
};

// All the POINT cards of a model. The locations are one column of (x, y, z) triples (an (N, 3) NumPy view).
class GeomPointTable final : public TypedTable<GeomPointTable, GeomPointCard>
{
public:
    static constexpr std::string_view NAME = "GeomPoint";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Point, base::IdSpace::Node};
    using Format = GeomPointFormat;

    [[nodiscard]] static base::EntityId cardId(const GeomPointCard& card) noexcept { return card.id; }
    std::uint32_t append(const GeomPointCard& card, base::CardKey key);
    [[nodiscard]] GeomPointCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const GeomPointCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& positionFrameIdColumn() noexcept { return m_positionFrameId; }
    [[nodiscard]] Column<std::array<double, 3>>& xyzColumn() noexcept { return m_xyz; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_positionFrameId;
    Column<std::array<double, 3>> m_xyz;
};

} // namespace oor::model
