#pragma once

#include "oor/model/cards/nodes/moduleFastener/ModuleFastenerCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleFastenerFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleFastenerCard& card);
    [[nodiscard]] static std::optional<ModuleFastenerCard> read(FieldReader& reader);
    static void write(const ModuleFastenerCard& card, FieldWriter& writer);
};

// All the MDFAST cards. The EID is an element number, which the rules of the CFAST want unique among all the
// elements; the table only keeps it unique among the MDFAST cards (IdSpace::PerCardType, category Other) and a
// validation check looks for a number that another kind of element shares. PID is a reference to a PFAST of the main
// Bulk Data; IDA and IDB are references to properties or elements (TYPE) in their module (the main Bulk Data, or the
// part of another module); GS, GA and GB are grid points of the main Bulk Data. The cards are kept whole
// next to their ID.
class ModuleFastenerTable final : public TypedTable<ModuleFastenerTable, ModuleFastenerCard>
{
public:
    static constexpr std::string_view NAME = "ModuleFastener";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleFastenerFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleFastenerCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleFastenerCard& card, base::CardKey key);
    [[nodiscard]] ModuleFastenerCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleFastenerCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleFastenerCard> m_cards;
};

} // namespace oor::model
