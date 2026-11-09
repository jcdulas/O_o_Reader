#pragma once

#include "oor/model/cards/solution_control/wettedSurface/WettedSurfaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct WettedSurfaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const WettedSurfaceCard& card);
    [[nodiscard]] static std::optional<WettedSurfaceCard> read(FieldReader& reader);
    static void write(const WettedSurfaceCard& card, FieldWriter& writer);
};

// All the WETSURF cards, in the category WettedSurface: the WSID is unique among them (IdSpace::PerCardType) and a
// WETLOAD names a surface by it. The wetted elements are references (ReferenceTarget::WettedElement, which covers the
// WETELME and WETELMG cards): a single ID and the two ends of a run are looked up alike. The cards are kept whole next
// to a column of their WSIDs, because the list of runs has a length of its own.
class WettedSurfaceTable final : public TypedTable<WettedSurfaceTable, WettedSurfaceCard>
{
public:
    static constexpr std::string_view NAME = "WettedSurface";
    static inline const CardTypeInfo INFO{NAME, CardCategory::WettedSurface, base::IdSpace::PerCardType};
    using Format = WettedSurfaceFormat;

    [[nodiscard]] static base::EntityId cardId(const WettedSurfaceCard& card) noexcept { return card.id; }
    std::uint32_t append(const WettedSurfaceCard& card, base::CardKey key);
    [[nodiscard]] WettedSurfaceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const WettedSurfaceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<WettedSurfaceCard> m_cards;
};

} // namespace oor::model
