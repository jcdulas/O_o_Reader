#pragma once

#include "oor/model/cards/explicit/hybridInflatorFractions/HybridInflatorFractionsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HybridInflatorFractionsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HybridInflatorFractionsCard& card);
    [[nodiscard]] static std::optional<HybridInflatorFractionsCard> read(FieldReader& reader);
    static void write(const HybridInflatorFractionsCard& card, FieldWriter& writer);
};

// All the INFLFRC cards. The FRACID is unique among them (IdSpace::PerCardType, category GasFractions): an INFLHB
// names it. An INFLFRC refers to no other card.
class HybridInflatorFractionsTable final : public TypedTable<HybridInflatorFractionsTable, HybridInflatorFractionsCard>
{
public:
    static constexpr std::string_view NAME = "HybridInflatorFractions";
    static inline const CardTypeInfo INFO{NAME, CardCategory::GasFractions, base::IdSpace::PerCardType};
    using Format = HybridInflatorFractionsFormat;

    [[nodiscard]] static base::EntityId cardId(const HybridInflatorFractionsCard& card) noexcept { return card.id; }
    std::uint32_t append(const HybridInflatorFractionsCard& card, base::CardKey key);
    [[nodiscard]] HybridInflatorFractionsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const HybridInflatorFractionsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an INFLFRC refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<HybridInflatorFractionsCard> m_cards;
};

} // namespace oor::model
