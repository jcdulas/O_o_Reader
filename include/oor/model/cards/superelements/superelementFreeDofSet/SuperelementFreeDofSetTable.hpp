#pragma once

#include "oor/model/cards/superelements/superelementFreeDofSet/SuperelementFreeDofSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementFreeDofSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementFreeDofSetCard& card);
    [[nodiscard]] static std::optional<SuperelementFreeDofSetCard> read(FieldReader& reader);
    static void write(const SuperelementFreeDofSetCard& card, FieldWriter& writer);
};

// All the SECSET cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to a column
// of their SEIDs.
class SuperelementFreeDofSetTable final : public TypedTable<SuperelementFreeDofSetTable, SuperelementFreeDofSetCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementFreeDofSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementFreeDofSetFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementFreeDofSetCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementFreeDofSetCard& card, base::CardKey key);
    [[nodiscard]] SuperelementFreeDofSetCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementFreeDofSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID is a superelement and the points are those of it.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementFreeDofSetCard> m_cards;
};

} // namespace oor::model
