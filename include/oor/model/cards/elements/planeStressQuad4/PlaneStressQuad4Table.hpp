#pragma once

#include "oor/model/cards/elements/planeStressQuad4/PlaneStressQuad4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct PlaneStressQuad4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlaneStressQuad4Card& card);
    [[nodiscard]] static std::optional<PlaneStressQuad4Card> read(FieldReader& reader);
    static void write(const PlaneStressQuad4Card& card, FieldWriter& writer);
};

// All the CPLSTS4 elements: the nodes are one column of four IDs per element (an (N, 4) NumPy view without copy).
class PlaneStressQuad4Table final : public TypedTable<PlaneStressQuad4Table, PlaneStressQuad4Card>
{
public:
    static constexpr std::string_view NAME = "PlaneStressQuad4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = PlaneStressQuad4Format;

    PlaneStressQuad4Table()
        : m_cornerThickness({PlaneStressQuad4Card::NO_THICKNESS, PlaneStressQuad4Card::NO_THICKNESS,
                             PlaneStressQuad4Card::NO_THICKNESS, PlaneStressQuad4Card::NO_THICKNESS})
    {
    }

    [[nodiscard]] static base::EntityId cardId(const PlaneStressQuad4Card& card) noexcept { return card.id; }
    std::uint32_t append(const PlaneStressQuad4Card& card, base::CardKey key);
    [[nodiscard]] PlaneStressQuad4Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const PlaneStressQuad4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, 4>>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] Column<double>& thetaInDegreesColumn() noexcept { return m_thetaInDegrees; }
    [[nodiscard]] Column<std::int32_t>& thicknessFlagColumn() noexcept { return m_thicknessFlag; }
    [[nodiscard]] Column<std::array<double, 4>>& cornerThicknessColumn() noexcept { return m_cornerThickness; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, 4>> m_nodes;
    Column<double> m_thetaInDegrees;
    Column<std::int32_t> m_thicknessFlag;
    Column<std::array<double, 4>> m_cornerThickness;
};

} // namespace oor::model
