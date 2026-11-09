#pragma once

#include "oor/model/cards/loads/wettedSurfaceLoad/WettedSurfaceLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct WettedSurfaceLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const WettedSurfaceLoadCard& card);
    [[nodiscard]] static std::optional<WettedSurfaceLoadCard> read(FieldReader& reader);
    static void write(const WettedSurfaceLoadCard& card, FieldWriter& writer);
};

// All the WETLOAD cards. The WLID is a set ID that several cards may share (IdSpace::Group, category Other) and it is
// the excitation set that a TLOAD1 names in its EXCITEID, so the table says it defines excitation IDs
// (CardTable::definesExcitationIds). The WSID is a reference to a WETSURF; the SERV_ID is a name. The cards are kept
// whole next to a column of their WLIDs.
class WettedSurfaceLoadTable final : public TypedTable<WettedSurfaceLoadTable, WettedSurfaceLoadCard>
{
public:
    static constexpr std::string_view NAME = "WettedSurfaceLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = WettedSurfaceLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const WettedSurfaceLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const WettedSurfaceLoadCard& card, base::CardKey key);
    [[nodiscard]] WettedSurfaceLoadCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const WettedSurfaceLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] bool definesExcitationIds() const noexcept override { return true; }
    [[nodiscard]] base::EntityId definedExcitationId(std::uint32_t row) const override { return m_id.get(row); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<WettedSurfaceLoadCard> m_cards;
};

} // namespace oor::model
