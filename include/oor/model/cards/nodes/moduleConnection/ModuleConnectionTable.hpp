#pragma once

#include "oor/model/cards/nodes/moduleConnection/ModuleConnectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleConnectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleConnectionCard& card);
    [[nodiscard]] static std::optional<ModuleConnectionCard> read(FieldReader& reader);
    static void write(const ModuleConnectionCard& card, FieldWriter& writer);
};

// All the MDCONCT cards. The BID is the ID, unique among them (IdSpace::PerCardType, category Other). CID is a
// coordinate system of module 0 (`Coordinate`); GRID, and the points of the pairs whose module is 0, are looked up in
// this model (`Node`, and `ScalarOrGridPoint` for the pairs, which may name a scalar point); the points of another
// module are plain IDs. The cards are kept whole next to their ID.
class ModuleConnectionTable final : public TypedTable<ModuleConnectionTable, ModuleConnectionCard>
{
public:
    static constexpr std::string_view NAME = "ModuleConnection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleConnectionFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleConnectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleConnectionCard& card, base::CardKey key);
    [[nodiscard]] ModuleConnectionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleConnectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleConnectionCard> m_cards;
};

} // namespace oor::model
