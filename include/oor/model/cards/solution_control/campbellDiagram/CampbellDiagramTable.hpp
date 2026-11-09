#pragma once

#include "oor/model/cards/solution_control/campbellDiagram/CampbellDiagramCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CampbellDiagramFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CampbellDiagramCard& card);
    [[nodiscard]] static std::optional<CampbellDiagramCard> read(FieldReader& reader);
    static void write(const CampbellDiagramCard& card, FieldWriter& writer);
};

// All the CAMPBLL cards. The ID is unique among them (IdSpace::PerCardType, category Other: the Case Control command
// CAMPBELL is text and no card names a CAMPBLL). The DDVALID is a DiscreteValueSet reference. The cards are kept whole
// next to the columns the query engine reads: the ID and the DDVAL.
class CampbellDiagramTable final : public TypedTable<CampbellDiagramTable, CampbellDiagramCard>
{
public:
    static constexpr std::string_view NAME = "CampbellDiagram";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = CampbellDiagramFormat;

    [[nodiscard]] static base::EntityId cardId(const CampbellDiagramCard& card) noexcept { return card.id; }
    std::uint32_t append(const CampbellDiagramCard& card, base::CardKey key);
    [[nodiscard]] CampbellDiagramCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CampbellDiagramCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& discreteValuesIdColumn() noexcept { return m_discreteValuesId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_discreteValuesId;
    CardRows<CampbellDiagramCard> m_cards;
};

} // namespace oor::model
