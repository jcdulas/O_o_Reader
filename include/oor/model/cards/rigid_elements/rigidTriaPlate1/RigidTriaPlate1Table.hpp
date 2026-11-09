#pragma once

#include "oor/model/cards/rigid_elements/rigidTriaPlate1/RigidTriaPlate1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct RigidTriaPlate1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidTriaPlate1Card& card);
    [[nodiscard]] static std::optional<RigidTriaPlate1Card> read(FieldReader& reader);
    static void write(const RigidTriaPlate1Card& card, FieldWriter& writer);
};

// All the RTRPLT1 cards. A rigid element is an element: its ID is in the ID space shared by all the elements.
class RigidTriaPlate1Table final : public TypedTable<RigidTriaPlate1Table, RigidTriaPlate1Card>
{
public:
    static constexpr std::string_view NAME = "RigidTriaPlate1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidTriaPlate1Format;

    [[nodiscard]] static base::EntityId cardId(const RigidTriaPlate1Card& card) noexcept { return card.id; }
    std::uint32_t append(const RigidTriaPlate1Card& card, base::CardKey key);
    [[nodiscard]] RigidTriaPlate1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidTriaPlate1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] Column<std::array<std::int32_t, 2>>& dependentComponentsColumn() noexcept
    {
        return m_dependentComponents;
    }
    [[nodiscard]] Column<double>& alphaCoefficientColumn() noexcept { return m_alphaCoefficient; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, 3>> m_nodes;
    Column<std::array<std::int32_t, 2>> m_dependentComponents{std::array<std::int32_t, 2>{123456, 123456}};
    Column<double> m_alphaCoefficient;
    Column<double> m_referenceTemperature;
};

} // namespace oor::model
