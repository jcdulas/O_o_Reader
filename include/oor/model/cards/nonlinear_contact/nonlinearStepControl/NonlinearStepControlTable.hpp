#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearStepControl/NonlinearStepControlCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct NonlinearStepControlFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearStepControlCard& card);
    [[nodiscard]] static std::optional<NonlinearStepControlCard> read(FieldReader& reader);
    static void write(const NonlinearStepControlCard& card, FieldWriter& writer);

    // The names of the fields, with what each holds.
    [[nodiscard]] static std::span<const ContactParameterRule> rules();

    // The CRITTID of the ADAPT line, the TABSCTL that holds the user criteria of the load stepping: 0 when the card
    // has no ADAPT line, or no CRITTID on it, or a CRITTID of 0.
    [[nodiscard]] static base::EntityId criteriaTable(const NonlinearStepControlCard& card);
};

// All the NLSTEP cards. The ID is unique among them (IdSpace::PerCardType, category NonlinearStep): the NLSTEP command
// of Case Control selects it. The cards are kept whole (CardRows); the query engine reads the ID only. The CRITTID of
// the ADAPT line names a TABSCTL (`LoadStepControl`) and is the one reference of the card.
class NonlinearStepControlTable final : public TypedTable<NonlinearStepControlTable, NonlinearStepControlCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearStepControl";
    static inline const CardTypeInfo INFO{NAME, CardCategory::NonlinearStep, base::IdSpace::PerCardType};
    using Format = NonlinearStepControlFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearStepControlCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonlinearStepControlCard& card, base::CardKey key);
    [[nodiscard]] NonlinearStepControlCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const NonlinearStepControlCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& totalTimeColumn() noexcept { return m_totalTime; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_totalTime{1.0};
    CardRows<NonlinearStepControlCard> m_cards;
};

} // namespace oor::model
