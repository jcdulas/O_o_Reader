#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqSteps/NonlinearHarmonicFreqStepsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearHarmonicFreqStepsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearHarmonicFreqStepsCard& card);
    [[nodiscard]] static std::optional<NonlinearHarmonicFreqStepsCard> read(FieldReader& reader);
    static void write(const NonlinearHarmonicFreqStepsCard& card, FieldWriter& writer);
};

// All the NLFREQ1 cards. The ID is unique among them (IdSpace::PerCardType) and numbers, in the category
// NonlinearFrequency, the same selection as the NLFREQ and NLFREQC cards.
class NonlinearHarmonicFreqStepsTable final
    : public TypedTable<NonlinearHarmonicFreqStepsTable, NonlinearHarmonicFreqStepsCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearHarmonicFreqSteps";
    static inline const CardTypeInfo INFO{NAME, CardCategory::NonlinearFrequency, base::IdSpace::PerCardType};
    using Format = NonlinearHarmonicFreqStepsFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearHarmonicFreqStepsCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonlinearHarmonicFreqStepsCard& card, base::CardKey key);
    [[nodiscard]] NonlinearHarmonicFreqStepsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NonlinearHarmonicFreqStepsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an NLFREQ1 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& startFreqColumn() noexcept { return m_startFreq; }
    [[nodiscard]] Column<double>& freqStepColumn() noexcept { return m_freqStep; }
    [[nodiscard]] Column<std::int32_t>& stepCountColumn() noexcept { return m_stepCount; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_startFreq;
    Column<double> m_freqStep;
    Column<std::int32_t> m_stepCount{1};
};

} // namespace oor::model
