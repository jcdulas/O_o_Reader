#pragma once

#include "oor/model/cards/explicit/cohesiveFriction/CohesiveFrictionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CohesiveFrictionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CohesiveFrictionCard& card);
    [[nodiscard]] static std::optional<CohesiveFrictionCard> read(FieldReader& reader);
    static void write(const CohesiveFrictionCard& card, FieldWriter& writer);
};

// All the COHFRIC cards: the CID is unique among them (IdSpace::PerCardType, category CohesiveFriction), and a COUCOHF
// names it in its COHFRICID.
class CohesiveFrictionTable final : public TypedTable<CohesiveFrictionTable, CohesiveFrictionCard>
{
public:
    static constexpr std::string_view NAME = "CohesiveFriction";
    static inline const CardTypeInfo INFO{NAME, CardCategory::CohesiveFriction, base::IdSpace::PerCardType};
    using Format = CohesiveFrictionFormat;

    [[nodiscard]] static base::EntityId cardId(const CohesiveFrictionCard& card) noexcept { return card.id; }
    std::uint32_t append(const CohesiveFrictionCard& card, base::CardKey key);
    [[nodiscard]] CohesiveFrictionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CohesiveFrictionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a COHFRIC refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CohesiveFrictionCard> m_cards;
};

} // namespace oor::model
