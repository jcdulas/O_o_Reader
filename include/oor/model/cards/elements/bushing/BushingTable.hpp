#pragma once

#include "oor/model/cards/elements/bushing/BushingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct BushingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BushingCard& card);
    [[nodiscard]] static std::optional<BushingCard> read(FieldReader& reader);
    static void write(const BushingCard& card, FieldWriter& writer);
};

// All the CBUSH elements. The orientation vector and the offset are (N, 3) NumPy views without copy.
class BushingTable final : public TypedTable<BushingTable, BushingCard>
{
public:
    static constexpr std::string_view NAME = "Bushing";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = BushingFormat;

    BushingTable()
        : m_nodeB(0),
          m_orientationNode(0),
          m_frameId(BushingCard::NO_FRAME_ID),
          m_offsetFraction(BushingCard::DEFAULT_S),
          m_offsetFrameId(BushingCard::NO_OFFSET_FRAME_ID)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const BushingCard& card) noexcept { return card.id; }
    std::uint32_t append(const BushingCard& card, base::CardKey key);
    [[nodiscard]] BushingCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BushingCard& card);
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
    [[nodiscard]] Column<double>& offsetFractionColumn() noexcept { return m_offsetFraction; }
    [[nodiscard]] Column<base::EntityId>& offsetFrameIdColumn() noexcept { return m_offsetFrameId; }
    [[nodiscard]] Column<std::array<double, 3>>& offsetColumn() noexcept { return m_offset; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<base::EntityId> m_orientationNode;
    Column<std::array<double, 3>> m_orientation;
    Column<base::EntityId> m_frameId;
    Column<double> m_offsetFraction;
    Column<base::EntityId> m_offsetFrameId;
    Column<std::array<double, 3>> m_offset;
};

} // namespace oor::model
