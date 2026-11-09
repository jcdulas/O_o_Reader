#pragma once

#include "oor/model/cards/nonlinear_contact/brakeProp/BrakePropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BrakePropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BrakePropCard& card);
    [[nodiscard]] static std::optional<BrakePropCard> read(FieldReader& reader);
    static void write(const BrakePropCard& card, FieldWriter& writer);
};

// All the BRKPROP cards. The ID is unique among them (IdSpace::PerCardType, category BrakeProperty): the BProp_ID of a
// BRKSYS names it. TAVSTIF names a TABL3Di (`MultiDimensionalTable`). The cards are kept whole next to their ID.
class BrakePropTable final : public TypedTable<BrakePropTable, BrakePropCard>
{
public:
    static constexpr std::string_view NAME = "BrakeProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::BrakeProperty, base::IdSpace::PerCardType};
    using Format = BrakePropFormat;

    [[nodiscard]] static base::EntityId cardId(const BrakePropCard& card) noexcept { return card.id; }
    std::uint32_t append(const BrakePropCard& card, base::CardKey key);
    [[nodiscard]] BrakePropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const BrakePropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<BrakePropCard> m_cards;
};

} // namespace oor::model
