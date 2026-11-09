#pragma once

#include "oor/model/cards/rigid_elements/rigidAxisymConnection/RigidAxisymConnectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RigidAxisymConnectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidAxisymConnectionCard& card);
    [[nodiscard]] static std::optional<RigidAxisymConnectionCard> read(FieldReader& reader);
    static void write(const RigidAxisymConnectionCard& card, FieldWriter& writer);
};

// All the RBAX3D cards. A rigid element is an element: its ID is in the space shared by all the elements. G3D and GAX
// are grid points (ReferenceTarget::Node).
class RigidAxisymConnectionTable final : public TypedTable<RigidAxisymConnectionTable, RigidAxisymConnectionCard>
{
public:
    static constexpr std::string_view NAME = "RigidAxisymConnection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidAxisymConnectionFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidAxisymConnectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidAxisymConnectionCard& card, base::CardKey key);
    [[nodiscard]] RigidAxisymConnectionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidAxisymConnectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // The two points of a card without copying the card (the geometry reads them for every element).
    [[nodiscard]] base::EntityId node3DAt(std::uint32_t row) const { return m_node3D.get(row); }
    [[nodiscard]] base::EntityId axisymNodeAt(std::uint32_t row) const { return m_axisymNode.get(row); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& node3DColumn() noexcept { return m_node3D; }
    [[nodiscard]] Column<base::EntityId>& axisymNodeColumn() noexcept { return m_axisymNode; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_node3D;
    Column<base::EntityId> m_axisymNode;
};

} // namespace oor::model
