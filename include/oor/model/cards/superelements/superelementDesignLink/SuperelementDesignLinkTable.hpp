#pragma once

#include "oor/model/cards/superelements/superelementDesignLink/SuperelementDesignLinkCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementDesignLinkFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementDesignLinkCard& card);
    [[nodiscard]] static std::optional<SuperelementDesignLinkCard> read(FieldReader& reader);
    static void write(const SuperelementDesignLinkCard& card, FieldWriter& writer);
};

// All the SEDLINK cards, filed under their ID (IdSpace::PerCardType, category Other like DLINK). The cards are kept
// whole next to a column of their IDs.
class SuperelementDesignLinkTable final : public TypedTable<SuperelementDesignLinkTable, SuperelementDesignLinkCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementDesignLink";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = SuperelementDesignLinkFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementDesignLinkCard& card) noexcept { return card.id; }
    std::uint32_t append(const SuperelementDesignLinkCard& card, base::CardKey key);
    [[nodiscard]] SuperelementDesignLinkCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementDesignLinkCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    // DSEID and every ISEIDi are superelements; each design variable is a DESVAR of its superelement.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<SuperelementDesignLinkCard> m_cards;
};

} // namespace oor::model
