#pragma once

#include "oor/model/cards/elements/planeStrainTria3/PlaneStrainTria3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct PlaneStrainTria3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlaneStrainTria3Card& card);
    [[nodiscard]] static std::optional<PlaneStrainTria3Card> read(FieldReader& reader);
    static void write(const PlaneStrainTria3Card& card, FieldWriter& writer);
};

// All the CPLSTN3 elements: the nodes are one column of three IDs per element (an (N, 3) NumPy view without copy).
class PlaneStrainTria3Table final : public TypedTable<PlaneStrainTria3Table, PlaneStrainTria3Card>
{
public:
    static constexpr std::string_view NAME = "PlaneStrainTria3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = PlaneStrainTria3Format;

    [[nodiscard]] static base::EntityId cardId(const PlaneStrainTria3Card& card) noexcept { return card.id; }
    std::uint32_t append(const PlaneStrainTria3Card& card, base::CardKey key);
    [[nodiscard]] PlaneStrainTria3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const PlaneStrainTria3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] Column<double>& thetaInDegreesColumn() noexcept { return m_thetaInDegrees; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, 3>> m_nodes;
    Column<double> m_thetaInDegrees;
};

} // namespace oor::model
