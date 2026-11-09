#pragma once

#include "oor/model/cards/explicit/hybridInflator/HybridInflatorCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HybridInflatorFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HybridInflatorCard& card);
    [[nodiscard]] static std::optional<HybridInflatorCard> read(FieldReader& reader);
    static void write(const HybridInflatorCard& card, FieldWriter& writer);
};

// All the INFLHB cards. The INFLID is unique among the INFLHB cards (IdSpace::PerCardType, category Inflator: INFLCG,
// INFLTNK and INFLTR share the category, because an ABINFL names one of the four by number). The tables are TABLED1
// cards, FRAC names an INFLFRC and the GASNAM list INFLGAS cards.
class HybridInflatorTable final : public TypedTable<HybridInflatorTable, HybridInflatorCard>
{
public:
    static constexpr std::string_view NAME = "HybridInflator";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Inflator, base::IdSpace::PerCardType};
    using Format = HybridInflatorFormat;

    [[nodiscard]] static base::EntityId cardId(const HybridInflatorCard& card) noexcept { return card.id; }
    std::uint32_t append(const HybridInflatorCard& card, base::CardKey key);
    [[nodiscard]] HybridInflatorCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const HybridInflatorCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<HybridInflatorCard> m_cards;
};

} // namespace oor::model
