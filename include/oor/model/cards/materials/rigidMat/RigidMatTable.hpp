#pragma once

#include "oor/model/cards/materials/rigidMat/RigidMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RigidMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidMatCard& card);
    [[nodiscard]] static std::optional<RigidMatCard> read(FieldReader& reader);
    static void write(const RigidMatCard& card, FieldWriter& writer);
};

// All the MATRIG cards. The ID is a material ID, in the space every material type shares (IdSpace::Material): the
// properties of the elements of the rigid body name it. The cards are kept whole (CardRows: most of the fields are
// optional); the query engine reads the ID only. CID refers to a coordinate system.
class RigidMatTable final : public TypedTable<RigidMatTable, RigidMatCard>
{
public:
    static constexpr std::string_view NAME = "RigidMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = RigidMatFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidMatCard& card, base::CardKey key);
    [[nodiscard]] RigidMatCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RigidMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<RigidMatCard> m_cards;
};

} // namespace oor::model
