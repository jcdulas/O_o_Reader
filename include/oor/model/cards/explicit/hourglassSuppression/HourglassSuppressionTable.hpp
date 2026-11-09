#pragma once

#include "oor/model/cards/explicit/hourglassSuppression/HourglassSuppressionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HourglassSuppressionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HourglassSuppressionCard& card);
    [[nodiscard]] static std::optional<HourglassSuppressionCard> read(FieldReader& reader);
    static void write(const HourglassSuppressionCard& card, FieldWriter& writer);
};

// All the HGSUPPR cards. The HID is unique among them (IdSpace::PerCardType) and no other card names it, so the
// category is Other. The PID is a property: it is visited as a property, whatever the class named by PROP (the property
// space is shared by every property type, so the class is not matched against the card type). The cards are kept whole
// (CardRows) next to their ID.
class HourglassSuppressionTable final : public TypedTable<HourglassSuppressionTable, HourglassSuppressionCard>
{
public:
    static constexpr std::string_view NAME = "HourglassSuppression";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = HourglassSuppressionFormat;

    [[nodiscard]] static base::EntityId cardId(const HourglassSuppressionCard& card) noexcept { return card.id; }
    std::uint32_t append(const HourglassSuppressionCard& card, base::CardKey key);
    [[nodiscard]] HourglassSuppressionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const HourglassSuppressionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<HourglassSuppressionCard> m_cards;
};

} // namespace oor::model
