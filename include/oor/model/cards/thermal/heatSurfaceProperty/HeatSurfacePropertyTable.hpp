#pragma once

#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

struct HeatSurfacePropertyFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatSurfacePropertyCard& card);
    [[nodiscard]] static std::optional<HeatSurfacePropertyCard> read(FieldReader& reader);
    static void write(const HeatSurfacePropertyCard& card, FieldWriter& writer);

    // True for a surface type of the card: POINT, LINE, ELCYL, FTUBE, TUBE.
    [[nodiscard]] static bool isSurfaceType(std::string_view type) noexcept;
};

// All the CHBDYP cards. The ID is a surface element ID, in the space the CHBDYE, CHBDYG and CHBDYP share
// (IdSpace::SurfaceElement, apart from the conduction elements). PID names a PHBDY,
// so the column is called `heatSurfacePropId` for the query engine, which takes a `propId` column for a property of the
// structure.
class HeatSurfacePropertyTable final : public TypedTable<HeatSurfacePropertyTable, HeatSurfacePropertyCard>
{
public:
    static constexpr std::string_view NAME = "HeatSurfaceProperty";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::SurfaceElement};
    using Format = HeatSurfacePropertyFormat;

    HeatSurfacePropertyTable();

    [[nodiscard]] static base::EntityId cardId(const HeatSurfacePropertyCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatSurfacePropertyCard& card, base::CardKey key);
    [[nodiscard]] HeatSurfacePropertyCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatSurfacePropertyCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // A blank VIEW or RADM number (0: no card has the ID 0), and a blank GO, CE and orientation vector takes the
    // default of the BDYOR of the model (format reference BDYOR), wherever the BDYOR is in the deck.
    // Model::applyDefaults runs it again for a BDYOR added later.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& heatSurfacePropIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::string>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<base::EntityId>& frontViewIdColumn() noexcept { return m_frontViewId; }
    [[nodiscard]] Column<base::EntityId>& backViewIdColumn() noexcept { return m_backViewId; }
    [[nodiscard]] Column<base::EntityId>& node1Column() noexcept { return m_node1; }
    [[nodiscard]] Column<base::EntityId>& node2Column() noexcept { return m_node2; }
    [[nodiscard]] Column<base::EntityId>& orientationNodeColumn() noexcept { return m_orientationNode; }
    [[nodiscard]] Column<base::EntityId>& frontRadiationPropIdColumn() noexcept { return m_frontRadiationPropId; }
    [[nodiscard]] Column<base::EntityId>& backRadiationPropIdColumn() noexcept { return m_backRadiationPropId; }
    [[nodiscard]] Column<base::EntityId>& midsideNodeColumn() noexcept { return m_midsideNode; }
    [[nodiscard]] Column<base::EntityId>& directionFrameIdColumn() noexcept { return m_directionFrameId; }
    [[nodiscard]] Column<double>& direction1Column() noexcept { return m_direction1; }
    [[nodiscard]] Column<double>& direction2Column() noexcept { return m_direction2; }
    [[nodiscard]] Column<double>& direction3Column() noexcept { return m_direction3; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::string> m_kind;
    Column<base::EntityId> m_frontViewId;
    Column<base::EntityId> m_backViewId;
    Column<base::EntityId> m_node1;
    Column<base::EntityId> m_node2;
    Column<base::EntityId> m_orientationNode;
    Column<base::EntityId> m_frontRadiationPropId;
    Column<base::EntityId> m_backRadiationPropId;
    Column<base::EntityId> m_midsideNode;
    Column<base::EntityId> m_directionFrameId;
    Column<double> m_direction1;
    Column<double> m_direction2;
    Column<double> m_direction3;
};

} // namespace oor::model
