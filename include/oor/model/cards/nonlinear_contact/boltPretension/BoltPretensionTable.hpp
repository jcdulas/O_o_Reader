#pragma once

#include "oor/model/cards/nonlinear_contact/boltPretension/BoltPretensionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BoltPretensionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BoltPretensionCard& card);
    [[nodiscard]] static std::optional<BoltPretensionCard> read(FieldReader& reader);
    static void write(const BoltPretensionCard& card, FieldWriter& writer);
};

// All the BOLT1 cards. The ID is unique among them (IdSpace::PerCardType, category Other). The cards are kept whole
// (CardRows); the query engine reads the ID only. The control grid point is a reference; the elements and grid points
// are too, or a SET3 with IDTYPE SET.
class BoltPretensionTable final : public TypedTable<BoltPretensionTable, BoltPretensionCard>
{
public:
    static constexpr std::string_view NAME = "BoltPretension";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = BoltPretensionFormat;

    [[nodiscard]] static base::EntityId cardId(const BoltPretensionCard& card) noexcept { return card.id; }
    std::uint32_t append(const BoltPretensionCard& card, base::CardKey key);
    [[nodiscard]] BoltPretensionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const BoltPretensionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<BoltPretensionCard> m_cards;
};

} // namespace oor::model
