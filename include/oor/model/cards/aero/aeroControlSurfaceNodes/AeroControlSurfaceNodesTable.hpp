#pragma once

#include "oor/model/cards/aero/aeroControlSurfaceNodes/AeroControlSurfaceNodesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroControlSurfaceNodesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroControlSurfaceNodesCard& card);
    [[nodiscard]] static std::optional<AeroControlSurfaceNodesCard> read(FieldReader& reader);
    static void write(const AeroControlSurfaceNodesCard& card, FieldWriter& writer);
};

// All the AESURFS cards, in the category Aero. The ID is ignored by the solver, so the space is IdSpace::Group (several
// cards may carry the same number). LIST1 and LIST2 are SET1 cards (ReferenceTarget::Set); the label names an AESURF
// but is not a reference. The card is kept whole next to a column of its ID.
class AeroControlSurfaceNodesTable final : public TypedTable<AeroControlSurfaceNodesTable, AeroControlSurfaceNodesCard>
{
public:
    static constexpr std::string_view NAME = "AeroControlSurfaceNodes";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::Group};
    using Format = AeroControlSurfaceNodesFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroControlSurfaceNodesCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroControlSurfaceNodesCard& card, base::CardKey key);
    [[nodiscard]] AeroControlSurfaceNodesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroControlSurfaceNodesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroControlSurfaceNodesCard> m_cards;
};

} // namespace oor::model
