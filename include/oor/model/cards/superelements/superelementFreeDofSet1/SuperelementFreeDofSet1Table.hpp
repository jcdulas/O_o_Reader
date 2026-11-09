#pragma once

#include "oor/model/cards/superelements/superelementFreeDofSet1/SuperelementFreeDofSet1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementFreeDofSet1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementFreeDofSet1Card& card);
    [[nodiscard]] static std::optional<SuperelementFreeDofSet1Card> read(FieldReader& reader);
    static void write(const SuperelementFreeDofSet1Card& card, FieldWriter& writer);
};

// All the SECSET1 cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to a
// column of their SEIDs.
class SuperelementFreeDofSet1Table final : public TypedTable<SuperelementFreeDofSet1Table, SuperelementFreeDofSet1Card>
{
public:
    static constexpr std::string_view NAME = "SuperelementFreeDofSet1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementFreeDofSet1Format;

    [[nodiscard]] static base::EntityId cardId(const SuperelementFreeDofSet1Card&) noexcept { return 0; }
    std::uint32_t append(const SuperelementFreeDofSet1Card& card, base::CardKey key);
    [[nodiscard]] SuperelementFreeDofSet1Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementFreeDofSet1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID is a superelement and the points are those of it. The alternate forms accept points that do not exist,
    // so every point is looked up as the end of a range is.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementFreeDofSet1Card> m_cards;
};

} // namespace oor::model
