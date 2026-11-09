#pragma once

#include "oor/model/cards/loads/inflatorInlet/InflatorInletCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct InflatorInletFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const InflatorInletCard& card);
    [[nodiscard]] static std::optional<InflatorInletCard> read(FieldReader& reader);
    static void write(const InflatorInletCard& card, FieldWriter& writer);
};

// All the ABINFL cards. The cards of a set share their INFID, which is the ID of the table (IdSpace::Group, category
// InflatorSet): a GBAG or a COUPLE names the set, not a card. The CID is a number of the card only. SUBID names a
// surface, INFTYPID an inflator card (category Inflator), the coefficient may name a table of time.
class InflatorInletTable final : public TypedTable<InflatorInletTable, InflatorInletCard>
{
public:
    static constexpr std::string_view NAME = "InflatorInlet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::InflatorSet, base::IdSpace::Group};
    using Format = InflatorInletFormat;

    [[nodiscard]] static base::EntityId cardId(const InflatorInletCard& card) noexcept { return card.inflatorId; }
    std::uint32_t append(const InflatorInletCard& card, base::CardKey key);
    [[nodiscard]] InflatorInletCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const InflatorInletCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_inflatorId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& inflatorIdColumn() noexcept { return m_inflatorId; }

private:
    Column<base::EntityId> m_inflatorId;
    CardRows<InflatorInletCard> m_cards;
};

} // namespace oor::model
