#pragma once

#include "oor/model/cards/explicit/gasBagHeatTransfer/GasBagHeatTransferCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GasBagHeatTransferFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GasBagHeatTransferCard& card);
    [[nodiscard]] static std::optional<GasBagHeatTransferCard> read(FieldReader& reader);
    static void write(const GasBagHeatTransferCard& card, FieldWriter& writer);
};

// All the HEATLOS cards. The cards of a set share their HTRID, which is the ID of the table (IdSpace::Group, category
// HeatLossSet): a GBAG or a COUPLE names the set, not a card. The HID is a number of the card only. SUBID names a
// surface, HTRTYPID a heat transfer card (any of the category HeatTransferModel), the coefficient may be a table of
// time.
class GasBagHeatTransferTable final : public TypedTable<GasBagHeatTransferTable, GasBagHeatTransferCard>
{
public:
    static constexpr std::string_view NAME = "GasBagHeatTransfer";
    static inline const CardTypeInfo INFO{NAME, CardCategory::HeatLossSet, base::IdSpace::Group};
    using Format = GasBagHeatTransferFormat;

    [[nodiscard]] static base::EntityId cardId(const GasBagHeatTransferCard& card) noexcept { return card.setId; }
    std::uint32_t append(const GasBagHeatTransferCard& card, base::CardKey key);
    [[nodiscard]] GasBagHeatTransferCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const GasBagHeatTransferCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }

private:
    Column<base::EntityId> m_setId;
    CardRows<GasBagHeatTransferCard> m_cards;
};

} // namespace oor::model
