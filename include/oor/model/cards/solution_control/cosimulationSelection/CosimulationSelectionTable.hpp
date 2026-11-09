#pragma once

#include "oor/model/cards/solution_control/cosimulationSelection/CosimulationSelectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CosimulationSelectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CosimulationSelectionCard& card);
    [[nodiscard]] static std::optional<CosimulationSelectionCard> read(FieldReader& reader);
    static void write(const CosimulationSelectionCard& card, FieldWriter& writer);
};

// All the COSMSEL cards, in the category Other: the SID is unique among them (IdSpace::PerCardType). It is selected by
// the Case Control command COSMSEL, which is text, so no reference names it. Each CSRVID is a reference to a COSMSRV
// (ReferenceTarget::CosimulationService), so a renumbering of the services follows into the list. The cards are kept
// whole next to a column of their SIDs.
class CosimulationSelectionTable final : public TypedTable<CosimulationSelectionTable, CosimulationSelectionCard>
{
public:
    static constexpr std::string_view NAME = "CosimulationSelection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = CosimulationSelectionFormat;

    [[nodiscard]] static base::EntityId cardId(const CosimulationSelectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const CosimulationSelectionCard& card, base::CardKey key);
    [[nodiscard]] CosimulationSelectionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CosimulationSelectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CosimulationSelectionCard> m_cards;
};

} // namespace oor::model
