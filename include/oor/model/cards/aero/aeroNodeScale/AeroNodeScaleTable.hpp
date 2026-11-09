#pragma once

#include "oor/model/cards/aero/aeroNodeScale/AeroNodeScaleCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroNodeScaleFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroNodeScaleCard& card);
    [[nodiscard]] static std::optional<AeroNodeScaleCard> read(FieldReader& reader);
    static void write(const AeroNodeScaleCard& card, FieldWriter& writer);
};

// All the AESCALE cards, in the category AeroScale that the ASID of an AEGRID names (ReferenceTarget::AeroScale). The
// ASID is unique among the AESCALE cards (IdSpace::PerCardType). The card is kept whole next to its ID.
class AeroNodeScaleTable final : public TypedTable<AeroNodeScaleTable, AeroNodeScaleCard>
{
public:
    static constexpr std::string_view NAME = "AeroNodeScale";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroScale, base::IdSpace::PerCardType};
    using Format = AeroNodeScaleFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroNodeScaleCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroNodeScaleCard& card, base::CardKey key);
    [[nodiscard]] AeroNodeScaleCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroNodeScaleCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the card refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroNodeScaleCard> m_cards;
};

} // namespace oor::model
