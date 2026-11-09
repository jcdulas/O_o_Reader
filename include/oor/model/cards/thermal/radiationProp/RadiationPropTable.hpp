#pragma once

#include "oor/model/cards/thermal/radiationProp/RadiationPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RadiationPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RadiationPropCard& card);
    [[nodiscard]] static std::optional<RadiationPropCard> read(FieldReader& reader);
    static void write(const RadiationPropCard& card, FieldWriter& writer);
};

// All the RADM cards. The ID is the RADMID the surface elements name, in a space of its own (category
// RadiationMaterial, IdSpace::PerCardType). The emissivities are one list in CSR form; an absent absorptivity is NaN.
class RadiationPropTable final : public TypedTable<RadiationPropTable, RadiationPropCard>
{
public:
    static constexpr std::string_view NAME = "RadiationProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::RadiationMaterial, base::IdSpace::PerCardType};
    using Format = RadiationPropFormat;

    RadiationPropTable();

    [[nodiscard]] static base::EntityId cardId(const RadiationPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const RadiationPropCard& card, base::CardKey key);
    [[nodiscard]] RadiationPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RadiationPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a RADM refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& absorptivityColumn() noexcept { return m_absorptivity; }
    [[nodiscard]] VariableLengthColumn<double>& emissivityColumn() noexcept { return m_emissivity; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_absorptivity;
    VariableLengthColumn<double> m_emissivity;
};

} // namespace oor::model
