#pragma once

#include "oor/model/cards/solution_control/wettedElemNodes/WettedElemNodesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct WettedElemNodesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const WettedElemNodesCard& card);
    [[nodiscard]] static std::optional<WettedElemNodesCard> read(FieldReader& reader);
    static void write(const WettedElemNodesCard& card, FieldWriter& writer);
};

// All the WETELMG cards, in the category WettedElement with the WETELME cards: the WEID is unique among the WETELMG
// cards (IdSpace::PerCardType) and a WETSURF names a wetted element by it. The grid points are references
// (ReferenceTarget::Node). The cards are kept whole next to a column of their WEIDs, because the list of grid points
// has a length that depends on the shape.
class WettedElemNodesTable final : public TypedTable<WettedElemNodesTable, WettedElemNodesCard>
{
public:
    static constexpr std::string_view NAME = "WettedElemNodes";
    static inline const CardTypeInfo INFO{NAME, CardCategory::WettedElement, base::IdSpace::PerCardType};
    using Format = WettedElemNodesFormat;

    [[nodiscard]] static base::EntityId cardId(const WettedElemNodesCard& card) noexcept { return card.id; }
    std::uint32_t append(const WettedElemNodesCard& card, base::CardKey key);
    [[nodiscard]] WettedElemNodesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const WettedElemNodesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<WettedElemNodesCard> m_cards;
};

} // namespace oor::model
