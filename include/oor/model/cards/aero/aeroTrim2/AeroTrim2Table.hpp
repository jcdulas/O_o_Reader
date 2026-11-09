#pragma once

#include "oor/model/cards/aero/aeroTrim2/AeroTrim2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroTrim2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroTrim2Card& card);
    [[nodiscard]] static std::optional<AeroTrim2Card> read(FieldReader& reader);
    static void write(const AeroTrim2Card& card, FieldWriter& writer);
};

// All the TRIM2 cards, in the category AeroTrimSet that the TRIM cards share: the Case Control TRIM names a SID of
// either (ReferenceTarget::AeroTrimSet), and the format reference wants the SID unique among both (a check across the
// two types; IdSpace::PerCardType refuses a repeat inside this one). The card is kept whole next to a column of its
// SID.
class AeroTrim2Table final : public TypedTable<AeroTrim2Table, AeroTrim2Card>
{
public:
    static constexpr std::string_view NAME = "AeroTrim2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroTrimSet, base::IdSpace::PerCardType};
    using Format = AeroTrim2Format;

    [[nodiscard]] static base::EntityId cardId(const AeroTrim2Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroTrim2Card& card, base::CardKey key);
    [[nodiscard]] AeroTrim2Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroTrim2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the variables are named by label

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroTrim2Card> m_cards;
};

} // namespace oor::model
