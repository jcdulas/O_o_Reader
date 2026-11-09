#pragma once

#include "oor/model/cards/rigid_elements/rigidInterpolationOverride/RigidInterpolationOverrideCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RigidInterpolationOverrideFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidInterpolationOverrideCard& card);
    [[nodiscard]] static std::optional<RigidInterpolationOverrideCard> read(FieldReader& reader);
    static void write(const RigidInterpolationOverrideCard& card, FieldWriter& writer);
};

// All the GMOVR3 cards. A card has no ID (IdSpace::None, category Other: it is not an element, though it names RBE3
// elements) and any number of them may be in a deck. The ELEMi are `Element` references. The cards are kept whole.
class RigidInterpolationOverrideTable final
    : public TypedTable<RigidInterpolationOverrideTable, RigidInterpolationOverrideCard>
{
public:
    static constexpr std::string_view NAME = "RigidInterpolationOverride";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = RigidInterpolationOverrideFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidInterpolationOverrideCard&) noexcept { return 0; }
    std::uint32_t append(const RigidInterpolationOverrideCard& card, base::CardKey key);
    [[nodiscard]] RigidInterpolationOverrideCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RigidInterpolationOverrideCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

private:
    CardRows<RigidInterpolationOverrideCard> m_cards;
};

} // namespace oor::model
