#pragma once

#include "oor/model/cards/solution_control/transferPathSetup/TransferPathSetupCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TransferPathSetupFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TransferPathSetupCard& card);
    [[nodiscard]] static std::optional<TransferPathSetupCard> read(FieldReader& reader);
    static void write(const TransferPathSetupCard& card, FieldWriter& writer);
};

// All the TPADEFN cards, in the category TransferPathSet: the ID is unique per card type (IdSpace::PerCardType) and is
// what the Case Control TPASET names. The cards are kept whole next to their ID. GRIDSET and every OUTSET are Set
// references.
class TransferPathSetupTable final : public TypedTable<TransferPathSetupTable, TransferPathSetupCard>
{
public:
    static constexpr std::string_view NAME = "TransferPathSetup";
    static inline const CardTypeInfo INFO{NAME, CardCategory::TransferPathSet, base::IdSpace::PerCardType};
    using Format = TransferPathSetupFormat;

    [[nodiscard]] static base::EntityId cardId(const TransferPathSetupCard& card) noexcept { return card.id; }
    std::uint32_t append(const TransferPathSetupCard& card, base::CardKey key);
    [[nodiscard]] TransferPathSetupCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TransferPathSetupCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TransferPathSetupCard> m_cards;
};

} // namespace oor::model
