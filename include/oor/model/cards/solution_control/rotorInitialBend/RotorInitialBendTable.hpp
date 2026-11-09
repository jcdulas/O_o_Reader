#pragma once

#include "oor/model/cards/solution_control/rotorInitialBend/RotorInitialBendCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RotorInitialBendFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RotorInitialBendCard& card);
    [[nodiscard]] static std::optional<RotorInitialBendCard> read(FieldReader& reader);
    static void write(const RotorInitialBendCard& card, FieldWriter& writer);
};

// All the ROTBENT cards. The ID is unique among them (IdSpace::PerCardType, category Other: no card names a ROTBENT).
// The rotor is a Rotor reference, the load a LoadCombinationSet reference and the grid points of the BRGDPR rows Node
// references. The cards are kept whole next to the columns the query engine reads: the three numbers of the first line.
class RotorInitialBendTable final : public TypedTable<RotorInitialBendTable, RotorInitialBendCard>
{
public:
    static constexpr std::string_view NAME = "RotorInitialBend";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = RotorInitialBendFormat;

    [[nodiscard]] static base::EntityId cardId(const RotorInitialBendCard& card) noexcept { return card.id; }
    std::uint32_t append(const RotorInitialBendCard& card, base::CardKey key);
    [[nodiscard]] RotorInitialBendCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RotorInitialBendCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& rotorIdColumn() noexcept { return m_rotorId; }
    [[nodiscard]] Column<base::EntityId>& loadCombinationIdColumn() noexcept { return m_loadCombinationId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_rotorId;
    Column<base::EntityId> m_loadCombinationId;
    CardRows<RotorInitialBendCard> m_cards;
};

} // namespace oor::model
