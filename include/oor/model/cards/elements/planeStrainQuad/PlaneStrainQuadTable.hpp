#pragma once

#include "oor/model/cards/elements/planeStrainQuad/PlaneStrainQuadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct PlaneStrainQuadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlaneStrainQuadCard& card);
    [[nodiscard]] static std::optional<PlaneStrainQuadCard> read(FieldReader& reader);
    static void write(const PlaneStrainQuadCard& card, FieldWriter& writer);
};

// All the CQUAD elements: the nodes are one column of nine IDs per element, 0 for an absent edge point (an (N, 9)
// NumPy view without copy).
class PlaneStrainQuadTable final : public TypedTable<PlaneStrainQuadTable, PlaneStrainQuadCard>
{
public:
    static constexpr std::string_view NAME = "PlaneStrainQuad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = PlaneStrainQuadFormat;

    PlaneStrainQuadTable() : m_matFrameId(PlaneStrainQuadCard::NO_MAT_FRAME_ID) {}

    [[nodiscard]] static base::EntityId cardId(const PlaneStrainQuadCard& card) noexcept { return card.id; }
    std::uint32_t append(const PlaneStrainQuadCard& card, base::CardKey key);
    [[nodiscard]] PlaneStrainQuadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const PlaneStrainQuadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, PlaneStrainQuadCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }
    [[nodiscard]] Column<double>& thetaInDegreesColumn() noexcept { return m_thetaInDegrees; }
    [[nodiscard]] Column<base::EntityId>& matFrameIdColumn() noexcept { return m_matFrameId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, PlaneStrainQuadCard::NODE_COUNT>> m_nodes;
    Column<double> m_thetaInDegrees;
    Column<base::EntityId> m_matFrameId;
};

} // namespace oor::model
