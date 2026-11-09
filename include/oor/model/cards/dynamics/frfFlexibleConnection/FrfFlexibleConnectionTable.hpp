#pragma once

#include "oor/model/cards/dynamics/frfFlexibleConnection/FrfFlexibleConnectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FrfFlexibleConnectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FrfFlexibleConnectionCard& card);
    [[nodiscard]] static std::optional<FrfFlexibleConnectionCard> read(FieldReader& reader);
    static void write(const FrfFlexibleConnectionCard& card, FieldWriter& writer);
};

// All the FRFFLEX cards. The FLEXID is unique among them (IdSpace::PerCardType, category Other). A component given by
// its ID is a reference to an FRFCOMP (ReferenceTarget::FrfComponent), one given by its name is kept as written; the
// KTABID, BTABID and GETABID are references to TABLEDi cards (ReferenceTarget::DynamicTable); the points are plain
// numbers. The cards are kept whole next to columns of their IDs and component numbers.
class FrfFlexibleConnectionTable final : public TypedTable<FrfFlexibleConnectionTable, FrfFlexibleConnectionCard>
{
public:
    static constexpr std::string_view NAME = "FrfFlexibleConnection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = FrfFlexibleConnectionFormat;

    [[nodiscard]] static base::EntityId cardId(const FrfFlexibleConnectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const FrfFlexibleConnectionCard& card, base::CardKey key);
    [[nodiscard]] FrfFlexibleConnectionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FrfFlexibleConnectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& componentColumn() noexcept { return m_components; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_components;
    CardRows<FrfFlexibleConnectionCard> m_cards;
};

} // namespace oor::model
