#pragma once

#include "oor/model/cards/nonlinear_contact/brakeMotion/BrakeMotionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BrakeMotionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BrakeMotionCard& card);
    [[nodiscard]] static std::optional<BrakeMotionCard> read(FieldReader& reader);
    static void write(const BrakeMotionCard& card, FieldWriter& writer);
};

// All the MOTION cards. The ID is unique among them (IdSpace::PerCardType, category BrakeMotion): the MT1_ID and MT2_ID
// of a BRKSYS name it. The card names no other card. The cards are kept whole next to their ID.
class BrakeMotionTable final : public TypedTable<BrakeMotionTable, BrakeMotionCard>
{
public:
    static constexpr std::string_view NAME = "BrakeMotion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::BrakeMotion, base::IdSpace::PerCardType};
    using Format = BrakeMotionFormat;

    [[nodiscard]] static base::EntityId cardId(const BrakeMotionCard& card) noexcept { return card.id; }
    std::uint32_t append(const BrakeMotionCard& card, base::CardKey key);
    [[nodiscard]] BrakeMotionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const BrakeMotionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field names another card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<BrakeMotionCard> m_cards;
};

} // namespace oor::model
