#pragma once

#include "oor/model/cards/optimisation/designTopometryRegion/DesignTopometryRegionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignTopometryRegionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignTopometryRegionCard& card);
    [[nodiscard]] static std::optional<DesignTopometryRegionCard> read(FieldReader& reader);
    static void write(const DesignTopometryRegionCard& card, FieldWriter& writer);
};

// All the TOMVAR cards. The ID is unique among them (IdSpace::PerCardType, no category of its own: the TID of a DLINK
// line names another TOMVAR by its number, which stays a plain ID since nothing but this line names a TOMVAR). The PID
// and the SYMPIDs are properties (Property), the CID of the SYM line a coordinate system (Coordinate) and the DSVID of
// the DDVAL line a DDVAL (DiscreteValueSet). The cards are kept whole next to the columns the query engine reads: the
// ID and the property.
class DesignTopometryRegionTable final : public TypedTable<DesignTopometryRegionTable, DesignTopometryRegionCard>
{
public:
    static constexpr std::string_view NAME = "DesignTopometryRegion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = DesignTopometryRegionFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignTopometryRegionCard& card) noexcept { return card.id; }
    std::uint32_t append(const DesignTopometryRegionCard& card, base::CardKey key);
    [[nodiscard]] DesignTopometryRegionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignTopometryRegionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    CardRows<DesignTopometryRegionCard> m_cards;
};

} // namespace oor::model
