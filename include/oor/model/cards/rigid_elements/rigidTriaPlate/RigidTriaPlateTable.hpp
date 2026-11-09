#pragma once

#include "oor/model/cards/rigid_elements/rigidTriaPlate/RigidTriaPlateCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct RigidTriaPlateFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidTriaPlateCard& card);
    [[nodiscard]] static std::optional<RigidTriaPlateCard> read(FieldReader& reader);
    static void write(const RigidTriaPlateCard& card, FieldWriter& writer);

    // Gives CNA its default when the three independent sets are blank (the reader and the Python constructors both
    // call it).
    static void resolveDefaults(RigidTriaPlateCard& card) noexcept;
};

// All the RTRPLT cards. A rigid element is an element: its ID is in the ID space shared by all the elements. The three
// grid points, the independent and the dependent components are three columns of three values.
class RigidTriaPlateTable final : public TypedTable<RigidTriaPlateTable, RigidTriaPlateCard>
{
public:
    static constexpr std::string_view NAME = "RigidTriaPlate";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidTriaPlateFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidTriaPlateCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidTriaPlateCard& card, base::CardKey key);
    [[nodiscard]] RigidTriaPlateCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidTriaPlateCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] Column<std::array<std::int32_t, 3>>& independentComponentsColumn() noexcept
    {
        return m_independentComponents;
    }
    [[nodiscard]] Column<std::array<std::int32_t, 3>>& dependentComponentsColumn() noexcept
    {
        return m_dependentComponents;
    }
    [[nodiscard]] Column<double>& alphaCoefficientColumn() noexcept { return m_alphaCoefficient; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, 3>> m_nodes;
    Column<std::array<std::int32_t, 3>> m_independentComponents;
    Column<std::array<std::int32_t, 3>> m_dependentComponents;
    Column<double> m_alphaCoefficient;
    Column<double> m_referenceTemperature;
};

} // namespace oor::model
