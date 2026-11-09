#pragma once

#include "oor/model/cards/solution_control/cosimulationInputs/CosimulationInputsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CosimulationInputsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CosimulationInputsCard& card);
    [[nodiscard]] static std::optional<CosimulationInputsCard> read(FieldReader& reader);
    static void write(const CosimulationInputsCard& card, FieldWriter& writer);
};

// All the COSMINP cards, in the category CosimulationInput: the PHYIN is unique among them (IdSpace::PerCardType) and a
// COSMSRV names a card by it. The card names nothing itself. The cards are kept whole next to a column of their PHYINs.
class CosimulationInputsTable final : public TypedTable<CosimulationInputsTable, CosimulationInputsCard>
{
public:
    static constexpr std::string_view NAME = "CosimulationInputs";
    static inline const CardTypeInfo INFO{NAME, CardCategory::CosimulationInput, base::IdSpace::PerCardType};
    using Format = CosimulationInputsFormat;

    [[nodiscard]] static base::EntityId cardId(const CosimulationInputsCard& card) noexcept { return card.id; }
    std::uint32_t append(const CosimulationInputsCard& card, base::CardKey key);
    [[nodiscard]] CosimulationInputsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CosimulationInputsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CosimulationInputsCard> m_cards;
};

} // namespace oor::model
