#pragma once

#include "oor/model/cards/solution_control/fatigueParams/FatigueParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FatigueParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FatigueParamsCard& card);
    [[nodiscard]] static std::optional<FatigueParamsCard> read(FieldReader& reader);
    static void write(const FatigueParamsCard& card, FieldWriter& writer);
};

// All the FTGPARM cards (IdSpace::PerCardType, category Other: the Case Control FATIGUE command selects one). The
// OUTPUT of the NAVG line is a Coordinate reference when it is positive. The cards are kept whole (CardRows) next to
// their ID: they have nine optional lines of many fields each, and the query engine reads the ID.
class FatigueParamsTable final : public TypedTable<FatigueParamsTable, FatigueParamsCard>
{
public:
    static constexpr std::string_view NAME = "FatigueParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = FatigueParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const FatigueParamsCard& card) noexcept { return card.id; }
    std::uint32_t append(const FatigueParamsCard& card, base::CardKey key);
    [[nodiscard]] FatigueParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FatigueParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FatigueParamsCard> m_cards;
};

} // namespace oor::model
