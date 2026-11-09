#pragma once

#include "oor/model/cards/superelements/superelementTree/SuperelementTreeCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementTreeFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementTreeCard& card);
    [[nodiscard]] static std::optional<SuperelementTreeCard> read(FieldReader& reader);
    static void write(const SuperelementTreeCard& card, FieldWriter& writer);
};

// All the SETREE cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to a column
// of their SEIDs.
class SuperelementTreeTable final : public TypedTable<SuperelementTreeTable, SuperelementTreeCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementTree";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementTreeFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementTreeCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementTreeCard& card, base::CardKey key);
    [[nodiscard]] SuperelementTreeCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementTreeCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID and every SEUPi are superelements.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_superelementId;
    CardRows<SuperelementTreeCard> m_cards;
};

} // namespace oor::model
