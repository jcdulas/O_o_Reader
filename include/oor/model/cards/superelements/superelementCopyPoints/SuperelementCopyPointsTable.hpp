#pragma once

#include "oor/model/cards/superelements/superelementCopyPoints/SuperelementCopyPointsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementCopyPointsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementCopyPointsCard& card);
    [[nodiscard]] static std::optional<SuperelementCopyPointsCard> read(FieldReader& reader);
    static void write(const SuperelementCopyPointsCard& card, FieldWriter& writer);
};

// All the CSUPER cards, filed under the number of the secondary superelement (SSID below 10000: IdSpace::Group,
// category Superelement). The cards are kept whole next to a column of their numbers.
class SuperelementCopyPointsTable final : public TypedTable<SuperelementCopyPointsTable, SuperelementCopyPointsCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementCopyPoints";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::Group};
    using Format = SuperelementCopyPointsFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementCopyPointsCard& card) noexcept
    {
        return card.number();
    }
    std::uint32_t append(const SuperelementCopyPointsCard& card, base::CardKey key);
    [[nodiscard]] SuperelementCopyPointsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementCopyPointsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_number.get(row); }
    // Gives the card another number and keeps its code. A number of 10000 or more would not fit the field (UsageError).
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    // PSID is a superelement (its sign is the mirror flag, not part of the number); the points are those of the card's
    // own model.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& numberColumn() noexcept { return m_number; }

private:
    Column<base::EntityId> m_number;
    CardRows<SuperelementCopyPointsCard> m_cards;
};

} // namespace oor::model
