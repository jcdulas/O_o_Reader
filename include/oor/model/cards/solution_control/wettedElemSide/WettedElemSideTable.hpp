#pragma once

#include "oor/model/cards/solution_control/wettedElemSide/WettedElemSideCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct WettedElemSideFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const WettedElemSideCard& card);
    [[nodiscard]] static std::optional<WettedElemSideCard> read(FieldReader& reader);
    static void write(const WettedElemSideCard& card, FieldWriter& writer);
};

// All the WETELME cards, in the category WettedElement with the WETELMG cards: the WEID is unique among the WETELME
// cards (IdSpace::PerCardType) and a WETSURF names a wetted element by it. The structural element is a reference
// (ReferenceTarget::Element). The cards are kept whole next to a column of their WEIDs.
class WettedElemSideTable final : public TypedTable<WettedElemSideTable, WettedElemSideCard>
{
public:
    static constexpr std::string_view NAME = "WettedElemSide";
    static inline const CardTypeInfo INFO{NAME, CardCategory::WettedElement, base::IdSpace::PerCardType};
    using Format = WettedElemSideFormat;

    [[nodiscard]] static base::EntityId cardId(const WettedElemSideCard& card) noexcept { return card.id; }
    std::uint32_t append(const WettedElemSideCard& card, base::CardKey key);
    [[nodiscard]] WettedElemSideCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const WettedElemSideCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<WettedElemSideCard> m_cards;
};

} // namespace oor::model
