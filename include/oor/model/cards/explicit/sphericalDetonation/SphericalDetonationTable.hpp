#pragma once

#include "oor/model/cards/explicit/sphericalDetonation/SphericalDetonationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SphericalDetonationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SphericalDetonationCard& card);
    [[nodiscard]] static std::optional<SphericalDetonationCard> read(FieldReader& reader);
    static void write(const SphericalDetonationCard& card, FieldWriter& writer);
};

// All the DETSPH cards. The DID is unique among them (IdSpace::PerCardType) and no other card names it, so the category
// is Other. The MID is the MATDEUL of the explosive. The cards are kept whole (CardRows) next to their ID.
class SphericalDetonationTable final : public TypedTable<SphericalDetonationTable, SphericalDetonationCard>
{
public:
    static constexpr std::string_view NAME = "SphericalDetonation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = SphericalDetonationFormat;

    [[nodiscard]] static base::EntityId cardId(const SphericalDetonationCard& card) noexcept { return card.id; }
    std::uint32_t append(const SphericalDetonationCard& card, base::CardKey key);
    [[nodiscard]] SphericalDetonationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SphericalDetonationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<SphericalDetonationCard> m_cards;
};

} // namespace oor::model
