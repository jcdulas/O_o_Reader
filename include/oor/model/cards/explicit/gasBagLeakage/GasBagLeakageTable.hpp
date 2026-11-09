#pragma once

#include "oor/model/cards/explicit/gasBagLeakage/GasBagLeakageCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GasBagLeakageFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GasBagLeakageCard& card);
    [[nodiscard]] static std::optional<GasBagLeakageCard> read(FieldReader& reader);
    static void write(const GasBagLeakageCard& card, FieldWriter& writer);
};

// All the LEAKAGE cards. The cards of a set share their PORID, which is the ID of the table (IdSpace::Group, category
// Leakage): a GBAG or a COUPLE names the set, not a card. The LID is a number of the card only. SUBID names a surface,
// PORTYPID a porosity card (any of the category Porosity), the coefficient may be a table of time.
class GasBagLeakageTable final : public TypedTable<GasBagLeakageTable, GasBagLeakageCard>
{
public:
    static constexpr std::string_view NAME = "GasBagLeakage";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Leakage, base::IdSpace::Group};
    using Format = GasBagLeakageFormat;

    [[nodiscard]] static base::EntityId cardId(const GasBagLeakageCard& card) noexcept { return card.setId; }
    std::uint32_t append(const GasBagLeakageCard& card, base::CardKey key);
    [[nodiscard]] GasBagLeakageCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const GasBagLeakageCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }

private:
    Column<base::EntityId> m_setId;
    CardRows<GasBagLeakageCard> m_cards;
};

} // namespace oor::model
