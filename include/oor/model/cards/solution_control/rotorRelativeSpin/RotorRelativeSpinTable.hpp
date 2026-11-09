#pragma once

#include "oor/model/cards/solution_control/rotorRelativeSpin/RotorRelativeSpinCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RotorRelativeSpinFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RotorRelativeSpinCard& card);
    [[nodiscard]] static std::optional<RotorRelativeSpinCard> read(FieldReader& reader);
    static void write(const RotorRelativeSpinCard& card, FieldWriter& writer);
};

// All the RSPINR cards (category Other, IdSpace::None: the pair of the rotor and its superelement identifies a card,
// no number does). The rotor (Rotor) and the two grid points (Node) are looked up when ROTRSEID is 0, the SPTID when it
// is an integer (DiscreteValueSet). The cards are kept whole next to the two columns that identify them for the query
// engine.
class RotorRelativeSpinTable final : public TypedTable<RotorRelativeSpinTable, RotorRelativeSpinCard>
{
public:
    static constexpr std::string_view NAME = "RotorRelativeSpin";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = RotorRelativeSpinFormat;

    [[nodiscard]] static base::EntityId cardId(const RotorRelativeSpinCard&) noexcept { return 0; }
    std::uint32_t append(const RotorRelativeSpinCard& card, base::CardKey key);
    [[nodiscard]] RotorRelativeSpinCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RotorRelativeSpinCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& rotorIdColumn() noexcept { return m_rotorId; }
    [[nodiscard]] Column<std::int32_t>& rotorSuperelementIdColumn() noexcept { return m_rotorSuperelementId; }

private:
    Column<base::EntityId> m_rotorId;
    Column<std::int32_t> m_rotorSuperelementId;
    CardRows<RotorRelativeSpinCard> m_cards;
};

} // namespace oor::model
