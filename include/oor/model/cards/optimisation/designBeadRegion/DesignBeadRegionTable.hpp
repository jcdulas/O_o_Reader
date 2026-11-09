#pragma once

#include "oor/model/cards/optimisation/designBeadRegion/DesignBeadRegionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignBeadRegionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignBeadRegionCard& card);
    [[nodiscard]] static std::optional<DesignBeadRegionCard> read(FieldReader& reader);
    static void write(const DesignBeadRegionCard& card, FieldWriter& writer);
};

// All the BEADVAR cards. The ID is unique among them (IdSpace::PerCardType, no category of its own: nothing names a
// topography region, as for the TOPVAR). The PID is a property (Property), the CID of the DESVAR line a coordinate
// system (Coordinate) and NGSET and DGSET sets of grid points (Set: a SET1). The cards are kept whole next to the
// columns the query engine reads: the ID and the property.
class DesignBeadRegionTable final : public TypedTable<DesignBeadRegionTable, DesignBeadRegionCard>
{
public:
    static constexpr std::string_view NAME = "DesignBeadRegion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = DesignBeadRegionFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignBeadRegionCard& card) noexcept { return card.id; }
    std::uint32_t append(const DesignBeadRegionCard& card, base::CardKey key);
    [[nodiscard]] DesignBeadRegionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignBeadRegionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    CardRows<DesignBeadRegionCard> m_cards;
};

} // namespace oor::model
