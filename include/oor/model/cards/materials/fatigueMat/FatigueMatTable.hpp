#pragma once

#include "oor/model/cards/materials/fatigueMat/FatigueMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FatigueMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FatigueMatCard& card);
    [[nodiscard]] static std::optional<FatigueMatCard> read(FieldReader& reader);
    static void write(const FatigueMatCard& card, FieldWriter& writer);
};

// All the MATFTG cards. The ID space is the card type's own: the ID is the one of the MAT1 that the card serves, so it
// is a reference to a material (renumbering the materials follows into it), except for a card with a MATID line, whose
// ID is its own and whose single MIDi are the references (the ranges of a THRU are left as written). The tables of the
// TABLE line may be TABLEM1 or TABRND1 cards, so they are plain IDs. The cards are kept whole (CardRows); the query
// engine reads the ID only.
class FatigueMatTable final : public TypedTable<FatigueMatTable, FatigueMatCard>
{
public:
    static constexpr std::string_view NAME = "FatigueMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = FatigueMatFormat;

    [[nodiscard]] static base::EntityId cardId(const FatigueMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const FatigueMatCard& card, base::CardKey key);
    [[nodiscard]] FatigueMatCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FatigueMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FatigueMatCard> m_cards;
};

} // namespace oor::model
