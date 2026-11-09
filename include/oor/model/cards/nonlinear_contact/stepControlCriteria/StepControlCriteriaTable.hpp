#pragma once

#include "oor/model/cards/nonlinear_contact/stepControlCriteria/StepControlCriteriaCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct StepControlCriteriaFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const StepControlCriteriaCard& card);
    [[nodiscard]] static std::optional<StepControlCriteriaCard> read(FieldReader& reader);
    static void write(const StepControlCriteriaCard& card, FieldWriter& writer);
};

// All the TABSCTL cards. The ID is unique among them (IdSpace::PerCardType, category LoadStepControl): the CRITTID of
// an NLSTEP names it. The SET3_IDi of a criterion name a SET3 (`Set`). The cards are kept whole next to their ID.
class StepControlCriteriaTable final : public TypedTable<StepControlCriteriaTable, StepControlCriteriaCard>
{
public:
    static constexpr std::string_view NAME = "StepControlCriteria";
    static inline const CardTypeInfo INFO{NAME, CardCategory::LoadStepControl, base::IdSpace::PerCardType};
    using Format = StepControlCriteriaFormat;

    [[nodiscard]] static base::EntityId cardId(const StepControlCriteriaCard& card) noexcept { return card.id; }
    std::uint32_t append(const StepControlCriteriaCard& card, base::CardKey key);
    [[nodiscard]] StepControlCriteriaCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const StepControlCriteriaCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<StepControlCriteriaCard> m_cards;
};

} // namespace oor::model
