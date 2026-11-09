#pragma once

#include "oor/model/cards/superelements/superelementElemTransfer/SuperelementElemTransferCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementElemTransferFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementElemTransferCard& card);
    [[nodiscard]] static std::optional<SuperelementElemTransferCard> read(FieldReader& reader);
    static void write(const SuperelementElemTransferCard& card, FieldWriter& writer);
};

// All the SEELT cards. The cards of a superelement share its number, their ID (IdSpace::Group, category Superelement),
// and the number 0 of the residual structure is not renumbered. The cards are kept whole next to a column of their
// SEIDs.
class SuperelementElemTransferTable final
    : public TypedTable<SuperelementElemTransferTable, SuperelementElemTransferCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementElemTransfer";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::Group};
    using Format = SuperelementElemTransferFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementElemTransferCard& card) noexcept
    {
        return card.superelementId;
    }
    std::uint32_t append(const SuperelementElemTransferCard& card, base::CardKey key);
    [[nodiscard]] SuperelementElemTransferCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementElemTransferCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_superelementId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    // A single element must exist; the ends of a THRU range may not (the solver skips the gaps).
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementElemTransferCard> m_cards;
};

} // namespace oor::model
