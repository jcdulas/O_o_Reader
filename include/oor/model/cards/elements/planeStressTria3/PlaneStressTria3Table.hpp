#pragma once

#include "oor/model/cards/elements/planeStressTria3/PlaneStressTria3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct PlaneStressTria3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlaneStressTria3Card& card);
    [[nodiscard]] static std::optional<PlaneStressTria3Card> read(FieldReader& reader);
    static void write(const PlaneStressTria3Card& card, FieldWriter& writer);
};

// All the CPLSTS3 elements: the nodes are one column of three IDs per element (an (N, 3) NumPy view without copy).
class PlaneStressTria3Table final : public TypedTable<PlaneStressTria3Table, PlaneStressTria3Card>
{
public:
    static constexpr std::string_view NAME = "PlaneStressTria3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = PlaneStressTria3Format;

    PlaneStressTria3Table()
        : m_cornerThickness({PlaneStressTria3Card::NO_THICKNESS, PlaneStressTria3Card::NO_THICKNESS,
                             PlaneStressTria3Card::NO_THICKNESS})
    {
    }

    [[nodiscard]] static base::EntityId cardId(const PlaneStressTria3Card& card) noexcept { return card.id; }
    std::uint32_t append(const PlaneStressTria3Card& card, base::CardKey key);
    [[nodiscard]] PlaneStressTria3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const PlaneStressTria3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] Column<double>& thetaInDegreesColumn() noexcept { return m_thetaInDegrees; }
    [[nodiscard]] Column<std::int32_t>& thicknessFlagColumn() noexcept { return m_thicknessFlag; }
    [[nodiscard]] Column<std::array<double, 3>>& cornerThicknessColumn() noexcept { return m_cornerThickness; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, 3>> m_nodes;
    Column<double> m_thetaInDegrees;
    Column<std::int32_t> m_thicknessFlag;
    Column<std::array<double, 3>> m_cornerThickness;
};

} // namespace oor::model
