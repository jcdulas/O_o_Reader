#pragma once

#include "oor/model/cards/explicit/gasBagSwitch/GasBagSwitchCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GasBagSwitchFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GasBagSwitchCard& card);
    [[nodiscard]] static std::optional<GasBagSwitchCard> read(FieldReader& reader);
    static void write(const GasBagSwitchCard& card, FieldWriter& writer);
};

// All the GBAGCOU cards. The ID is unique among them (IdSpace::PerCardType) and nothing names it, so the category is
// Other. CID names a COUPLE and GID a GBAG.
class GasBagSwitchTable final : public TypedTable<GasBagSwitchTable, GasBagSwitchCard>
{
public:
    static constexpr std::string_view NAME = "GasBagSwitch";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = GasBagSwitchFormat;

    [[nodiscard]] static base::EntityId cardId(const GasBagSwitchCard& card) noexcept { return card.id; }
    std::uint32_t append(const GasBagSwitchCard& card, base::CardKey key);
    [[nodiscard]] GasBagSwitchCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const GasBagSwitchCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<GasBagSwitchCard> m_cards;
};

} // namespace oor::model
