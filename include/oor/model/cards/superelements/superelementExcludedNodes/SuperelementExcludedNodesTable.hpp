#pragma once

#include "oor/model/cards/superelements/superelementExcludedNodes/SuperelementExcludedNodesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementExcludedNodesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementExcludedNodesCard& card);
    [[nodiscard]] static std::optional<SuperelementExcludedNodesCard> read(FieldReader& reader);
    static void write(const SuperelementExcludedNodesCard& card, FieldWriter& writer);
};

// All the SEEXCLD cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to a
// column of their SEIDAs.
class SuperelementExcludedNodesTable final
    : public TypedTable<SuperelementExcludedNodesTable, SuperelementExcludedNodesCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementExcludedNodes";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementExcludedNodesFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementExcludedNodesCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementExcludedNodesCard& card, base::CardKey key);
    [[nodiscard]] SuperelementExcludedNodesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementExcludedNodesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEIDA and SEIDB are superelements; the points are those of superelement SEIDA.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdAColumn() noexcept { return m_superelementIdA; }

private:
    Column<base::EntityId> m_superelementIdA;
    CardRows<SuperelementExcludedNodesCard> m_cards;
};

} // namespace oor::model
