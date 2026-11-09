#pragma once

#include "oor/model/cards/nonlinear_contact/brakeSystem/BrakeSystemCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BrakeSystemFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BrakeSystemCard& card);
    [[nodiscard]] static std::optional<BrakeSystemCard> read(FieldReader& reader);
    static void write(const BrakeSystemCard& card, FieldWriter& writer);
};

// All the BRKSYS cards. The ID is unique among them (IdSpace::PerCardType) and is in the category BrakeSqueal, as the
// ID of a BSQUEAL is: the Case Control BSQUEAL selects either. The bodies of the disks are looked up as contact bodies,
// the motions as MOTION cards and the property as a BRKPROP. The cards are kept whole next to their ID.
class BrakeSystemTable final : public TypedTable<BrakeSystemTable, BrakeSystemCard>
{
public:
    static constexpr std::string_view NAME = "BrakeSystem";
    static inline const CardTypeInfo INFO{NAME, CardCategory::BrakeSqueal, base::IdSpace::PerCardType};
    using Format = BrakeSystemFormat;

    [[nodiscard]] static base::EntityId cardId(const BrakeSystemCard& card) noexcept { return card.id; }
    std::uint32_t append(const BrakeSystemCard& card, base::CardKey key);
    [[nodiscard]] BrakeSystemCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const BrakeSystemCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<BrakeSystemCard> m_cards;
};

} // namespace oor::model
