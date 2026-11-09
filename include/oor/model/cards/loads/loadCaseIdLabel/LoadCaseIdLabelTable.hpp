#pragma once

#include "oor/model/cards/loads/loadCaseIdLabel/LoadCaseIdLabelCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct LoadCaseIdLabelFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const LoadCaseIdLabelCard& card);
    [[nodiscard]] static std::optional<LoadCaseIdLabelCard> read(FieldReader& reader);
    static void write(const LoadCaseIdLabelCard& card, FieldWriter& writer);
};

// All the LOADCLID cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load): the LOAD Case
// Control command selects it. The card names no other card (the load IDs and the names are those of the parts, which
// the model does not hold). The cards are kept whole next to a column of their LIDs: the items are a list of
// structures with a name.
class LoadCaseIdLabelTable final : public TypedTable<LoadCaseIdLabelTable, LoadCaseIdLabelCard>
{
public:
    static constexpr std::string_view NAME = "LoadCaseIdLabel";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = LoadCaseIdLabelFormat;

    [[nodiscard]] static base::EntityId cardId(const LoadCaseIdLabelCard& card) noexcept { return card.id; }
    std::uint32_t append(const LoadCaseIdLabelCard& card, base::CardKey key);
    [[nodiscard]] LoadCaseIdLabelCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const LoadCaseIdLabelCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<LoadCaseIdLabelCard> m_cards;
};

} // namespace oor::model
