#pragma once

#include "oor/model/cards/explicit/couplingInteraction/CouplingInteractionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CouplingInteractionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CouplingInteractionCard& card);
    [[nodiscard]] static std::optional<CouplingInteractionCard> read(FieldReader& reader);
    static void write(const CouplingInteractionCard& card, FieldWriter& writer);
};

// All the COUPINT cards: the CID is unique among them (IdSpace::PerCardType, CardCategory::Other: no card names a
// COUPINT). CID1 and CID2 are COUPLE cards.
class CouplingInteractionTable final : public TypedTable<CouplingInteractionTable, CouplingInteractionCard>
{
public:
    static constexpr std::string_view NAME = "CouplingInteraction";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = CouplingInteractionFormat;

    [[nodiscard]] static base::EntityId cardId(const CouplingInteractionCard& card) noexcept { return card.id; }
    std::uint32_t append(const CouplingInteractionCard& card, base::CardKey key);
    [[nodiscard]] CouplingInteractionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CouplingInteractionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CouplingInteractionCard> m_cards;
};

} // namespace oor::model
