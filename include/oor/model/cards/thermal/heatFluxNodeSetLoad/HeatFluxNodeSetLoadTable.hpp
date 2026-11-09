#pragma once

#include "oor/model/cards/thermal/heatFluxNodeSetLoad/HeatFluxNodeSetLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

struct HeatFluxNodeSetLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatFluxNodeSetLoadCard& card);
    [[nodiscard]] static std::optional<HeatFluxNodeSetLoadCard> read(FieldReader& reader);
    static void write(const HeatFluxNodeSetLoadCard& card, FieldWriter& writer);

    // True for a type of face: POINT, LINE, REV, AREA3, AREA4, AREA6, AREA8.
    [[nodiscard]] static bool isFlag(std::string_view flag) noexcept;
    // How many of the first grid points a face needs and how many it can have (0 for a word that is not a type).
    [[nodiscard]] static std::size_t requiredNodeCount(std::string_view flag) noexcept;
    [[nodiscard]] static std::size_t maximumNodeCount(std::string_view flag) noexcept;
};

// All the QHBDY cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load). The absent area
// factor is NaN.
class HeatFluxNodeSetLoadTable final : public TypedTable<HeatFluxNodeSetLoadTable, HeatFluxNodeSetLoadCard>
{
public:
    static constexpr std::string_view NAME = "HeatFluxNodeSetLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = HeatFluxNodeSetLoadFormat;

    HeatFluxNodeSetLoadTable();

    [[nodiscard]] static base::EntityId cardId(const HeatFluxNodeSetLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatFluxNodeSetLoadCard& card, base::CardKey key);
    [[nodiscard]] HeatFluxNodeSetLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatFluxNodeSetLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& faceKindColumn() noexcept { return m_faceKind; }
    [[nodiscard]] Column<double>& heatFluxColumn() noexcept { return m_heatFlux; }
    [[nodiscard]] Column<double>& areaColumn() noexcept { return m_area; }
    [[nodiscard]] Column<std::array<base::EntityId, HeatFluxNodeSetLoadCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_faceKind;
    Column<double> m_heatFlux;
    Column<double> m_area;
    Column<std::array<base::EntityId, HeatFluxNodeSetLoadCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
