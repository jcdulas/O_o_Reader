#pragma once

#include "oor/model/cards/nodes/moduleSeam/ModuleSeamCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleSeamFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleSeamCard& card);
    [[nodiscard]] static std::optional<ModuleSeamCard> read(FieldReader& reader);
    static void write(const ModuleSeamCard& card, FieldWriter& writer);
};

// All the MDSEAM cards. The EID is an element number, which the rules of the CSEAM want unique among all the
// elements; the table only keeps it unique among the MDSEAM cards (IdSpace::PerCardType, category Other) and a
// validation check looks for a number that another kind of element shares. PID is a reference to a PSEAM of the main
// Bulk Data; the patch numbers are references to properties (PSHELL) or elements (ELEM) when their module is the main
// Bulk Data, and are not looked up in another module; GS and GE are grid points of the main Bulk Data. The cards are
// kept whole next to their ID.
class ModuleSeamTable final : public TypedTable<ModuleSeamTable, ModuleSeamCard>
{
public:
    static constexpr std::string_view NAME = "ModuleSeam";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleSeamFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleSeamCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleSeamCard& card, base::CardKey key);
    [[nodiscard]] ModuleSeamCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleSeamCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleSeamCard> m_cards;
};

} // namespace oor::model
