#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearUserOutput/NonlinearUserOutputCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearUserOutputFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearUserOutputCard& card);
    [[nodiscard]] static std::optional<NonlinearUserOutputCard> read(FieldReader& reader);
    static void write(const NonlinearUserOutputCard& card, FieldWriter& writer);
};

// All the NLOUTUD cards. The NID is unique among them (IdSpace::PerCardType). The IDs of the continuation lines are
// references to elements (UNAME = EEXOUT) or to grid points (GEXOUT). The cards are kept whole (CardRows); the query
// engine reads the NID only.
class NonlinearUserOutputTable final : public TypedTable<NonlinearUserOutputTable, NonlinearUserOutputCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearUserOutput";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = NonlinearUserOutputFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearUserOutputCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonlinearUserOutputCard& card, base::CardKey key);
    [[nodiscard]] NonlinearUserOutputCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const NonlinearUserOutputCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<NonlinearUserOutputCard> m_cards;
};

} // namespace oor::model
