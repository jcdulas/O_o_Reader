#pragma once

#include "oor/model/cards/elements/gap2/Gap2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct Gap2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const Gap2Card& card);
    [[nodiscard]] static std::optional<Gap2Card> read(FieldReader& reader);
    static void write(const Gap2Card& card, FieldWriter& writer);
};

// All the CGAP elements. The orientation vector is an (N, 3) NumPy view without copy.
class Gap2Table final : public TypedTable<Gap2Table, Gap2Card>
{
public:
    static constexpr std::string_view NAME = "Gap2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = Gap2Format;

    Gap2Table() : m_orientationNode(0), m_frameId(Gap2Card::NO_FRAME_ID) {}

    [[nodiscard]] static base::EntityId cardId(const Gap2Card& card) noexcept { return card.id; }
    std::uint32_t append(const Gap2Card& card, base::CardKey key);
    [[nodiscard]] Gap2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const Gap2Card& card);
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
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<base::EntityId> m_orientationNode;
    Column<std::array<double, 3>> m_orientation;
    Column<base::EntityId> m_frameId;
};

} // namespace oor::model
