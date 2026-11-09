#pragma once

#include "oor/model/cards/explicit/beltProp/BeltPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BeltPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BeltPropCard& card);
    [[nodiscard]] static std::optional<BeltPropCard> read(FieldReader& reader);
    static void write(const BeltPropCard& card, FieldWriter& writer);
};

// All the PBELT cards. A PBELT is a property: its ID is in the space every property type shares (IdSpace::Property), so
// the CROD that names a PID finds it. Its four tables are TABLED1 cards.
class BeltPropTable final : public TypedTable<BeltPropTable, BeltPropCard>
{
public:
    static constexpr std::string_view NAME = "BeltProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BeltPropFormat;

    [[nodiscard]] static base::EntityId cardId(const BeltPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const BeltPropCard& card, base::CardKey key);
    [[nodiscard]] BeltPropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const BeltPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<BeltPropCard> m_cards;
};

} // namespace oor::model
