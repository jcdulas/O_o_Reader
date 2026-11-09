#pragma once

#include "oor/model/cards/solution_control/rotorLineNodes/RotorLineNodesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RotorLineNodesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RotorLineNodesCard& card);
    [[nodiscard]] static std::optional<RotorLineNodesCard> read(FieldReader& reader);
    static void write(const RotorLineNodesCard& card, FieldWriter& writer);
};

// All the ROTORG cards. The ID is the rotor ID shared by the ROTORG cards of a rotor (IdSpace::Group), in the category
// Rotor that the other rotor cards (ROTOR, ROTORAX, ROTORSE) share: the RSPINR, RSPINT, REFROT, REFRTSE, ROTBENT,
// ROTHYBD and RGYRO cards name a rotor by it. The grids are nodes: a single grid is a Node reference, the two ends of
// a run are NodeInRange (the grids between them keep their numbers). The three members of a run are one list each, in
// CSR form.
class RotorLineNodesTable final : public TypedTable<RotorLineNodesTable, RotorLineNodesCard>
{
public:
    static constexpr std::string_view NAME = "RotorLineNodes";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Rotor, base::IdSpace::Group};
    using Format = RotorLineNodesFormat;

    [[nodiscard]] static base::EntityId cardId(const RotorLineNodesCard& card) noexcept { return card.id; }
    std::uint32_t append(const RotorLineNodesCard& card, base::CardKey key);
    [[nodiscard]] RotorLineNodesCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RotorLineNodesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& firstColumn() noexcept { return m_first; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& lastColumn() noexcept { return m_last; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& incrementColumn() noexcept { return m_increment; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_first;
    VariableLengthColumn<base::EntityId> m_last;
    VariableLengthColumn<base::EntityId> m_increment;
};

} // namespace oor::model
