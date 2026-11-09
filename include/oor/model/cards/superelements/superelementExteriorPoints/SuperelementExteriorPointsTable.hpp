#pragma once

#include "oor/model/cards/superelements/superelementExteriorPoints/SuperelementExteriorPointsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementExteriorPointsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementExteriorPointsCard& card);
    [[nodiscard]] static std::optional<SuperelementExteriorPointsCard> read(FieldReader& reader);
    static void write(const SuperelementExteriorPointsCard& card, FieldWriter& writer);
};

// All the CSUPEXT cards. The cards of a superelement share its number, their ID (IdSpace::Group, category
// Superelement). The cards are kept whole next to a column of their SEIDs.
class SuperelementExteriorPointsTable final
    : public TypedTable<SuperelementExteriorPointsTable, SuperelementExteriorPointsCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementExteriorPoints";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::Group};
    using Format = SuperelementExteriorPointsFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementExteriorPointsCard& card) noexcept
    {
        return card.superelementId;
    }
    std::uint32_t append(const SuperelementExteriorPointsCard& card, base::CardKey key);
    [[nodiscard]] SuperelementExteriorPointsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementExteriorPointsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_superelementId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    // A single point must exist; the ends of a THRU range may not.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementExteriorPointsCard> m_cards;
};

} // namespace oor::model
