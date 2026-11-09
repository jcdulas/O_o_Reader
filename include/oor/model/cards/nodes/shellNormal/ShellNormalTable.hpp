#pragma once

#include "oor/model/cards/nodes/shellNormal/ShellNormalCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ShellNormalFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShellNormalCard& card);
    [[nodiscard]] static std::optional<ShellNormalCard> read(FieldReader& reader);
    static void write(const ShellNormalCard& card, FieldWriter& writer);
};

// All the SNORM cards of a model. The ID is the grid point, unique per card type (IdSpace::PerCardType: one normal for
// a grid point, and the same number as a GRID or another card is the same point). The normals are one column of
// (n1, n2, n3) triples (an (N, 3) NumPy view). The GID names a grid point and the CID a coordinate system.
class ShellNormalTable final : public TypedTable<ShellNormalTable, ShellNormalCard>
{
public:
    static constexpr std::string_view NAME = "ShellNormal";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ShellNormalFormat;

    [[nodiscard]] static base::EntityId cardId(const ShellNormalCard& card) noexcept { return card.nodeId; }
    std::uint32_t append(const ShellNormalCard& card, base::CardKey key);
    [[nodiscard]] ShellNormalCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ShellNormalCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_nodeId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_nodeId.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& nodeIdColumn() noexcept { return m_nodeId; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<std::array<double, 3>>& normalColumn() noexcept { return m_normal; }

private:
    Column<base::EntityId> m_nodeId;
    Column<base::EntityId> m_frameId;
    Column<std::array<double, 3>> m_normal;
};

} // namespace oor::model
