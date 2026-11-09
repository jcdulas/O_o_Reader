#pragma once

#include "oor/model/cards/rigid_elements/rigidJoint/RigidJointCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct RigidJointFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidJointCard& card);
    [[nodiscard]] static std::optional<RigidJointCard> read(FieldReader& reader);
    static void write(const RigidJointCard& card, FieldWriter& writer);
};

// All the RJOINT cards. A rigid element is an element: its ID is in the ID space shared by all the elements.
class RigidJointTable final : public TypedTable<RigidJointTable, RigidJointCard>
{
public:
    static constexpr std::string_view NAME = "RigidJoint";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidJointFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidJointCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidJointCard& card, base::CardKey key);
    [[nodiscard]] RigidJointCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidJointCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<std::int32_t>& componentsBColumn() noexcept { return m_componentsB; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<std::int32_t> m_componentsB{123456};
};

} // namespace oor::model
