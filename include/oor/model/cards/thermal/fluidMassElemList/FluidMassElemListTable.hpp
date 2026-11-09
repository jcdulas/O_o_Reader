#pragma once

#include "oor/model/cards/thermal/fluidMassElemList/FluidMassElemListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FluidMassElemListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FluidMassElemListCard& card);
    [[nodiscard]] static std::optional<FluidMassElemListCard> read(FieldReader& reader);
    static void write(const FluidMassElemListCard& card, FieldWriter& writer);
};

// All the ELIST cards, in the category ElementList: the LID is unique per card type (IdSpace::PerCardType) and is what
// the ELIST1 and ELIST2 of an MFLUID name. Every element of the list, and each end of a THRU range, is visited as
// ReferenceTarget::ElementInRange with its absolute value (the sign says the wetted side and stays on the card), so a
// renumbering of the elements follows into the list and an element the model does not have is not an unresolved
// reference (the solver skips the gaps of a range, and the lists are a check of their own). The cards are kept whole
// next to a column of their LIDs.
class FluidMassElemListTable final : public TypedTable<FluidMassElemListTable, FluidMassElemListCard>
{
public:
    static constexpr std::string_view NAME = "FluidMassElemList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ElementList, base::IdSpace::PerCardType};
    using Format = FluidMassElemListFormat;

    [[nodiscard]] static base::EntityId cardId(const FluidMassElemListCard& card) noexcept { return card.id; }
    std::uint32_t append(const FluidMassElemListCard& card, base::CardKey key);
    [[nodiscard]] FluidMassElemListCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FluidMassElemListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FluidMassElemListCard> m_cards;
};

} // namespace oor::model
