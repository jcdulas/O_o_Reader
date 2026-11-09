#pragma once

#include "oor/model/cards/explicit/occupantJoint/OccupantJointCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct OccupantJointFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const OccupantJointCard& card);
    [[nodiscard]] static std::optional<OccupantJointCard> read(FieldReader& reader);
    static void write(const OccupantJointCard& card, FieldWriter& writer);
};

// All the ATBJNT cards. The ID is unique among them (IdSpace::PerCardType, category Other: no other card names it). The
// grid points G0 to G7 are references to GRID cards; the element numbers and the name are not. The cards are kept whole
// next to their ID.
class OccupantJointTable final : public TypedTable<OccupantJointTable, OccupantJointCard>
{
public:
    static constexpr std::string_view NAME = "OccupantJoint";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = OccupantJointFormat;

    [[nodiscard]] static base::EntityId cardId(const OccupantJointCard& card) noexcept { return card.id; }
    std::uint32_t append(const OccupantJointCard& card, base::CardKey key);
    [[nodiscard]] OccupantJointCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const OccupantJointCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<OccupantJointCard> m_cards;
};

} // namespace oor::model
