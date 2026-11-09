#pragma once

#include "oor/model/cards/elements/planeStrainQuad4/PlaneStrainQuad4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct PlaneStrainQuad4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlaneStrainQuad4Card& card);
    [[nodiscard]] static std::optional<PlaneStrainQuad4Card> read(FieldReader& reader);
    static void write(const PlaneStrainQuad4Card& card, FieldWriter& writer);
};

// All the CPLSTN4 elements: the nodes are one column of four IDs per element (an (N, 4) NumPy view without copy).
class PlaneStrainQuad4Table final : public TypedTable<PlaneStrainQuad4Table, PlaneStrainQuad4Card>
{
public:
    static constexpr std::string_view NAME = "PlaneStrainQuad4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = PlaneStrainQuad4Format;

    [[nodiscard]] static base::EntityId cardId(const PlaneStrainQuad4Card& card) noexcept { return card.id; }
    std::uint32_t append(const PlaneStrainQuad4Card& card, base::CardKey key);
    [[nodiscard]] PlaneStrainQuad4Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const PlaneStrainQuad4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, 4>>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] Column<double>& thetaInDegreesColumn() noexcept { return m_thetaInDegrees; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, 4>> m_nodes;
    Column<double> m_thetaInDegrees;
};

} // namespace oor::model
