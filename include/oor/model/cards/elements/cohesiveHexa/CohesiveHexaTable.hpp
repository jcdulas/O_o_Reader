#pragma once

#include "oor/model/cards/elements/cohesiveHexa/CohesiveHexaCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct CohesiveHexaFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CohesiveHexaCard& card);
    [[nodiscard]] static std::optional<CohesiveHexaCard> read(FieldReader& reader);
    static void write(const CohesiveHexaCard& card, FieldWriter& writer);
};

// All the CIFHEX elements: an element ID, in the space every element shares (IdSpace::Element). PID names a PCOHE and
// the points are grid points. The nodes are one column of twenty IDs per element, 0 for an absent optional point (an
// (N, 20) NumPy view without copy).
class CohesiveHexaTable final : public TypedTable<CohesiveHexaTable, CohesiveHexaCard>
{
public:
    static constexpr std::string_view NAME = "CohesiveHexa";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = CohesiveHexaFormat;

    [[nodiscard]] static base::EntityId cardId(const CohesiveHexaCard& card) noexcept { return card.id; }
    std::uint32_t append(const CohesiveHexaCard& card, base::CardKey key);
    [[nodiscard]] CohesiveHexaCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CohesiveHexaCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, CohesiveHexaCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, CohesiveHexaCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
