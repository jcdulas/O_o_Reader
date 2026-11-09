#pragma once

#include "oor/model/cards/rigid_elements/rigidBodyStar/RigidBodyStarCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct RigidBodyStarFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidBodyStarCard& card);
    [[nodiscard]] static std::optional<RigidBodyStarCard> read(FieldReader& reader);
    static void write(const RigidBodyStarCard& card, FieldWriter& writer);
};

// All the RBE2 cards. A rigid element is an element: its ID is in the ID space shared by all the elements. The
// dependent grid points of every card are one list in CSR form.
class RigidBodyStarTable final : public TypedTable<RigidBodyStarTable, RigidBodyStarCard>
{
public:
    static constexpr std::string_view NAME = "RigidBodyStar";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidBodyStarFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidBodyStarCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidBodyStarCard& card, base::CardKey key);
    [[nodiscard]] RigidBodyStarCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidBodyStarCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& independentColumn() noexcept { return m_independent; }
    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& dependentColumn() noexcept { return m_dependents; }
    [[nodiscard]] Column<double>& alphaCoefficientColumn() noexcept { return m_alphaCoefficient; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_independent;
    Column<std::int32_t> m_components;
    VariableLengthColumn<base::EntityId> m_dependents;
    Column<double> m_alphaCoefficient;
    Column<double> m_referenceTemperature;
};

} // namespace oor::model
