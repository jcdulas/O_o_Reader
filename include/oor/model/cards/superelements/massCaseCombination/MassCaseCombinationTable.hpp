#pragma once

#include "oor/model/cards/superelements/massCaseCombination/MassCaseCombinationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct MassCaseCombinationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MassCaseCombinationCard& card);
    [[nodiscard]] static std::optional<MassCaseCombinationCard> read(FieldReader& reader);
    static void write(const MassCaseCombinationCard& card, FieldWriter& writer);
};

// All the MASSSET cards, filed under their ID (IdSpace::PerCardType, category MassCombination). The cards are kept
// whole next to a column of their IDs.
class MassCaseCombinationTable final : public TypedTable<MassCaseCombinationTable, MassCaseCombinationCard>
{
public:
    static constexpr std::string_view NAME = "MassCaseCombination";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MassCombination, base::IdSpace::PerCardType};
    using Format = MassCaseCombinationFormat;

    [[nodiscard]] static base::EntityId cardId(const MassCaseCombinationCard& card) noexcept { return card.id; }
    std::uint32_t append(const MassCaseCombinationCard& card, base::CardKey key);
    [[nodiscard]] MassCaseCombinationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MassCaseCombinationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    // The mass case of a term is a mass increment (ReferenceTarget::MassIncrement); 0, the base case, is not a
    // reference.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& overallScaleColumn() noexcept { return m_overallScale; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_overallScale;
    CardRows<MassCaseCombinationCard> m_cards;
};

} // namespace oor::model
