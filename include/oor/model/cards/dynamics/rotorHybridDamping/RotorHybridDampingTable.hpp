#pragma once

#include "oor/model/cards/dynamics/rotorHybridDamping/RotorHybridDampingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RotorHybridDampingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RotorHybridDampingCard& card);
    [[nodiscard]] static std::optional<RotorHybridDampingCard> read(FieldReader& reader);
    static void write(const RotorHybridDampingCard& card, FieldWriter& writer);
};

// All the ROTHYBD cards (category Other). A card has no ID of its own (IdSpace::None), so there may be many. The
// HYBDAMPi are references to HYBDAMP cards (ReferenceTarget::HybridDamping) and the ROTORIDi to rotors
// (ReferenceTarget::Rotor: a ROTORG, ROTOR, ROTORAX or ROTORSE). The cards
// are kept whole next to a column of the number of their pairs, which is what the query engine reads.
class RotorHybridDampingTable final : public TypedTable<RotorHybridDampingTable, RotorHybridDampingCard>
{
public:
    static constexpr std::string_view NAME = "RotorHybridDamping";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = RotorHybridDampingFormat;

    [[nodiscard]] static base::EntityId cardId(const RotorHybridDampingCard&) noexcept { return 0; }
    std::uint32_t append(const RotorHybridDampingCard& card, base::CardKey key);
    [[nodiscard]] RotorHybridDampingCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RotorHybridDampingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::int32_t>& pairCountColumn() noexcept { return m_pairCount; }

private:
    Column<std::int32_t> m_pairCount;
    CardRows<RotorHybridDampingCard> m_cards;
};

} // namespace oor::model
