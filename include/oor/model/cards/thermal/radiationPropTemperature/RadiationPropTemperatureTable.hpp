#pragma once

#include "oor/model/cards/thermal/radiationPropTemperature/RadiationPropTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RadiationPropTemperatureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RadiationPropTemperatureCard& card);
    [[nodiscard]] static std::optional<RadiationPropTemperatureCard> read(FieldReader& reader);
    static void write(const RadiationPropTemperatureCard& card, FieldWriter& writer);
};

// All the RADMT cards. The ID space is the card type's own (IdSpace::PerCardType, category Other: the category
// RadiationMaterial is the RADM's, the card the surface elements name): the ID is the RADMID of the RADM that the card
// extends, so it is also a reference to a RADM (ReferenceTarget::RadiationMaterial: a renumbering of the RADM follows
// into it), and the tables are references to TABLEMj cards (ReferenceTarget::MaterialTable).
class RadiationPropTemperatureTable final
    : public TypedTable<RadiationPropTemperatureTable, RadiationPropTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "RadiationPropTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = RadiationPropTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const RadiationPropTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const RadiationPropTemperatureCard& card, base::CardKey key);
    [[nodiscard]] RadiationPropTemperatureCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RadiationPropTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& absorptivityTableIdColumn() noexcept { return m_absorptivityTableId; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& emissivityTableIdsColumn() noexcept
    {
        return m_emissivityTableIds;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_absorptivityTableId;
    VariableLengthColumn<base::EntityId> m_emissivityTableIds;
};

} // namespace oor::model
