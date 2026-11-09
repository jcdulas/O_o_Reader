#pragma once

#include "oor/model/cards/nodes/moduleLabel/ModuleLabelCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ModuleLabelFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleLabelCard& card);
    [[nodiscard]] static std::optional<ModuleLabelCard> read(FieldReader& reader);
    static void write(const ModuleLabelCard& card, FieldWriter& writer);
};

// All the MDLABEL cards. The MID is the ID, unique among them (IdSpace::PerCardType, category Other: one label for each
// module). A module is not a card of the model, so the card names no other card. One column for each field.
class ModuleLabelTable final : public TypedTable<ModuleLabelTable, ModuleLabelCard>
{
public:
    static constexpr std::string_view NAME = "ModuleLabel";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleLabelFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleLabelCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleLabelCard& card, base::CardKey key);
    [[nodiscard]] ModuleLabelCard card(std::uint32_t row) const
    {
        return ModuleLabelCard{m_id.get(row), m_entryLabel.get(row)};
    }
    void store(std::uint32_t row, const ModuleLabelCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // modules are not cards: nothing to look up

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& entryLabelColumn() noexcept { return m_entryLabel; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_entryLabel;
};

} // namespace oor::model
