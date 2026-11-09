#pragma once

#include "oor/model/cards/nodes/moduleBoundaryNodes/ModuleBoundaryNodesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleBoundaryNodesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleBoundaryNodesCard& card);
    [[nodiscard]] static std::optional<ModuleBoundaryNodesCard> read(FieldReader& reader);
    static void write(const ModuleBoundaryNodesCard& card, FieldWriter& writer);
};

// All the MDBNDRY cards, category Other. A card has no ID of its own (IdSpace::None): a module may have several. They
// are kept whole next to a column of their MIDAs. The points are visited as grid points (`Node`, the ends of a THRU
// range `NodeInRange`) when MIDA is 0, the main Bulk Data; the modules are plain integers.
class ModuleBoundaryNodesTable final : public TypedTable<ModuleBoundaryNodesTable, ModuleBoundaryNodesCard>
{
public:
    static constexpr std::string_view NAME = "ModuleBoundaryNodes";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ModuleBoundaryNodesFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleBoundaryNodesCard&) noexcept { return 0; }
    std::uint32_t append(const ModuleBoundaryNodesCard& card, base::CardKey key);
    [[nodiscard]] ModuleBoundaryNodesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleBoundaryNodesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& moduleIdAColumn() noexcept { return m_moduleIdA; }

private:
    Column<base::EntityId> m_moduleIdA;
    CardRows<ModuleBoundaryNodesCard> m_cards;
};

} // namespace oor::model
