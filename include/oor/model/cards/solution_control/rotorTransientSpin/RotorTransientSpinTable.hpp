#pragma once

#include "oor/model/cards/solution_control/rotorTransientSpin/RotorTransientSpinCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RotorTransientSpinFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RotorTransientSpinCard& card);
    [[nodiscard]] static std::optional<RotorTransientSpinCard> read(FieldReader& reader);
    static void write(const RotorTransientSpinCard& card, FieldWriter& writer);
};

// All the RSPINT cards (category Other, IdSpace::None: the pair of the rotor and its superelement identifies a card,
// no number does). The rotor (Rotor) and the two grid points (Node) are looked up when ROTRSEID is 0; the table of the
// spin rate (DynamicTable, a TABLED1) and the output point SPDOUT (Node: an EPOINT is a point of the node space) are
// always looked up. The cards are kept whole next to the two columns that identify them for the query engine.
class RotorTransientSpinTable final : public TypedTable<RotorTransientSpinTable, RotorTransientSpinCard>
{
public:
    static constexpr std::string_view NAME = "RotorTransientSpin";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = RotorTransientSpinFormat;

    [[nodiscard]] static base::EntityId cardId(const RotorTransientSpinCard&) noexcept { return 0; }
    std::uint32_t append(const RotorTransientSpinCard& card, base::CardKey key);
    [[nodiscard]] RotorTransientSpinCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RotorTransientSpinCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& rotorIdColumn() noexcept { return m_rotorId; }
    [[nodiscard]] Column<std::int32_t>& rotorSuperelementIdColumn() noexcept { return m_rotorSuperelementId; }

private:
    Column<base::EntityId> m_rotorId;
    Column<std::int32_t> m_rotorSuperelementId;
    CardRows<RotorTransientSpinCard> m_cards;
};

} // namespace oor::model
