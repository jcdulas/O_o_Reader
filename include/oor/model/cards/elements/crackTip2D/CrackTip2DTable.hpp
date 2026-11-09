#pragma once

#include "oor/model/cards/elements/crackTip2D/CrackTip2DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct CrackTip2DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CrackTip2DCard& card);
    [[nodiscard]] static std::optional<CrackTip2DCard> read(FieldReader& reader);
    static void write(const CrackTip2DCard& card, FieldWriter& writer);
};

// All the CRAC2D elements: the nodes are one column of 18 IDs per element, 0 for a blank field (an (N, 18) NumPy view
// without copy). The ID is an element ID, in the space every element shares; PID names a PRAC2D.
class CrackTip2DTable final : public TypedTable<CrackTip2DTable, CrackTip2DCard>
{
public:
    static constexpr std::string_view NAME = "CrackTip2D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = CrackTip2DFormat;

    [[nodiscard]] static base::EntityId cardId(const CrackTip2DCard& card) noexcept { return card.id; }
    std::uint32_t append(const CrackTip2DCard& card, base::CardKey key);
    [[nodiscard]] CrackTip2DCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CrackTip2DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, CrackTip2DCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, CrackTip2DCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
