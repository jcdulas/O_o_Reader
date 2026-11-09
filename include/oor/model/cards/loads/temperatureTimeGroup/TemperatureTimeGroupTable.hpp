#pragma once

#include "oor/model/cards/loads/temperatureTimeGroup/TemperatureTimeGroupCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TemperatureTimeGroupFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TemperatureTimeGroupCard& card);
    [[nodiscard]] static std::optional<TemperatureTimeGroupCard> read(FieldReader& reader);
    static void write(const TemperatureTimeGroupCard& card, FieldWriter& writer);
};

// All the TMPSET cards, in the category TemperatureGroup: the ID is unique per card type (IdSpace::PerCardType) and is
// what the GROUP_ID of a TTEMP names. The cards are kept whole next to their ID. A single grid point of the list is a
// Node reference and the ends of a THRU range are NodeInRange.
class TemperatureTimeGroupTable final : public TypedTable<TemperatureTimeGroupTable, TemperatureTimeGroupCard>
{
public:
    static constexpr std::string_view NAME = "TemperatureTimeGroup";
    static inline const CardTypeInfo INFO{NAME, CardCategory::TemperatureGroup, base::IdSpace::PerCardType};
    using Format = TemperatureTimeGroupFormat;

    [[nodiscard]] static base::EntityId cardId(const TemperatureTimeGroupCard& card) noexcept { return card.id; }
    std::uint32_t append(const TemperatureTimeGroupCard& card, base::CardKey key);
    [[nodiscard]] TemperatureTimeGroupCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TemperatureTimeGroupCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TemperatureTimeGroupCard> m_cards;
};

} // namespace oor::model
