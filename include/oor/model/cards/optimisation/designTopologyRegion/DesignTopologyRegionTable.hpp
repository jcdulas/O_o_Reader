#pragma once

#include "oor/model/cards/optimisation/designTopologyRegion/DesignTopologyRegionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignTopologyRegionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignTopologyRegionCard& card);
    [[nodiscard]] static std::optional<DesignTopologyRegionCard> read(FieldReader& reader);
    static void write(const DesignTopologyRegionCard& card, FieldWriter& writer);
};

// All the TOPVAR cards. The ID is unique among them (IdSpace::PerCardType, no category of its own: nothing names a
// topology variable); the PID is a property and CID of a line a coordinate system.
class DesignTopologyRegionTable final : public TypedTable<DesignTopologyRegionTable, DesignTopologyRegionCard>
{
public:
    static constexpr std::string_view NAME = "DesignTopologyRegion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = DesignTopologyRegionFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignTopologyRegionCard& card) noexcept { return card.id; }
    std::uint32_t append(const DesignTopologyRegionCard& card, base::CardKey key);
    [[nodiscard]] DesignTopologyRegionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignTopologyRegionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignTopologyRegionCard> m_cards;
};

} // namespace oor::model
