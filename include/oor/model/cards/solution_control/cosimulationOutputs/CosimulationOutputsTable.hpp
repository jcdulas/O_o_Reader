#pragma once

#include "oor/model/cards/solution_control/cosimulationOutputs/CosimulationOutputsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CosimulationOutputsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CosimulationOutputsCard& card);
    [[nodiscard]] static std::optional<CosimulationOutputsCard> read(FieldReader& reader);
    static void write(const CosimulationOutputsCard& card, FieldWriter& writer);
};

// All the COSMOUT cards, in the category CosimulationOutput: the PHYOUT is unique among them (IdSpace::PerCardType) and
// a COSMSRV names a card by it. The card names nothing itself. The cards are kept whole next to a column of their
// PHYOUTs.
class CosimulationOutputsTable final : public TypedTable<CosimulationOutputsTable, CosimulationOutputsCard>
{
public:
    static constexpr std::string_view NAME = "CosimulationOutputs";
    static inline const CardTypeInfo INFO{NAME, CardCategory::CosimulationOutput, base::IdSpace::PerCardType};
    using Format = CosimulationOutputsFormat;

    [[nodiscard]] static base::EntityId cardId(const CosimulationOutputsCard& card) noexcept { return card.id; }
    std::uint32_t append(const CosimulationOutputsCard& card, base::CardKey key);
    [[nodiscard]] CosimulationOutputsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CosimulationOutputsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CosimulationOutputsCard> m_cards;
};

} // namespace oor::model
