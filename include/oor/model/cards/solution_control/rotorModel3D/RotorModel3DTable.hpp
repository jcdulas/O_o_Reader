#pragma once

#include "oor/model/cards/solution_control/rotorModel3D/RotorModel3DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RotorModel3DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RotorModel3DCard& card);
    [[nodiscard]] static std::optional<RotorModel3DCard> read(FieldReader& reader);
    static void write(const RotorModel3DCard& card, FieldWriter& writer);
};

// All the ROTOR cards. The ID is the rotor ID (category Rotor, IdSpace::Group, like ROTORG and ROTORAX). The elements
// are Element references (ElementInRange for the ends of a THRU range), the properties Property (PropertyInRange) and
// the AXIS grid points Node. The cards are kept whole next to the columns the query engine reads: the rotor ID and the
// frame.
class RotorModel3DTable final : public TypedTable<RotorModel3DTable, RotorModel3DCard>
{
public:
    static constexpr std::string_view NAME = "RotorModel3D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Rotor, base::IdSpace::Group};
    using Format = RotorModel3DFormat;

    [[nodiscard]] static base::EntityId cardId(const RotorModel3DCard& card) noexcept { return card.id; }
    std::uint32_t append(const RotorModel3DCard& card, base::CardKey key);
    [[nodiscard]] RotorModel3DCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RotorModel3DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& frameColumn() noexcept { return m_frame; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_frame;
    CardRows<RotorModel3DCard> m_cards;
};

} // namespace oor::model
