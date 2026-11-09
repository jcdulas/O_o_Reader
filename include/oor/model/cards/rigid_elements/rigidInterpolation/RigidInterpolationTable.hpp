#pragma once

#include "oor/model/cards/rigid_elements/rigidInterpolation/RigidInterpolationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct RigidInterpolationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidInterpolationCard& card);
    [[nodiscard]] static std::optional<RigidInterpolationCard> read(FieldReader& reader);
    static void write(const RigidInterpolationCard& card, FieldWriter& writer);
};

// All the RBE3 cards. A rigid element is an element: its ID is in the ID space shared by all the elements. The groups
// of every card are four lists in CSR form (weights, components, number of nodes, and all the nodes one group after the
// other); the UM list is two more.
class RigidInterpolationTable final : public TypedTable<RigidInterpolationTable, RigidInterpolationCard>
{
public:
    static constexpr std::string_view NAME = "RigidInterpolation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidInterpolationFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidInterpolationCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidInterpolationCard& card, base::CardKey key);
    [[nodiscard]] RigidInterpolationCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidInterpolationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& referenceNodeColumn() noexcept { return m_referenceNode; }
    [[nodiscard]] Column<std::int32_t>& refComponentsColumn() noexcept { return m_refComponents; }
    [[nodiscard]] VariableLengthColumn<double>& groupWeightColumn() noexcept { return m_groupWeight; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& groupComponentsColumn() noexcept { return m_groupComponents; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& groupSizeColumn() noexcept { return m_groupSize; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& groupNodeColumn() noexcept { return m_groupNodes; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& dependentNodeColumn() noexcept { return m_dependentNodes; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& dependentComponentsColumn() noexcept
    {
        return m_dependentComponents;
    }
    [[nodiscard]] Column<double>& alphaCoefficientColumn() noexcept { return m_alphaCoefficient; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_referenceNode;
    Column<std::int32_t> m_refComponents;
    VariableLengthColumn<double> m_groupWeight;
    VariableLengthColumn<std::int32_t> m_groupComponents;
    VariableLengthColumn<std::int32_t> m_groupSize;
    VariableLengthColumn<base::EntityId> m_groupNodes;
    VariableLengthColumn<base::EntityId> m_dependentNodes;
    VariableLengthColumn<std::int32_t> m_dependentComponents;
    Column<double> m_alphaCoefficient;
    Column<double> m_referenceTemperature;
};

} // namespace oor::model
