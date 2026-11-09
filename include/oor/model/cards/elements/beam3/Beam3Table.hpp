#pragma once

#include "oor/model/cards/elements/beam3/Beam3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct Beam3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const Beam3Card& card);
    [[nodiscard]] static std::optional<Beam3Card> read(FieldReader& reader);
    static void write(const Beam3Card& card, FieldWriter& writer);
};

// All the CBEAM3 elements. The orientation vector, the offsets and the twists are (N, 3) NumPy views without copy.
class Beam3Table final : public TypedTable<Beam3Table, Beam3Card>
{
public:
    static constexpr std::string_view NAME = "Beam3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = Beam3Format;

    [[nodiscard]] static base::EntityId cardId(const Beam3Card& card) noexcept { return card.id; }
    std::uint32_t append(const Beam3Card& card, base::CardKey key);
    [[nodiscard]] Beam3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const Beam3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<base::EntityId>& nodeCColumn() noexcept { return m_nodeC; }
    [[nodiscard]] Column<base::EntityId>& orientationNodeColumn() noexcept { return m_orientationNode; }
    [[nodiscard]] Column<std::array<double, 3>>& orientationColumn() noexcept { return m_orientation; }
    [[nodiscard]] Column<std::array<double, 3>>& offsetAColumn() noexcept { return m_offsetA; }
    [[nodiscard]] Column<std::array<double, 3>>& offsetBColumn() noexcept { return m_offsetB; }
    [[nodiscard]] Column<std::array<double, 3>>& offsetCColumn() noexcept { return m_offsetC; }
    [[nodiscard]] Column<std::array<double, 3>>& twistColumn() noexcept { return m_twist; }
    [[nodiscard]] Column<base::EntityId>& warpingPointAColumn() noexcept { return m_warpingPointA; }
    [[nodiscard]] Column<base::EntityId>& warpingPointBColumn() noexcept { return m_warpingPointB; }
    [[nodiscard]] Column<base::EntityId>& warpingPointCColumn() noexcept { return m_warpingPointC; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<base::EntityId> m_nodeC;
    Column<base::EntityId> m_orientationNode;
    Column<std::array<double, 3>> m_orientation;
    Column<std::array<double, 3>> m_offsetA;
    Column<std::array<double, 3>> m_offsetB;
    Column<std::array<double, 3>> m_offsetC;
    Column<std::array<double, 3>> m_twist;
    Column<base::EntityId> m_warpingPointA;
    Column<base::EntityId> m_warpingPointB;
    Column<base::EntityId> m_warpingPointC;
};

} // namespace oor::model
