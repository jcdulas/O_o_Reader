#pragma once

#include "oor/model/cards/elements/crackTip3D/CrackTip3DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct CrackTip3DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CrackTip3DCard& card);
    [[nodiscard]] static std::optional<CrackTip3DCard> read(FieldReader& reader);
    static void write(const CrackTip3DCard& card, FieldWriter& writer);
};

// All the CRAC3D elements: the nodes are one column of 64 IDs per element, 0 for a blank field (an (N, 64) NumPy view
// without copy). The ID is an element ID, in the space every element shares; PID names a PRAC3D.
class CrackTip3DTable final : public TypedTable<CrackTip3DTable, CrackTip3DCard>
{
public:
    static constexpr std::string_view NAME = "CrackTip3D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = CrackTip3DFormat;

    [[nodiscard]] static base::EntityId cardId(const CrackTip3DCard& card) noexcept { return card.id; }
    std::uint32_t append(const CrackTip3DCard& card, base::CardKey key);
    [[nodiscard]] CrackTip3DCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CrackTip3DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, CrackTip3DCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, CrackTip3DCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
