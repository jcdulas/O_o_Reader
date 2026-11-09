#pragma once

#include "oor/model/cards/nodes/moduleWeld/ModuleWeldCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleWeldFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleWeldCard& card);
    [[nodiscard]] static std::optional<ModuleWeldCard> read(FieldReader& reader);
    static void write(const ModuleWeldCard& card, FieldWriter& writer);
};

// All the MDWELD cards. The EWID is an element number, which the rules of the CWELD want unique among all the
// elements; the table only keeps it unique among the MDWELD cards (IdSpace::PerCardType, category Other) and a
// validation check looks for a number that another kind of element shares. PWID is a reference to a PWELD of the main
// Bulk Data; the patches (PSHELL properties or shell elements), the vertex points of an ALIGN, the point GS of an
// ELEMID and the lists of a GRIDID are references when their module is the main Bulk Data and are not looked up in
// another module; the other grid points are points of the main Bulk Data. The cards are kept whole next to their ID.
class ModuleWeldTable final : public TypedTable<ModuleWeldTable, ModuleWeldCard>
{
public:
    static constexpr std::string_view NAME = "ModuleWeld";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleWeldFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleWeldCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleWeldCard& card, base::CardKey key);
    [[nodiscard]] ModuleWeldCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleWeldCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleWeldCard> m_cards;
};

} // namespace oor::model
