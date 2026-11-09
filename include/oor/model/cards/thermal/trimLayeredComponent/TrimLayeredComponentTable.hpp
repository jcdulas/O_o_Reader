#pragma once

#include "oor/model/cards/thermal/trimLayeredComponent/TrimLayeredComponentCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TrimLayeredComponentFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TrimLayeredComponentCard& card);
    [[nodiscard]] static std::optional<TrimLayeredComponentCard> read(FieldReader& reader);
    static void write(const TrimLayeredComponentCard& card, FieldWriter& writer);
};

// All the TCOMPG cards, in the category Other. The ID of a card is its SET3ID, a reference to a SET3
// (ReferenceTarget::Set: a renumbering of the sets follows into it and moves the ID with it), shared by the cards that
// name the same SET3 (IdSpace::Group). Each MID is a reference to a material. The cards are kept whole next to a column
// of their IDs.
class TrimLayeredComponentTable final : public TypedTable<TrimLayeredComponentTable, TrimLayeredComponentCard>
{
public:
    static constexpr std::string_view NAME = "TrimLayeredComponent";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = TrimLayeredComponentFormat;

    [[nodiscard]] static base::EntityId cardId(const TrimLayeredComponentCard& card) noexcept { return card.id; }
    std::uint32_t append(const TrimLayeredComponentCard& card, base::CardKey key);
    [[nodiscard]] TrimLayeredComponentCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TrimLayeredComponentCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TrimLayeredComponentCard> m_cards;
};

} // namespace oor::model
