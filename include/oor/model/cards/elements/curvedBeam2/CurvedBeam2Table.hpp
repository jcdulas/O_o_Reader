#pragma once

#include "oor/model/cards/elements/curvedBeam2/CurvedBeam2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct CurvedBeam2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CurvedBeam2Card& card);
    [[nodiscard]] static std::optional<CurvedBeam2Card> read(FieldReader& reader);
    static void write(const CurvedBeam2Card& card, FieldWriter& writer);
};

// All the CBEND elements. The orientation vector is an (N, 3) NumPy view without copy.
class CurvedBeam2Table final : public TypedTable<CurvedBeam2Table, CurvedBeam2Card>
{
public:
    static constexpr std::string_view NAME = "CurvedBeam2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = CurvedBeam2Format;

    CurvedBeam2Table() : m_orientationNode(0), m_geometryOption(1) {}

    [[nodiscard]] static base::EntityId cardId(const CurvedBeam2Card& card) noexcept { return card.id; }
    std::uint32_t append(const CurvedBeam2Card& card, base::CardKey key);
    [[nodiscard]] CurvedBeam2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const CurvedBeam2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<base::EntityId>& orientationNodeColumn() noexcept { return m_orientationNode; }
    [[nodiscard]] Column<std::array<double, 3>>& orientationColumn() noexcept { return m_orientation; }
    [[nodiscard]] Column<std::int32_t>& geometryOptionColumn() noexcept { return m_geometryOption; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<base::EntityId> m_orientationNode;
    Column<std::array<double, 3>> m_orientation;
    Column<std::int32_t> m_geometryOption;
};

} // namespace oor::model
