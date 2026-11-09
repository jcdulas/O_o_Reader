#pragma once

#include "oor/model/cards/solution_control/SolutionFields.hpp"
#include "oor/model/cards/solution_control/nonlinearTimeSteps/NonlinearTimeStepsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct NonlinearTimeStepsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearTimeStepsCard& card);
    [[nodiscard]] static std::optional<NonlinearTimeStepsCard> read(FieldReader& reader);
    static void write(const NonlinearTimeStepsCard& card, FieldWriter& writer);
};

// All the TSTEPNL cards. The ID is the one a TSTEP command of the Case Control selects: it is unique among the TSTEPNL
// cards (IdSpace::PerCardType) and shared in the category TimeStep with the TSTEP cards. A field that is not given is
// NO_INTEGER or NaN in its column.
class NonlinearTimeStepsTable final : public TypedTable<NonlinearTimeStepsTable, NonlinearTimeStepsCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearTimeSteps";
    static inline const CardTypeInfo INFO{NAME, CardCategory::TimeStep, base::IdSpace::PerCardType};
    using Format = NonlinearTimeStepsFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearTimeStepsCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonlinearTimeStepsCard& card, base::CardKey key);
    [[nodiscard]] NonlinearTimeStepsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NonlinearTimeStepsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a TSTEPNL refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& stepCountColumn() noexcept { return m_stepCount; }
    [[nodiscard]] Column<double>& timeStepColumn() noexcept { return m_timeStep; }
    [[nodiscard]] Column<std::int32_t>& outputIntervalColumn() noexcept { return m_outputInterval; }
    [[nodiscard]] Column<std::uint8_t>& methodColumn() noexcept { return m_method; }
    [[nodiscard]] Column<std::int32_t>& stiffnessUpdateStepColumn() noexcept { return m_stiffnessUpdateStep; }
    [[nodiscard]] Column<std::int32_t>& maxIterationsColumn() noexcept { return m_maxIterations; }
    [[nodiscard]] Column<std::string>& convergenceFlagsColumn() noexcept { return m_convergenceFlags; }
    [[nodiscard]] Column<double>& epsilonUColumn() noexcept { return m_epsilonU; }
    [[nodiscard]] Column<double>& epsilonPColumn() noexcept { return m_epsilonP; }
    [[nodiscard]] Column<double>& epsilonWColumn() noexcept { return m_epsilonW; }
    [[nodiscard]] Column<std::int32_t>& maxDivergencesColumn() noexcept { return m_maxDivergences; }
    [[nodiscard]] Column<std::int32_t>& maxQuasiNewtonVectorsColumn() noexcept { return m_maxQuasiNewtonVectors; }
    [[nodiscard]] Column<std::int32_t>& maxLineSearchesColumn() noexcept { return m_maxLineSearches; }
    [[nodiscard]] Column<double>& stressSubincrementFractionColumn() noexcept { return m_stressSubincrementFraction; }
    [[nodiscard]] Column<std::int32_t>& maxBisectionsColumn() noexcept { return m_maxBisections; }
    [[nodiscard]] Column<std::int32_t>& adjustIntervalColumn() noexcept { return m_adjustInterval; }
    [[nodiscard]] Column<std::int32_t>& minStepsPerCycleColumn() noexcept { return m_minStepsPerCycle; }
    [[nodiscard]] Column<double>& stepBoundsColumn() noexcept { return m_stepBounds; }
    [[nodiscard]] Column<double>& maxRotationRatioColumn() noexcept { return m_maxRotationRatio; }
    [[nodiscard]] Column<double>& displacementToleranceColumn() noexcept { return m_displacementTolerance; }
    [[nodiscard]] Column<double>& rotationToleranceColumn() noexcept { return m_rotationTolerance; }
    [[nodiscard]] Column<std::int32_t>& minIterationsColumn() noexcept { return m_minIterations; }

private:
    static constexpr double BLANK = std::numeric_limits<double>::quiet_NaN();

    Column<base::EntityId> m_id;
    Column<std::int32_t> m_stepCount;
    Column<double> m_timeStep;
    Column<std::int32_t> m_outputInterval{1};
    Column<std::uint8_t> m_method;
    Column<std::int32_t> m_stiffnessUpdateStep{NO_INTEGER};
    Column<std::int32_t> m_maxIterations{NO_INTEGER};
    Column<std::string> m_convergenceFlags;
    Column<double> m_epsilonU{1.0E-2};
    Column<double> m_epsilonP{BLANK};
    Column<double> m_epsilonW{BLANK};
    Column<std::int32_t> m_maxDivergences{2};
    Column<std::int32_t> m_maxQuasiNewtonVectors{10};
    Column<std::int32_t> m_maxLineSearches{2};
    Column<double> m_stressSubincrementFraction{0.2};
    Column<std::int32_t> m_maxBisections{NO_INTEGER};
    Column<std::int32_t> m_adjustInterval{5};
    Column<std::int32_t> m_minStepsPerCycle{NO_INTEGER};
    Column<double> m_stepBounds{0.6};
    Column<double> m_maxRotationRatio{32.0};
    Column<double> m_displacementTolerance{0.1};
    Column<double> m_rotationTolerance{20.0};
    Column<std::int32_t> m_minIterations{NO_INTEGER};
};

} // namespace oor::model
