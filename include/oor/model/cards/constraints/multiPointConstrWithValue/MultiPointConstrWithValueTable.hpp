#pragma once

#include "oor/model/cards/constraints/multiPointConstrWithValue/MultiPointConstrWithValueCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct MultiPointConstrWithValueFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MultiPointConstrWithValueCard& card);
    [[nodiscard]] static std::optional<MultiPointConstrWithValueCard> read(FieldReader& reader);
    static void write(const MultiPointConstrWithValueCard& card, FieldWriter& writer);
};

// All the MPCY cards. The ID is a constraint set ID shared by many cards (IdSpace::Group): the MPC Case Control command
// selects the set, together with the MPC cards that have the same ID. The dependent term is five columns; the
// independent terms of every card are three lists (points, components, coefficients) in CSR form. The points are
// references to grid or scalar points (a scalar point that no SPOINT defines is allowed).
class MultiPointConstrWithValueTable final
    : public TypedTable<MultiPointConstrWithValueTable, MultiPointConstrWithValueCard>
{
public:
    static constexpr std::string_view NAME = "MultiPointConstrWithValue";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = MultiPointConstrWithValueFormat;

    [[nodiscard]] static base::EntityId cardId(const MultiPointConstrWithValueCard& card) noexcept { return card.id; }
    std::uint32_t append(const MultiPointConstrWithValueCard& card, base::CardKey key);
    [[nodiscard]] MultiPointConstrWithValueCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const MultiPointConstrWithValueCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& dependentNodeColumn() noexcept { return m_dependentNode; }
    [[nodiscard]] Column<std::int32_t>& dependentComponentsColumn() noexcept { return m_dependentComponents; }
    [[nodiscard]] Column<double>& dependentCoefficientColumn() noexcept { return m_dependentCoefficient; }
    [[nodiscard]] Column<double>& rightHandSideColumn() noexcept { return m_rightHandSide; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentColumn() noexcept { return m_component; }
    [[nodiscard]] VariableLengthColumn<double>& coefficientColumn() noexcept { return m_coefficient; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_dependentNode;
    Column<std::int32_t> m_dependentComponents;
    Column<double> m_dependentCoefficient;
    Column<double> m_rightHandSide;
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<std::int32_t> m_component;
    VariableLengthColumn<double> m_coefficient;
};

} // namespace oor::model
