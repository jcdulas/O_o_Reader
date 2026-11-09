#pragma once

#include "oor/model/cards/explicit/eulerRegionProp/EulerRegionPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerRegionPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerRegionPropCard& card);
    [[nodiscard]] static std::optional<EulerRegionPropCard> read(FieldReader& reader);
    static void write(const EulerRegionPropCard& card, FieldWriter& writer);
};

// All the PEULER1 cards. A PEULER1 is a property: its ID is in the space every property type shares
// (IdSpace::Property). Its SID names a TICEUL1 (the initial regions) and its SID2 a body force (an EULFOR1).
class EulerRegionPropTable final : public TypedTable<EulerRegionPropTable, EulerRegionPropCard>
{
public:
    static constexpr std::string_view NAME = "EulerRegionProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = EulerRegionPropFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerRegionPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerRegionPropCard& card, base::CardKey key);
    [[nodiscard]] EulerRegionPropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerRegionPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerRegionPropCard> m_cards;
};

} // namespace oor::model
