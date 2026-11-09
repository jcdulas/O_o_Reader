#pragma once

#include "oor/model/cards/elements/solidPyramid/SolidPyramidCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SolidPyramidFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SolidPyramidCard& card);
    [[nodiscard]] static std::optional<SolidPyramidCard> read(FieldReader& reader);
    static void write(const SolidPyramidCard& card, FieldWriter& writer);
};

// All the CPYRAM elements: the nodes are one column of thirteen IDs per element, 0 for an absent edge point (an (N, 13)
// NumPy view without copy).
class SolidPyramidTable final : public TypedTable<SolidPyramidTable, SolidPyramidCard>
{
public:
    static constexpr std::string_view NAME = "SolidPyramid";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = SolidPyramidFormat;

    [[nodiscard]] static base::EntityId cardId(const SolidPyramidCard& card) noexcept { return card.id; }
    std::uint32_t append(const SolidPyramidCard& card, base::CardKey key);
    [[nodiscard]] SolidPyramidCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SolidPyramidCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, SolidPyramidCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, SolidPyramidCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
