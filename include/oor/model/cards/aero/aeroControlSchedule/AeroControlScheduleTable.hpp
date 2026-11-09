#pragma once

#include "oor/model/cards/aero/aeroControlSchedule/AeroControlScheduleCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroControlScheduleFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroControlScheduleCard& card);
    [[nodiscard]] static std::optional<AeroControlScheduleCard> read(FieldReader& reader);
    static void write(const AeroControlScheduleCard& card, FieldWriter& writer);
};

// All the CSSCHD cards, in the category ControlSchedule. The SID is unique among the CSSCHD cards
// (IdSpace::PerCardType) and is what the Case Control CSSCHD names (ReferenceTarget::ControlSchedule). AESID is an
// AESURF (ReferenceTarget::AeroControlSurface); LALPHA, LMACH and LSCHD are AEFACT cards (ReferenceTarget::AeroFactor).
// The card is kept whole next to a column of its SID.
class AeroControlScheduleTable final : public TypedTable<AeroControlScheduleTable, AeroControlScheduleCard>
{
public:
    static constexpr std::string_view NAME = "AeroControlSchedule";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ControlSchedule, base::IdSpace::PerCardType};
    using Format = AeroControlScheduleFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroControlScheduleCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroControlScheduleCard& card, base::CardKey key);
    [[nodiscard]] AeroControlScheduleCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroControlScheduleCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroControlScheduleCard> m_cards;
};

} // namespace oor::model
