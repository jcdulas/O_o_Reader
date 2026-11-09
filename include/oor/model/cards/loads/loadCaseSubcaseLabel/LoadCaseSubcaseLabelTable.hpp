#pragma once

#include "oor/model/cards/loads/loadCaseSubcaseLabel/LoadCaseSubcaseLabelCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct LoadCaseSubcaseLabelFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const LoadCaseSubcaseLabelCard& card);
    [[nodiscard]] static std::optional<LoadCaseSubcaseLabelCard> read(FieldReader& reader);
    static void write(const LoadCaseSubcaseLabelCard& card, FieldWriter& writer);
};

// All the LOADCSUB cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load): the LOAD Case
// Control command selects it. The card names no other card (the subcase numbers and the names are those of the parts,
// which the model does not hold). The cards are kept whole next to a column of their LIDs: the items are a list of
// structures with a name.
class LoadCaseSubcaseLabelTable final : public TypedTable<LoadCaseSubcaseLabelTable, LoadCaseSubcaseLabelCard>
{
public:
    static constexpr std::string_view NAME = "LoadCaseSubcaseLabel";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = LoadCaseSubcaseLabelFormat;

    [[nodiscard]] static base::EntityId cardId(const LoadCaseSubcaseLabelCard& card) noexcept { return card.id; }
    std::uint32_t append(const LoadCaseSubcaseLabelCard& card, base::CardKey key);
    [[nodiscard]] LoadCaseSubcaseLabelCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const LoadCaseSubcaseLabelCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<LoadCaseSubcaseLabelCard> m_cards;
};

} // namespace oor::model
