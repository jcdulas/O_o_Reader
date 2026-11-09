#pragma once

#include "oor/model/cards/explicit/eulerForceRegion/EulerForceRegionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerForceRegionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerForceRegionCard& card);
    [[nodiscard]] static std::optional<EulerForceRegionCard> read(FieldReader& reader);
    static void write(const EulerForceRegionCard& card, FieldWriter& writer);
};

// All the EULFREG cards. The cards of a set share their ESID, which is the ID of the table (IdSpace::Group, category
// EulerRegionSet): an EULFOR1 names the set, not a card. The ERID is a number of the card only. The VID is visited with
// the target its TYPE selects (a shape, a BCBOX, a SET1), the MID is a material and the three ACC fields are tables.
class EulerForceRegionTable final : public TypedTable<EulerForceRegionTable, EulerForceRegionCard>
{
public:
    static constexpr std::string_view NAME = "EulerForceRegion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EulerRegionSet, base::IdSpace::Group};
    using Format = EulerForceRegionFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerForceRegionCard& card) noexcept { return card.setId; }
    std::uint32_t append(const EulerForceRegionCard& card, base::CardKey key);
    [[nodiscard]] EulerForceRegionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerForceRegionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }

private:
    Column<base::EntityId> m_setId;
    CardRows<EulerForceRegionCard> m_cards;
};

} // namespace oor::model
