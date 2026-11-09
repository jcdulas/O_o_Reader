#pragma once

#include "oor/model/cards/solution_control/cosimulationArea/CosimulationAreaCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CosimulationAreaFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CosimulationAreaCard& card);
    [[nodiscard]] static std::optional<CosimulationAreaCard> read(FieldReader& reader);
    static void write(const CosimulationAreaCard& card, FieldWriter& writer);
};

// All the COSMGRP cards, in the category CosimulationGroup: the GRPID is unique among them (IdSpace::PerCardType) and a
// COSMSRV names a group by it. What the card names depends on its TYPE: the elements of the faces (SURFACE) and of a
// VOLUME are `Element` references (the two ends of a THRU range `ElementInRange`), the grid points of a POINT `Node`
// (`NodeInRange` for the ends). The cards are kept whole next to a column of their GRPIDs, because each of the three
// forms has lists of a length of its own.
class CosimulationAreaTable final : public TypedTable<CosimulationAreaTable, CosimulationAreaCard>
{
public:
    static constexpr std::string_view NAME = "CosimulationArea";
    static inline const CardTypeInfo INFO{NAME, CardCategory::CosimulationGroup, base::IdSpace::PerCardType};
    using Format = CosimulationAreaFormat;

    [[nodiscard]] static base::EntityId cardId(const CosimulationAreaCard& card) noexcept { return card.id; }
    std::uint32_t append(const CosimulationAreaCard& card, base::CardKey key);
    [[nodiscard]] CosimulationAreaCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CosimulationAreaCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CosimulationAreaCard> m_cards;
};

} // namespace oor::model
