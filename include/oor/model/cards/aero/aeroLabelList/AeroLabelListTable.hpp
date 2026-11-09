#pragma once

#include "oor/model/cards/aero/aeroLabelList/AeroLabelListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroLabelListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroLabelListCard& card);
    [[nodiscard]] static std::optional<AeroLabelListCard> read(FieldReader& reader);
    static void write(const AeroLabelListCard& card, FieldWriter& writer);
};

// All the AELISTC cards, in the category Aero. The SID is unique among the AELISTC cards (IdSpace::PerCardType). The
// card is kept whole next to a column of its SID.
class AeroLabelListTable final : public TypedTable<AeroLabelListTable, AeroLabelListCard>
{
public:
    static constexpr std::string_view NAME = "AeroLabelList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::PerCardType};
    using Format = AeroLabelListFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroLabelListCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroLabelListCard& card, base::CardKey key);
    [[nodiscard]] AeroLabelListCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroLabelListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the strings are names

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroLabelListCard> m_cards;
};

} // namespace oor::model
