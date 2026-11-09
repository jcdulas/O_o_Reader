#pragma once

#include "oor/model/cards/explicit/initialGasComposition/InitialGasCompositionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct InitialGasCompositionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const InitialGasCompositionCard& card);
    [[nodiscard]] static std::optional<InitialGasCompositionCard> read(FieldReader& reader);
    static void write(const InitialGasCompositionCard& card, FieldWriter& writer);
};

// All the INITGAS cards. The INTID is unique among them (IdSpace::PerCardType, category InitialGas): a GBAG or a COUPLE
// names it. Each gas is an INFLGAS.
class InitialGasCompositionTable final : public TypedTable<InitialGasCompositionTable, InitialGasCompositionCard>
{
public:
    static constexpr std::string_view NAME = "InitialGasComposition";
    static inline const CardTypeInfo INFO{NAME, CardCategory::InitialGas, base::IdSpace::PerCardType};
    using Format = InitialGasCompositionFormat;

    [[nodiscard]] static base::EntityId cardId(const InitialGasCompositionCard& card) noexcept { return card.id; }
    std::uint32_t append(const InitialGasCompositionCard& card, base::CardKey key);
    [[nodiscard]] InitialGasCompositionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const InitialGasCompositionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<InitialGasCompositionCard> m_cards;
};

} // namespace oor::model
