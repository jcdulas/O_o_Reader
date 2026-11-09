#pragma once

#include "oor/model/cards/rigid_elements/rigidBodyMulti/RigidBodyMultiCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct RigidBodyMultiFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidBodyMultiCard& card);
    [[nodiscard]] static std::optional<RigidBodyMultiCard> read(FieldReader& reader);
    static void write(const RigidBodyMultiCard& card, FieldWriter& writer);
};

// All the RBE1 cards. A rigid element is an element: its ID is in the ID space shared by all the elements. The points
// and the components of the independent and of the dependent pairs are four lists in CSR form.
class RigidBodyMultiTable final : public TypedTable<RigidBodyMultiTable, RigidBodyMultiCard>
{
public:
    static constexpr std::string_view NAME = "RigidBodyMulti";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidBodyMultiFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidBodyMultiCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidBodyMultiCard& card, base::CardKey key);
    [[nodiscard]] RigidBodyMultiCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidBodyMultiCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& independentNodeColumn() noexcept { return m_independentNode; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& independentComponentsColumn() noexcept
    {
        return m_independentComponents;
    }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& dependentNodeColumn() noexcept { return m_dependentNode; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& dependentComponentsColumn() noexcept
    {
        return m_dependentComponents;
    }
    [[nodiscard]] Column<double>& alphaCoefficientColumn() noexcept { return m_alphaCoefficient; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_independentNode;
    VariableLengthColumn<std::int32_t> m_independentComponents;
    VariableLengthColumn<base::EntityId> m_dependentNode;
    VariableLengthColumn<std::int32_t> m_dependentComponents;
    Column<double> m_alphaCoefficient;
    Column<double> m_referenceTemperature;
};

} // namespace oor::model
