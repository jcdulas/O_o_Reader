#pragma once

#include "oor/model/cards/thermal/heatSurfaceNodes/HeatSurfaceNodesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

struct HeatSurfaceNodesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatSurfaceNodesCard& card);
    [[nodiscard]] static std::optional<HeatSurfaceNodesCard> read(FieldReader& reader);
    static void write(const HeatSurfaceNodesCard& card, FieldWriter& writer);

    // True for a surface type of the card: REV, REV1, AREA3, AREA4, AREA6, AREA8.
    [[nodiscard]] static bool isSurfaceType(std::string_view type) noexcept;
    // How many of the first grid points a type needs (REV: 2, AREA3: 3, AREA6: 3, AREA8: 4, ...) and how many it can
    // have at most (REV: 3, AREA6: 6, AREA8: 8, ...). 0 for a word that is not a type.
    [[nodiscard]] static std::size_t requiredNodeCount(std::string_view type) noexcept;
    [[nodiscard]] static std::size_t maximumNodeCount(std::string_view type) noexcept;
};

// All the CHBDYG cards. The ID is a surface element ID, in the space the CHBDYE, CHBDYG and CHBDYP share
// (IdSpace::SurfaceElement, apart from the conduction elements); the surface element has the area of its face (see the
// geometry of the analysis layer).
class HeatSurfaceNodesTable final : public TypedTable<HeatSurfaceNodesTable, HeatSurfaceNodesCard>
{
public:
    static constexpr std::string_view NAME = "HeatSurfaceNodes";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::SurfaceElement};
    using Format = HeatSurfaceNodesFormat;

    [[nodiscard]] static base::EntityId cardId(const HeatSurfaceNodesCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatSurfaceNodesCard& card, base::CardKey key);
    [[nodiscard]] HeatSurfaceNodesCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatSurfaceNodesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // A blank VIEW or RADM number (0: no card has the ID 0) takes the default of the BDYOR of the model (format
    // reference BDYOR), wherever the BDYOR is in the deck. Model::applyDefaults runs it again for a BDYOR added later.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<base::EntityId>& frontViewIdColumn() noexcept { return m_frontViewId; }
    [[nodiscard]] Column<base::EntityId>& backViewIdColumn() noexcept { return m_backViewId; }
    [[nodiscard]] Column<base::EntityId>& frontRadiationPropIdColumn() noexcept { return m_frontRadiationPropId; }
    [[nodiscard]] Column<base::EntityId>& backRadiationPropIdColumn() noexcept { return m_backRadiationPropId; }
    [[nodiscard]] Column<std::array<base::EntityId, HeatSurfaceNodesCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_kind;
    Column<base::EntityId> m_frontViewId;
    Column<base::EntityId> m_backViewId;
    Column<base::EntityId> m_frontRadiationPropId;
    Column<base::EntityId> m_backRadiationPropId;
    Column<std::array<base::EntityId, HeatSurfaceNodesCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
