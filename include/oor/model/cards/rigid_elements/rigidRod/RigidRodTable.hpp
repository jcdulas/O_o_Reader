#pragma once

#include "oor/model/cards/rigid_elements/rigidRod/RigidRodCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct RigidRodFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidRodCard& card);
    [[nodiscard]] static std::optional<RigidRodCard> read(FieldReader& reader);
    static void write(const RigidRodCard& card, FieldWriter& writer);
};

// All the RROD cards. A rigid element is an element: its ID is in the ID space shared by all the elements.
class RigidRodTable final : public TypedTable<RigidRodTable, RigidRodCard>
{
public:
    static constexpr std::string_view NAME = "RigidRod";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidRodFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidRodCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidRodCard& card, base::CardKey key);
    [[nodiscard]] RigidRodCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidRodCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<std::int32_t>& componentsAtAColumn() noexcept { return m_componentsAtA; }
    [[nodiscard]] Column<std::int32_t>& componentsAtBColumn() noexcept { return m_componentsAtB; }
    [[nodiscard]] Column<double>& alphaCoefficientColumn() noexcept { return m_alphaCoefficient; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<std::int32_t> m_componentsAtA;
    Column<std::int32_t> m_componentsAtB;
    Column<double> m_alphaCoefficient;
    Column<double> m_referenceTemperature;
};

} // namespace oor::model
