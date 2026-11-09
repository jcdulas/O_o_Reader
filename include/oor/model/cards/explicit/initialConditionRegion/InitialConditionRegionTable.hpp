#pragma once

#include "oor/model/cards/explicit/initialConditionRegion/InitialConditionRegionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct InitialConditionRegionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const InitialConditionRegionCard& card);
    [[nodiscard]] static std::optional<InitialConditionRegionCard> read(FieldReader& reader);
    static void write(const InitialConditionRegionCard& card, FieldWriter& writer);
};

// All the TICREG cards. The cards of a group share their TSID, which is the ID of the table (IdSpace::Group, category
// EulerInitialRegions): a TICEUL1 names the group, not a card. The TRID is a number of the card only. The VID is
// visited with the target its TYPE selects (a shape, a BCBOX, a SET1), the MID is a material and the second TSID a
// TICVAL.
class InitialConditionRegionTable final : public TypedTable<InitialConditionRegionTable, InitialConditionRegionCard>
{
public:
    static constexpr std::string_view NAME = "InitialConditionRegion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EulerInitialRegions, base::IdSpace::Group};
    using Format = InitialConditionRegionFormat;

    [[nodiscard]] static base::EntityId cardId(const InitialConditionRegionCard& card) noexcept { return card.groupId; }
    std::uint32_t append(const InitialConditionRegionCard& card, base::CardKey key);
    [[nodiscard]] InitialConditionRegionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const InitialConditionRegionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_groupId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& groupIdColumn() noexcept { return m_groupId; }

private:
    Column<base::EntityId> m_groupId;
    CardRows<InitialConditionRegionCard> m_cards;
};

} // namespace oor::model
