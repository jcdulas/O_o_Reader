#pragma once

#include "oor/model/cards/thermal/heatSurfaceDefaults/HeatSurfaceDefaultsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatSurfaceDefaultsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatSurfaceDefaultsCard& card);
    [[nodiscard]] static std::optional<HeatSurfaceDefaultsCard> read(FieldReader& reader);
    static void write(const HeatSurfaceDefaultsCard& card, FieldWriter& writer);
};

// All the BDYOR cards (category Other). A card has no ID of its own (IdSpace::None); the format reference allows one in
// a deck, which is a rule between cards (the model's checks tell a second one). The references are the ones the CHBDYP
// has: the VIEW cards (ReferenceTarget::ViewFactorFaceFactor), the RADM cards (RadiationMaterial), the PHBDY
// (HeatBoundaryProperty), the orientation grid point (Node) and the system (Coordinate). The cards are kept whole next
// to a column of their PIDs, which is what the query engine reads.
class HeatSurfaceDefaultsTable final : public TypedTable<HeatSurfaceDefaultsTable, HeatSurfaceDefaultsCard>
{
public:
    static constexpr std::string_view NAME = "HeatSurfaceDefaults";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = HeatSurfaceDefaultsFormat;

    [[nodiscard]] static base::EntityId cardId(const HeatSurfaceDefaultsCard&) noexcept { return 0; }
    std::uint32_t append(const HeatSurfaceDefaultsCard& card, base::CardKey key);
    [[nodiscard]] HeatSurfaceDefaultsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const HeatSurfaceDefaultsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // Only one BDYOR may be in the Bulk Data: a second one is reported, the first is the one in force.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }

private:
    Column<base::EntityId> m_propId;
    CardRows<HeatSurfaceDefaultsCard> m_cards;
};

// The BDYOR of a model whose blank CHBDYE, CHBDYG and CHBDYP fields it gives defaults to: the first one that is alive
// (a second is reported by HeatSurfaceDefaultsTable::finishRead), nothing for a model that has none.
[[nodiscard]] std::optional<HeatSurfaceDefaultsCard> heatSurfaceDefaultsDefaults(const Model& model);

} // namespace oor::model
