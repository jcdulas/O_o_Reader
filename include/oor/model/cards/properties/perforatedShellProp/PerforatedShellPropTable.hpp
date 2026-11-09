#pragma once

#include "oor/model/cards/properties/perforatedShellProp/PerforatedShellPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PerforatedShellPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PerforatedShellPropCard& card);
    [[nodiscard]] static std::optional<PerforatedShellPropCard> read(FieldReader& reader);
    static void write(const PerforatedShellPropCard& card, FieldWriter& writer);
};

// All the PSHLPF properties. A PSHLPF is a primary property (IdSpace::Property: the PID of a shell element finds it)
// and names a material, which a renumbering of the materials follows into. The cards are kept whole next to a column of
// their PIDs: they are few, and the query engine reads the ID.
class PerforatedShellPropTable final : public TypedTable<PerforatedShellPropTable, PerforatedShellPropCard>
{
public:
    static constexpr std::string_view NAME = "PerforatedShellProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = PerforatedShellPropFormat;

    [[nodiscard]] static base::EntityId cardId(const PerforatedShellPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const PerforatedShellPropCard& card, base::CardKey key);
    [[nodiscard]] PerforatedShellPropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PerforatedShellPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PerforatedShellPropCard> m_cards;
};

} // namespace oor::model
