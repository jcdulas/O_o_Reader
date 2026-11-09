#pragma once

#include "oor/model/cards/superelements/superelementInteriorNodes/SuperelementInteriorNodesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementInteriorNodesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementInteriorNodesCard& card);
    [[nodiscard]] static std::optional<SuperelementInteriorNodesCard> read(FieldReader& reader);
    static void write(const SuperelementInteriorNodesCard& card, FieldWriter& writer);
};

// All the SESET cards. The cards of a superelement share its number, which is their ID (IdSpace::Group, category
// Superelement): the number defines the superelement. The cards are kept whole next to a column of their SEIDs.
class SuperelementInteriorNodesTable final
    : public TypedTable<SuperelementInteriorNodesTable, SuperelementInteriorNodesCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementInteriorNodes";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::Group};
    using Format = SuperelementInteriorNodesFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementInteriorNodesCard& card) noexcept
    {
        return card.superelementId;
    }
    std::uint32_t append(const SuperelementInteriorNodesCard& card, base::CardKey key);
    [[nodiscard]] SuperelementInteriorNodesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementInteriorNodesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_superelementId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    // The points of the list are grid points that may not exist (the solver does not report them): NodeInRange.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementInteriorNodesCard> m_cards;
};

} // namespace oor::model
