#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearOutputItems/NonlinearOutputItemsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearOutputItemsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearOutputItemsCard& card);
    [[nodiscard]] static std::optional<NonlinearOutputItemsCard> read(FieldReader& reader);
    static void write(const NonlinearOutputItemsCard& card, FieldWriter& writer);
};

// All the NLOUT cards. The ID is unique among them (IdSpace::PerCardType). The NLSTRESS command of Case Control names
// it with a keyword that the Case Control commands do not take apart, so no reference is resolved. The cards are kept
// whole (CardRows); the query engine reads the ID only.
class NonlinearOutputItemsTable final : public TypedTable<NonlinearOutputItemsTable, NonlinearOutputItemsCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearOutputItems";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = NonlinearOutputItemsFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearOutputItemsCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonlinearOutputItemsCard& card, base::CardKey key);
    [[nodiscard]] NonlinearOutputItemsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const NonlinearOutputItemsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an NLOUT refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<NonlinearOutputItemsCard> m_cards;
};

} // namespace oor::model
