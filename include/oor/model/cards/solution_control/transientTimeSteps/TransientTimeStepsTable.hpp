#pragma once

#include "oor/model/cards/solution_control/transientTimeSteps/TransientTimeStepsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TransientTimeStepsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TransientTimeStepsCard& card);
    [[nodiscard]] static std::optional<TransientTimeStepsCard> read(FieldReader& reader);
    static void write(const TransientTimeStepsCard& card, FieldWriter& writer);
};

// All the TSTEP cards. The ID is the one a TSTEP command of the Case Control selects: it is unique among the TSTEP
// cards (IdSpace::PerCardType) and shared in the category TimeStep with the TSTEPNL cards, which the same command can
// select. The three values of each interval are a list per card each, in CSR form.
class TransientTimeStepsTable final : public TypedTable<TransientTimeStepsTable, TransientTimeStepsCard>
{
public:
    static constexpr std::string_view NAME = "TransientTimeSteps";
    static inline const CardTypeInfo INFO{NAME, CardCategory::TimeStep, base::IdSpace::PerCardType};
    using Format = TransientTimeStepsFormat;

    [[nodiscard]] static base::EntityId cardId(const TransientTimeStepsCard& card) noexcept { return card.id; }
    std::uint32_t append(const TransientTimeStepsCard& card, base::CardKey key);
    [[nodiscard]] TransientTimeStepsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const TransientTimeStepsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a TSTEP refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& stepCountsColumn() noexcept { return m_stepCounts; }
    [[nodiscard]] VariableLengthColumn<double>& timeStepsColumn() noexcept { return m_timeSteps; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& outputIntervalsColumn() noexcept { return m_outputIntervals; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<std::int32_t> m_stepCounts;
    VariableLengthColumn<double> m_timeSteps;
    VariableLengthColumn<std::int32_t> m_outputIntervals;
};

} // namespace oor::model
