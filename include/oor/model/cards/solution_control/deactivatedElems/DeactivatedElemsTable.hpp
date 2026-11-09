#pragma once

#include "oor/model/cards/solution_control/deactivatedElems/DeactivatedElemsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DeactivatedElemsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DeactivatedElemsCard& card);
    [[nodiscard]] static std::optional<DeactivatedElemsCard> read(FieldReader& reader);
    static void write(const DeactivatedElemsCard& card, FieldWriter& writer);
};

// All the DEACTEL cards. The ID is unique among them (IdSpace::PerCardType, category DeactivatedElementSet): the Case
// Control DEACTEL command selects it. ISET names a SET3 (`Set`). The cards are kept whole next to their ID.
class DeactivatedElemsTable final : public TypedTable<DeactivatedElemsTable, DeactivatedElemsCard>
{
public:
    static constexpr std::string_view NAME = "DeactivatedElems";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DeactivatedElementSet, base::IdSpace::PerCardType};
    using Format = DeactivatedElemsFormat;

    [[nodiscard]] static base::EntityId cardId(const DeactivatedElemsCard& card) noexcept { return card.id; }
    std::uint32_t append(const DeactivatedElemsCard& card, base::CardKey key);
    [[nodiscard]] DeactivatedElemsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DeactivatedElemsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DeactivatedElemsCard> m_cards;
};

} // namespace oor::model
