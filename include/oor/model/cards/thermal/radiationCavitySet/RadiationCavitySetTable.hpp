#pragma once

#include "oor/model/cards/thermal/radiationCavitySet/RadiationCavitySetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RadiationCavitySetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RadiationCavitySetCard& card);
    [[nodiscard]] static std::optional<RadiationCavitySetCard> read(FieldReader& reader);
    static void write(const RadiationCavitySetCard& card, FieldWriter& writer);
};

// All the RADSET cards. A RADSET has no ID (IdSpace::None: the cards are one list of cavities between them); its
// cavities are references to the cards of the radiation cavities.
class RadiationCavitySetTable final : public TypedTable<RadiationCavitySetTable, RadiationCavitySetCard>
{
public:
    static constexpr std::string_view NAME = "RadiationCavitySet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = RadiationCavitySetFormat;

    [[nodiscard]] static base::EntityId cardId(const RadiationCavitySetCard&) noexcept { return 0; }
    std::uint32_t append(const RadiationCavitySetCard& card, base::CardKey key);
    [[nodiscard]] RadiationCavitySetCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RadiationCavitySetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] VariableLengthColumn<base::EntityId>& cavitiesColumn() noexcept { return m_cavities; }

private:
    VariableLengthColumn<base::EntityId> m_cavities;
};

} // namespace oor::model
