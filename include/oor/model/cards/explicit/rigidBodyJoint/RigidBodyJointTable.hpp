#pragma once

#include "oor/model/cards/explicit/rigidBodyJoint/RigidBodyJointCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RigidBodyJointFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidBodyJointCard& card);
    [[nodiscard]] static std::optional<RigidBodyJointCard> read(FieldReader& reader);
    static void write(const RigidBodyJointCard& card, FieldWriter& writer);
};

// All the RBJOINT cards. The ID is unique among them (IdSpace::PerCardType, category Other: no other card names a
// joint). The nodes are references to GRID cards. The cards are kept whole next to their ID.
class RigidBodyJointTable final : public TypedTable<RigidBodyJointTable, RigidBodyJointCard>
{
public:
    static constexpr std::string_view NAME = "RigidBodyJoint";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = RigidBodyJointFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidBodyJointCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidBodyJointCard& card, base::CardKey key);
    [[nodiscard]] RigidBodyJointCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RigidBodyJointCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<RigidBodyJointCard> m_cards;
};

} // namespace oor::model
