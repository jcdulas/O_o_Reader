#pragma once

#include "oor/model/cards/aero/aeroTrim/AeroTrimCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroTrimFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroTrimCard& card);
    [[nodiscard]] static std::optional<AeroTrimCard> read(FieldReader& reader);
    static void write(const AeroTrimCard& card, FieldWriter& writer);
};

// All the TRIM cards, in the category AeroTrimSet that the TRIM2 cards share: the Case Control TRIM names a SID of
// either (ReferenceTarget::AeroTrimSet), and the format reference wants the SID unique among both
// (`aero.trim-id-repeated` checks across the two types; IdSpace::PerCardType refuses a repeat inside this one). The
// card is kept whole: a TRIM has a few variables.
class AeroTrimTable final : public TypedTable<AeroTrimTable, AeroTrimCard>
{
public:
    static constexpr std::string_view NAME = "AeroTrim";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroTrimSet, base::IdSpace::PerCardType};
    using Format = AeroTrimFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroTrimCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroTrimCard& card, base::CardKey key);
    [[nodiscard]] AeroTrimCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroTrimCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroTrimCard> m_cards;
};

} // namespace oor::model
