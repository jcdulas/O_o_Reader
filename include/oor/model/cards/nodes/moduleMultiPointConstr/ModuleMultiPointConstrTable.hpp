#pragma once

#include "oor/model/cards/nodes/moduleMultiPointConstr/ModuleMultiPointConstrCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleMultiPointConstrFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleMultiPointConstrCard& card);
    [[nodiscard]] static std::optional<ModuleMultiPointConstrCard> read(FieldReader& reader);
    static void write(const ModuleMultiPointConstrCard& card, FieldWriter& writer);
};

// All the MDMPC cards. The SID is a constraint set ID shared by many cards (IdSpace::Group, category Constraint): the
// Case Control MPC command selects it, together with the MPC cards of the same number. A point is a reference to a grid
// or scalar point in its module: the main Bulk Data, or the part of another module. The cards are kept
// whole next to their ID.
class ModuleMultiPointConstrTable final : public TypedTable<ModuleMultiPointConstrTable, ModuleMultiPointConstrCard>
{
public:
    static constexpr std::string_view NAME = "ModuleMultiPointConstr";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = ModuleMultiPointConstrFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleMultiPointConstrCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleMultiPointConstrCard& card, base::CardKey key);
    [[nodiscard]] ModuleMultiPointConstrCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleMultiPointConstrCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleMultiPointConstrCard> m_cards;
};

} // namespace oor::model
