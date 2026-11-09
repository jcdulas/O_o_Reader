#pragma once

#include "oor/model/cards/rigid_elements/rigidBar1/RigidBar1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct RigidBar1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidBar1Card& card);
    [[nodiscard]] static std::optional<RigidBar1Card> read(FieldReader& reader);
    static void write(const RigidBar1Card& card, FieldWriter& writer);
};

// All the RBAR1 cards. A rigid element is an element: its ID is in the ID space shared by all the elements.
class RigidBar1Table final : public TypedTable<RigidBar1Table, RigidBar1Card>
{
public:
    static constexpr std::string_view NAME = "RigidBar1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidBar1Format;

    [[nodiscard]] static base::EntityId cardId(const RigidBar1Card& card) noexcept { return card.id; }
    std::uint32_t append(const RigidBar1Card& card, base::CardKey key);
    [[nodiscard]] RigidBar1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidBar1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<std::int32_t>& componentsBColumn() noexcept { return m_componentsB; }
    [[nodiscard]] Column<double>& alphaCoefficientColumn() noexcept { return m_alphaCoefficient; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<std::int32_t> m_componentsB{123456};
    Column<double> m_alphaCoefficient;
    Column<double> m_referenceTemperature;
};

} // namespace oor::model
