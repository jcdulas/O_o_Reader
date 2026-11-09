#pragma once

#include "oor/model/cards/optimisation/designNodeMove/DesignNodeMoveCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct DesignNodeMoveFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignNodeMoveCard& card);
    [[nodiscard]] static std::optional<DesignNodeMoveCard> read(FieldReader& reader);
    static void write(const DesignNodeMoveCard& card, FieldWriter& writer);
};

// All the DVGRID cards (no ID: IdSpace::None). The DVID is a design variable, the GID a grid point (or a POINT) and the
// CID a coordinate system.
class DesignNodeMoveTable final : public TypedTable<DesignNodeMoveTable, DesignNodeMoveCard>
{
public:
    static constexpr std::string_view NAME = "DesignNodeMove";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = DesignNodeMoveFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignNodeMoveCard&) noexcept { return 0; }
    std::uint32_t append(const DesignNodeMoveCard& card, base::CardKey key);
    [[nodiscard]] DesignNodeMoveCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DesignNodeMoveCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& designVarIdColumn() noexcept { return m_designVarId; }
    [[nodiscard]] Column<base::EntityId>& nodeIdColumn() noexcept { return m_nodeId; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<double>& coefficientColumn() noexcept { return m_coefficient; }
    [[nodiscard]] Column<std::array<double, 3>>& directionColumn() noexcept { return m_direction; }

private:
    Column<base::EntityId> m_designVarId;
    Column<base::EntityId> m_nodeId;
    Column<base::EntityId> m_frameId;
    Column<double> m_coefficient;
    Column<std::array<double, 3>> m_direction;
};

} // namespace oor::model
