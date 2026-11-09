#pragma once

#include "oor/model/cards/solution_control/fluidStructureControl/FluidStructureControlCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct FluidStructureControlFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FluidStructureControlCard& card);
    [[nodiscard]] static std::optional<FluidStructureControlCard> read(FieldReader& reader);
    static void write(const FluidStructureControlCard& card, FieldWriter& writer);
};

// All the FSICTRL cards, in the category Other. A card has no numeric ID (IdSpace::None): it is found by its SERV_ID,
// the name of the service, which the WETLOAD cards of the service repeat (a name, not a reference: a rule between cards
// can match them). The card names no other card. The cards are kept whole next to a column of their service names.
class FluidStructureControlTable final : public TypedTable<FluidStructureControlTable, FluidStructureControlCard>
{
public:
    static constexpr std::string_view NAME = "FluidStructureControl";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = FluidStructureControlFormat;

    [[nodiscard]] static base::EntityId cardId(const FluidStructureControlCard&) noexcept { return 0; }
    std::uint32_t append(const FluidStructureControlCard& card, base::CardKey key);
    [[nodiscard]] FluidStructureControlCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FluidStructureControlCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    // The service name of a row, without copying the card.
    [[nodiscard]] std::string serviceNameAt(std::uint32_t row) const { return m_serviceName.get(row); }
    [[nodiscard]] Column<std::string>& serviceNameColumn() noexcept { return m_serviceName; }

private:
    Column<std::string> m_serviceName;
    CardRows<FluidStructureControlCard> m_cards;
};

} // namespace oor::model
