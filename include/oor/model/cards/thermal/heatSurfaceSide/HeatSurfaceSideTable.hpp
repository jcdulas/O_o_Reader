#pragma once

#include "oor/model/cards/thermal/heatSurfaceSide/HeatSurfaceSideCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatSurfaceSideFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatSurfaceSideCard& card);
    [[nodiscard]] static std::optional<HeatSurfaceSideCard> read(FieldReader& reader);
    static void write(const HeatSurfaceSideCard& card, FieldWriter& writer);
};

// All the CHBDYE cards. The ID is a surface element ID, in the space the CHBDYE, CHBDYG and CHBDYP share
// (IdSpace::SurfaceElement, apart from the conduction elements); EID2, the conduction element, is a reference to an
// element.
class HeatSurfaceSideTable final : public TypedTable<HeatSurfaceSideTable, HeatSurfaceSideCard>
{
public:
    static constexpr std::string_view NAME = "HeatSurfaceSide";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::SurfaceElement};
    using Format = HeatSurfaceSideFormat;

    [[nodiscard]] static base::EntityId cardId(const HeatSurfaceSideCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatSurfaceSideCard& card, base::CardKey key);
    [[nodiscard]] HeatSurfaceSideCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatSurfaceSideCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // A blank VIEW or RADM number (0: no card has the ID 0) takes the default of the BDYOR of the model (format
    // reference BDYOR), wherever the BDYOR is in the deck. Model::applyDefaults runs it again for a BDYOR added later.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& conductionElemIdColumn() noexcept { return m_conductionElemId; }
    [[nodiscard]] Column<std::int32_t>& sideCodeColumn() noexcept { return m_sideCode; }
    [[nodiscard]] Column<base::EntityId>& frontViewIdColumn() noexcept { return m_frontViewId; }
    [[nodiscard]] Column<base::EntityId>& backViewIdColumn() noexcept { return m_backViewId; }
    [[nodiscard]] Column<base::EntityId>& frontRadiationPropIdColumn() noexcept { return m_frontRadiationPropId; }
    [[nodiscard]] Column<base::EntityId>& backRadiationPropIdColumn() noexcept { return m_backRadiationPropId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_conductionElemId;
    Column<std::int32_t> m_sideCode;
    Column<base::EntityId> m_frontViewId;
    Column<base::EntityId> m_backViewId;
    Column<base::EntityId> m_frontRadiationPropId;
    Column<base::EntityId> m_backRadiationPropId;
};

} // namespace oor::model
