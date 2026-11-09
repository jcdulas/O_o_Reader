#pragma once

#include "oor/model/cards/explicit/eulerBarrier/EulerBarrierCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerBarrierFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerBarrierCard& card);
    [[nodiscard]] static std::optional<EulerBarrierCard> read(FieldReader& reader);
    static void write(const EulerBarrierCard& card, FieldWriter& writer);
};

// All the BARRIER cards. The BID is unique among them (IdSpace::PerCardType) and no other card names it, so the
// category is Other. The BCID names a set of BCSEG cards (ContactSurface) and the MESH an Euler mesh. The cards are
// kept whole (CardRows) next to their ID.
class EulerBarrierTable final : public TypedTable<EulerBarrierTable, EulerBarrierCard>
{
public:
    static constexpr std::string_view NAME = "EulerBarrier";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = EulerBarrierFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerBarrierCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerBarrierCard& card, base::CardKey key);
    [[nodiscard]] EulerBarrierCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerBarrierCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerBarrierCard> m_cards;
};

} // namespace oor::model
