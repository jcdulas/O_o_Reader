#pragma once

#include "oor/model/cards/elements/beamArbitrarySectionProp/BeamArbitrarySectionPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BeamArbitrarySectionPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BeamArbitrarySectionPropCard& card);
    [[nodiscard]] static std::optional<BeamArbitrarySectionPropCard> read(FieldReader& reader);
    static void write(const BeamArbitrarySectionPropCard& card, FieldWriter& writer);
};

// All the PBMSECT cards, kept whole next to a column of their PIDs. A PBMSECT is a property (IdSpace::Property): the
// PID of a CBEAM finds it. It names a material, the SET1 or SET3 cards of its perimeters and branches, and the PCOMP or
// PCOMPG cards of its core and layers.
class BeamArbitrarySectionPropTable final
    : public TypedTable<BeamArbitrarySectionPropTable, BeamArbitrarySectionPropCard>
{
public:
    static constexpr std::string_view NAME = "BeamArbitrarySectionProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BeamArbitrarySectionPropFormat;

    [[nodiscard]] static base::EntityId cardId(const BeamArbitrarySectionPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const BeamArbitrarySectionPropCard& card, base::CardKey key);
    [[nodiscard]] BeamArbitrarySectionPropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const BeamArbitrarySectionPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<BeamArbitrarySectionPropCard> m_cards;
};

} // namespace oor::model
