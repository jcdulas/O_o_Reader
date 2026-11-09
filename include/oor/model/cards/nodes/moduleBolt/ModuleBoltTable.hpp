#pragma once

#include "oor/model/cards/nodes/moduleBolt/ModuleBoltCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleBoltFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleBoltCard& card);
    [[nodiscard]] static std::optional<ModuleBoltCard> read(FieldReader& reader);
    static void write(const ModuleBoltCard& card, FieldWriter& writer);
};

// All the MDBOLT cards. The ID (the element ID of the bolt) is unique among them (IdSpace::PerCardType, category Other:
// it is an element of the multi-module family, not of the model's element space). GRIDC and the grid points of the two
// lists are looked up as grid points (`Node`) when their module (MODC, MODT, MODB) is 0; the modules are plain
// integers. The cards are kept whole next to their ID.
class ModuleBoltTable final : public TypedTable<ModuleBoltTable, ModuleBoltCard>
{
public:
    static constexpr std::string_view NAME = "ModuleBolt";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleBoltFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleBoltCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleBoltCard& card, base::CardKey key);
    [[nodiscard]] ModuleBoltCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleBoltCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleBoltCard> m_cards;
};

} // namespace oor::model
