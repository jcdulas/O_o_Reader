#pragma once

#include "oor/model/cards/explicit/couplingCohesiveFriction/CouplingCohesiveFrictionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CouplingCohesiveFrictionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CouplingCohesiveFrictionCard& card);
    [[nodiscard]] static std::optional<CouplingCohesiveFrictionCard> read(FieldReader& reader);
    static void write(const CouplingCohesiveFrictionCard& card, FieldWriter& writer);
};

// All the COUCOHF cards. The cards of a set share their COHFRID, which is the ID of the table (IdSpace::Group, category
// CohesiveFrictionSet): a COUPLE names the set, not a card. The CID is a number of the card only.
class CouplingCohesiveFrictionTable final
    : public TypedTable<CouplingCohesiveFrictionTable, CouplingCohesiveFrictionCard>
{
public:
    static constexpr std::string_view NAME = "CouplingCohesiveFriction";
    static inline const CardTypeInfo INFO{NAME, CardCategory::CohesiveFrictionSet, base::IdSpace::Group};
    using Format = CouplingCohesiveFrictionFormat;

    [[nodiscard]] static base::EntityId cardId(const CouplingCohesiveFrictionCard& card) noexcept { return card.setId; }
    std::uint32_t append(const CouplingCohesiveFrictionCard& card, base::CardKey key);
    [[nodiscard]] CouplingCohesiveFrictionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CouplingCohesiveFrictionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }

private:
    Column<base::EntityId> m_setId;
    CardRows<CouplingCohesiveFrictionCard> m_cards;
};

} // namespace oor::model
