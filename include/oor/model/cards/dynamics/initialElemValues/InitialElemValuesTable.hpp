#pragma once

#include "oor/model/cards/dynamics/initialElemValues/InitialElemValuesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct InitialElemValuesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const InitialElemValuesCard& card);
    [[nodiscard]] static std::optional<InitialElemValuesCard> read(FieldReader& reader);
    static void write(const InitialElemValuesCard& card, FieldWriter& writer);
};

// All the TICEL cards. The SID is a set ID shared by the cards of the set (IdSpace::Group, category InitialCondition:
// the IC command selects the set, as for the TIC). SETID names a SET1 of the elements. The cards are kept whole
// (CardRows) next to their ID, because the list of variables has a length of its own.
class InitialElemValuesTable final : public TypedTable<InitialElemValuesTable, InitialElemValuesCard>
{
public:
    static constexpr std::string_view NAME = "InitialElemValues";
    static inline const CardTypeInfo INFO{NAME, CardCategory::InitialCondition, base::IdSpace::Group};
    using Format = InitialElemValuesFormat;

    [[nodiscard]] static base::EntityId cardId(const InitialElemValuesCard& card) noexcept { return card.id; }
    std::uint32_t append(const InitialElemValuesCard& card, base::CardKey key);
    [[nodiscard]] InitialElemValuesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const InitialElemValuesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<InitialElemValuesCard> m_cards;
};

} // namespace oor::model
