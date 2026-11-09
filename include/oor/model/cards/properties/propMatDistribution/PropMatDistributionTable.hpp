#pragma once

#include "oor/model/cards/properties/propMatDistribution/PropMatDistributionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PropMatDistributionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PropMatDistributionCard& card);
    [[nodiscard]] static std::optional<PropMatDistributionCard> read(FieldReader& reader);
    static void write(const PropMatDistributionCard& card, FieldWriter& writer);
};

// All the PDISTBM cards, in the category Other: the ID is unique per card type (IdSpace::PerCardType). The PID is a
// reference to a property (the target Property: that the property is of the kind TYPE says is not part of the
// reference), each EID to an element and each MIDV to a material. The cards are kept whole next to a column of their
// IDs.
class PropMatDistributionTable final : public TypedTable<PropMatDistributionTable, PropMatDistributionCard>
{
public:
    static constexpr std::string_view NAME = "PropMatDistribution";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = PropMatDistributionFormat;

    [[nodiscard]] static base::EntityId cardId(const PropMatDistributionCard& card) noexcept { return card.id; }
    std::uint32_t append(const PropMatDistributionCard& card, base::CardKey key);
    [[nodiscard]] PropMatDistributionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PropMatDistributionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PropMatDistributionCard> m_cards;
};

} // namespace oor::model
