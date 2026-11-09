#pragma once

#include "oor/model/cards/explicit/eulerHydrostaticPreset/EulerHydrostaticPresetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerHydrostaticPresetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerHydrostaticPresetCard& card);
    [[nodiscard]] static std::optional<EulerHydrostaticPresetCard> read(FieldReader& reader);
    static void write(const EulerHydrostaticPresetCard& card, FieldWriter& writer);
};

// All the HYDSTAT cards. The HID is unique among the HYDSTAT cards (IdSpace::PerCardType) and is in the category
// HydrostaticPreset, which a COUPLE names. Its MID is a material, its GID a grid point and its CID a coordinate system.
class EulerHydrostaticPresetTable final : public TypedTable<EulerHydrostaticPresetTable, EulerHydrostaticPresetCard>
{
public:
    static constexpr std::string_view NAME = "EulerHydrostaticPreset";
    static inline const CardTypeInfo INFO{NAME, CardCategory::HydrostaticPreset, base::IdSpace::PerCardType};
    using Format = EulerHydrostaticPresetFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerHydrostaticPresetCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerHydrostaticPresetCard& card, base::CardKey key);
    [[nodiscard]] EulerHydrostaticPresetCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerHydrostaticPresetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerHydrostaticPresetCard> m_cards;
};

} // namespace oor::model
