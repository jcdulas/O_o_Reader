#pragma once

#include "oor/model/cards/elements/weld/WeldCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct WeldFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const WeldCard& card);
    [[nodiscard]] static std::optional<WeldCard> read(FieldReader& reader);
    static void write(const WeldCard& card, FieldWriter& writer);
};

// All the CWELD elements. The location is an (N, 3) NumPy view (NaN where blank), the two grid lists (N, 8) ones.
class WeldTable final : public TypedTable<WeldTable, WeldCard>
{
public:
    static constexpr std::string_view NAME = "Weld";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = WeldFormat;

    WeldTable()
        : m_kind(std::string("PartPatch")),
          m_matFrameId(WeldCard::NO_MAT_FRAME_ID),
          m_location(std::array<double, 3>{WeldCard::NO_COORDINATE, WeldCard::NO_COORDINATE,
                                           WeldCard::NO_COORDINATE})
    {
    }

    [[nodiscard]] static base::EntityId cardId(const WeldCard& card) noexcept { return card.id; }
    std::uint32_t append(const WeldCard& card, base::CardKey key);
    [[nodiscard]] WeldCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const WeldCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<base::EntityId>& spotNodeColumn() noexcept { return m_spotNode; }
    [[nodiscard]] Column<std::string>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<std::string>& patchShapesColumn() noexcept { return m_patchShapes; }
    [[nodiscard]] Column<base::EntityId>& matFrameIdColumn() noexcept { return m_matFrameId; }
    [[nodiscard]] Column<base::EntityId>& patchIdAColumn() noexcept { return m_patchIdA; }
    [[nodiscard]] Column<base::EntityId>& patchIdBColumn() noexcept { return m_patchIdB; }
    [[nodiscard]] Column<std::array<double, 3>>& locationColumn() noexcept { return m_location; }
    [[nodiscard]] Column<std::array<base::EntityId, WeldCard::GRID_COUNT>>& patchNodesAColumn() noexcept
    {
        return m_patchNodesA;
    }
    [[nodiscard]] Column<std::array<base::EntityId, WeldCard::GRID_COUNT>>& patchNodesBColumn() noexcept
    {
        return m_patchNodesB;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_spotNode;
    Column<std::string> m_kind;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<std::string> m_patchShapes;
    Column<base::EntityId> m_matFrameId;
    Column<base::EntityId> m_patchIdA;
    Column<base::EntityId> m_patchIdB;
    Column<std::array<double, 3>> m_location;
    Column<std::array<base::EntityId, WeldCard::GRID_COUNT>> m_patchNodesA;
    Column<std::array<base::EntityId, WeldCard::GRID_COUNT>> m_patchNodesB;
};

} // namespace oor::model
