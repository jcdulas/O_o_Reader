#pragma once

#include "oor/model/cards/properties/propDistribution/PropDistributionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PropDistributionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PropDistributionCard& card);
    [[nodiscard]] static std::optional<PropDistributionCard> read(FieldReader& reader);
    static void write(const PropDistributionCard& card, FieldWriter& writer);
};

// All the PDISTB cards, in the category Other: the ID is unique per card type (IdSpace::PerCardType). The PID is a
// reference to a property (the target Property: a renumbering of the properties follows into it; that the property is
// of the kind TYPE says is not part of the reference) and each EID is a reference to an element. The cards are kept
// whole next to a column of their IDs.
class PropDistributionTable final : public TypedTable<PropDistributionTable, PropDistributionCard>
{
public:
    static constexpr std::string_view NAME = "PropDistribution";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = PropDistributionFormat;

    [[nodiscard]] static base::EntityId cardId(const PropDistributionCard& card) noexcept { return card.id; }
    std::uint32_t append(const PropDistributionCard& card, base::CardKey key);
    [[nodiscard]] PropDistributionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PropDistributionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PropDistributionCard> m_cards;
};

} // namespace oor::model
