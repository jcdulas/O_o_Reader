#pragma once

#include "oor/model/cards/nodes/moduleExcludedNodes/ModuleExcludedNodesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleExcludedNodesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleExcludedNodesCard& card);
    [[nodiscard]] static std::optional<ModuleExcludedNodesCard> read(FieldReader& reader);
    static void write(const ModuleExcludedNodesCard& card, FieldWriter& writer);
};

// All the MDEXCLD cards, category Other. A card has no ID of its own (IdSpace::None): a module may have several. They
// are kept whole next to a column of their MIDAs. The points are visited as grid points (`Node`, the ends of a THRU
// range `NodeInRange`) when MIDA is 0, the main Bulk Data; the modules are plain integers.
class ModuleExcludedNodesTable final : public TypedTable<ModuleExcludedNodesTable, ModuleExcludedNodesCard>
{
public:
    static constexpr std::string_view NAME = "ModuleExcludedNodes";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ModuleExcludedNodesFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleExcludedNodesCard&) noexcept { return 0; }
    std::uint32_t append(const ModuleExcludedNodesCard& card, base::CardKey key);
    [[nodiscard]] ModuleExcludedNodesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleExcludedNodesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& moduleIdAColumn() noexcept { return m_moduleIdA; }

private:
    Column<base::EntityId> m_moduleIdA;
    CardRows<ModuleExcludedNodesCard> m_cards;
};

} // namespace oor::model
