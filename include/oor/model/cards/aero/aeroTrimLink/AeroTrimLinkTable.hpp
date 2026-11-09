#pragma once

#include "oor/model/cards/aero/aeroTrimLink/AeroTrimLinkCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroTrimLinkFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroTrimLinkCard& card);
    [[nodiscard]] static std::optional<AeroTrimLinkCard> read(FieldReader& reader);
    static void write(const AeroTrimLinkCard& card, FieldWriter& writer);
};

// All the AELINK cards, in the category Aero. The ID is the number of the trim set the relation applies to (0 for every
// subcase), shared by the cards of that trim: IdSpace::Group, so that `model.aelink[1]` is the list of the relations of
// the trim 1. The card names no card by number (the labels are names). It is kept whole next to a column of its ID.
class AeroTrimLinkTable final : public TypedTable<AeroTrimLinkTable, AeroTrimLinkCard>
{
public:
    static constexpr std::string_view NAME = "AeroTrimLink";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::Group};
    using Format = AeroTrimLinkFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroTrimLinkCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroTrimLinkCard& card, base::CardKey key);
    [[nodiscard]] AeroTrimLinkCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroTrimLinkCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    // A positive ID is no longer the word ALWAYS.
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // names and a selection, no reference

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroTrimLinkCard> m_cards;
};

} // namespace oor::model
