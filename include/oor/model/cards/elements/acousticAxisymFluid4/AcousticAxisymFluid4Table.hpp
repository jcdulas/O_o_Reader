#pragma once

#include "oor/model/cards/elements/acousticAxisymFluid4/AcousticAxisymFluid4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticAxisymFluid4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticAxisymFluid4Card& card);
    [[nodiscard]] static std::optional<AcousticAxisymFluid4Card> read(FieldReader& reader);
    static void write(const AcousticAxisymFluid4Card& card, FieldWriter& writer);
};

// All the CAXIF4 cards. A fluid element is an element (IdSpace::Element, CardCategory::Element): its EID is unique
// among all the structural and fluid elements. Its points are GRIDF fluid points that no card defines: nothing is
// visited.
class AcousticAxisymFluid4Table final
    : public TypedTable<AcousticAxisymFluid4Table, AcousticAxisymFluid4Card>
{
public:
    static constexpr std::string_view NAME = "AcousticAxisymFluid4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = AcousticAxisymFluid4Format;

    [[nodiscard]] static base::EntityId cardId(const AcousticAxisymFluid4Card& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticAxisymFluid4Card& card, base::CardKey key);
    [[nodiscard]] AcousticAxisymFluid4Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AcousticAxisymFluid4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AcousticAxisymFluid4Card> m_cards;
};

} // namespace oor::model
