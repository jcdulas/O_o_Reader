#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearBushLoad3D/NonlinearBushLoad3DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearBushLoad3DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearBushLoad3DCard& card);
    [[nodiscard]] static std::optional<NonlinearBushLoad3DCard> read(FieldReader& reader);
    static void write(const NonlinearBushLoad3DCard& card, FieldWriter& writer);
};

// All the NLBSH3D cards. The SID is a nonlinear load set ID shared by the NOLIN cards and the other nonlinear loads
// (IdSpace::Group, category DynamicLoad, as the NLRGAP): the NONLINEAR Case Control command selects it. GA and GB are
// grid points; the two LIDs name LBSH3DG cards (ReferenceTarget::BushingForceSet).
class NonlinearBushLoad3DTable final : public TypedTable<NonlinearBushLoad3DTable, NonlinearBushLoad3DCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearBushLoad3D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = NonlinearBushLoad3DFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearBushLoad3DCard& card) noexcept { return card.setId; }
    std::uint32_t append(const NonlinearBushLoad3DCard& card, base::CardKey key);
    [[nodiscard]] NonlinearBushLoad3DCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NonlinearBushLoad3DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_setId.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<base::EntityId>& loadId1Column() noexcept { return m_loadId1; }
    [[nodiscard]] Column<base::EntityId>& loadId2Column() noexcept { return m_loadId2; }

private:
    Column<base::EntityId> m_setId;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<base::EntityId> m_loadId1;
    Column<base::EntityId> m_loadId2;
};

} // namespace oor::model
