#pragma once

#include "oor/model/cards/superelements/superelementUserDofSet1/SuperelementUserDofSet1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct SuperelementUserDofSet1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementUserDofSet1Card& card);
    [[nodiscard]] static std::optional<SuperelementUserDofSet1Card> read(FieldReader& reader);
    static void write(const SuperelementUserDofSet1Card& card, FieldWriter& writer);
};

// All the SEUSET1 cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to columns
// of their SEIDs and set names.
class SuperelementUserDofSet1Table final : public TypedTable<SuperelementUserDofSet1Table, SuperelementUserDofSet1Card>
{
public:
    static constexpr std::string_view NAME = "SuperelementUserDofSet1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementUserDofSet1Format;

    [[nodiscard]] static base::EntityId cardId(const SuperelementUserDofSet1Card&) noexcept { return 0; }
    std::uint32_t append(const SuperelementUserDofSet1Card& card, base::CardKey key);
    [[nodiscard]] SuperelementUserDofSet1Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementUserDofSet1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID is a superelement and the points are those of it; the ends of a THRU range may not exist.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }
    [[nodiscard]] Column<std::string>& setNameColumn() noexcept { return m_setName; }

private:
    Column<base::EntityId> m_superelementId;
    Column<std::string> m_setName;
    CardRows<SuperelementUserDofSet1Card> m_cards;
};

} // namespace oor::model
