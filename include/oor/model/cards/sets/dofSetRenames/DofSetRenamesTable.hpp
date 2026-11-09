#pragma once

#include "oor/model/cards/sets/dofSetRenames/DofSetRenamesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

// The longest name of a set: one field holds four characters.
inline constexpr std::size_t DOF_SET_NAME_LENGTH = 4;
// The most renamings a card holds: the eight fields of the first line.
inline constexpr std::size_t DOF_SET_MAX_RENAMES = 4;

struct DofSetRenamesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DofSetRenamesCard& card);
    [[nodiscard]] static std::optional<DofSetRenamesCard> read(FieldReader& reader);
    static void write(const DofSetRenamesCard& card, FieldWriter& writer);
};

// All the DEFUSET cards, in the category Set. A card has no ID (IdSpace::None) and names no other card: the names are
// the solver's own names of sets, not cards of the model. The cards are kept whole next to a column of the number of
// their renamings, which is what the query engine reads.
class DofSetRenamesTable final : public TypedTable<DofSetRenamesTable, DofSetRenamesCard>
{
public:
    static constexpr std::string_view NAME = "DofSetRenames";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::None};
    using Format = DofSetRenamesFormat;

    [[nodiscard]] static base::EntityId cardId(const DofSetRenamesCard&) noexcept { return 0; }
    std::uint32_t append(const DofSetRenamesCard& card, base::CardKey key);
    [[nodiscard]] DofSetRenamesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DofSetRenamesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the names are not cards

    [[nodiscard]] Column<std::int32_t>& renameCountColumn() noexcept { return m_renameCount; }

private:
    Column<std::int32_t> m_renameCount;
    CardRows<DofSetRenamesCard> m_cards;
};

} // namespace oor::model
