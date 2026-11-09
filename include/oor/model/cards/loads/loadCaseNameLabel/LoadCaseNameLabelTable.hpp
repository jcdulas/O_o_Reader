#pragma once

#include "oor/model/cards/loads/loadCaseNameLabel/LoadCaseNameLabelCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct LoadCaseNameLabelFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const LoadCaseNameLabelCard& card);
    [[nodiscard]] static std::optional<LoadCaseNameLabelCard> read(FieldReader& reader);
    static void write(const LoadCaseNameLabelCard& card, FieldWriter& writer);
};

// All the LOADCNAM cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load): the LOAD Case
// Control command selects it. The card names no other card (the load names and the part names are those of the parts,
// which the model does not hold). The cards are kept whole next to a column of their LIDs: the items are a list of
// structures with two names.
class LoadCaseNameLabelTable final : public TypedTable<LoadCaseNameLabelTable, LoadCaseNameLabelCard>
{
public:
    static constexpr std::string_view NAME = "LoadCaseNameLabel";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = LoadCaseNameLabelFormat;

    [[nodiscard]] static base::EntityId cardId(const LoadCaseNameLabelCard& card) noexcept { return card.id; }
    std::uint32_t append(const LoadCaseNameLabelCard& card, base::CardKey key);
    [[nodiscard]] LoadCaseNameLabelCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const LoadCaseNameLabelCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<LoadCaseNameLabelCard> m_cards;
};

} // namespace oor::model
