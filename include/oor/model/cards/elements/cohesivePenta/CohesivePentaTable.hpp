#pragma once

#include "oor/model/cards/elements/cohesivePenta/CohesivePentaCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct CohesivePentaFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CohesivePentaCard& card);
    [[nodiscard]] static std::optional<CohesivePentaCard> read(FieldReader& reader);
    static void write(const CohesivePentaCard& card, FieldWriter& writer);
};

// All the CIFPENT elements: an element ID, in the space every element shares (IdSpace::Element). PID names a PCOHE and
// the points are grid points. The nodes are one column of fifteen IDs per element, 0 for an absent optional point (an
// (N, 15) NumPy view without copy).
class CohesivePentaTable final : public TypedTable<CohesivePentaTable, CohesivePentaCard>
{
public:
    static constexpr std::string_view NAME = "CohesivePenta";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = CohesivePentaFormat;

    [[nodiscard]] static base::EntityId cardId(const CohesivePentaCard& card) noexcept { return card.id; }
    std::uint32_t append(const CohesivePentaCard& card, base::CardKey key);
    [[nodiscard]] CohesivePentaCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CohesivePentaCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, CohesivePentaCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, CohesivePentaCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
