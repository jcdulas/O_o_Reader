#pragma once

#include "oor/model/cards/solution_control/fatigueDefinition/FatigueDefinitionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FatigueDefinitionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FatigueDefinitionCard& card);
    [[nodiscard]] static std::optional<FatigueDefinitionCard> read(FieldReader& reader);
    static void write(const FatigueDefinitionCard& card, FieldWriter& writer);
};

// All the FTGDEF cards (IdSpace::PerCardType, category Other: the Case Control FATIGUE command selects one). The PFTGID
// and PFTGIDi are FatigueProperty references, the ELSIDi, NDSIDi and XELSIDi Set references. The cards are kept whole
// (CardRows) next to their ID: they have sections of any length, and the query engine reads the ID.
class FatigueDefinitionTable final : public TypedTable<FatigueDefinitionTable, FatigueDefinitionCard>
{
public:
    static constexpr std::string_view NAME = "FatigueDefinition";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = FatigueDefinitionFormat;

    [[nodiscard]] static base::EntityId cardId(const FatigueDefinitionCard& card) noexcept { return card.id; }
    std::uint32_t append(const FatigueDefinitionCard& card, base::CardKey key);
    [[nodiscard]] FatigueDefinitionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FatigueDefinitionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FatigueDefinitionCard> m_cards;
};

} // namespace oor::model
